/*
   Copyright [2011] [Yao Yuan(yeaya@163.com)]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

package com.xixibase.cache.multi;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.SocketChannel;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

import com.xixibase.cache.CacheClientManager;
import com.xixibase.cache.CacheItem;
import com.xixibase.cache.Defines;
import com.xixibase.cache.TransCoder;
import com.xixibase.cache.XixiSocket;
import com.xixibase.util.Log;

public final class MultiGet extends Defines {
	private static Log log = Log.getLog(MultiGet.class.getName());

	private CacheClientManager manager;
	private int groupID;
	private TransCoder transCoder;
	
	private Selector selector;
	private int numConns = 0;
	private String lastError = null;
	
	public MultiGet(CacheClientManager manager, int groupID, TransCoder transCoder) {
		this.manager = manager;
		this.groupID = groupID;
		this.transCoder = transCoder;
	}
	
	public String getLastError() {
		return lastError;
	}

	public List<CacheItem> multiGet(final List<String> keys) {
		lastError = null;
		if (keys == null) {
			lastError = "multiGet, keys == null";
			log.error(lastError);
			return null;
		}
		if (keys.size() == 0) {
			lastError = "multiGet, keys.size() == 0";
			log.error(lastError);
			return new ArrayList<CacheItem>();
		}
		ArrayList<CacheItem> result = new ArrayList<CacheItem>(keys.size());
		for (int i = 0; i < keys.size(); i++) {
			result.add(null);
		}
		
		Map<String, Connection> conns = new HashMap<String, Connection>();
		try {
			Iterator<String> it = keys.iterator();
			int index = 0;
			while (it.hasNext()) {
				Integer keyIndex = new Integer(index);
				index++;
				String key = it.next();
				if (key == null) {
					lastError = "multiGet, key == null";
					log.error(lastError);
					continue;
				}
				byte[] keyBuf = transCoder.encodeKey(key);
				if (keyBuf == null) {
					lastError = "multiGet, failed to encode key";
					log.error(lastError);
					continue;
				}

				String host = manager.getHost(key);
				if (host == null) {
					lastError = "multiGet, can not get host with the key";
					log.error(lastError);
					continue;
				}

				Connection conn = conns.get(host);
				if (conn == null) {
					conn = new Connection();
					conns.put(host, conn);
				}
				conn.add(key, keyBuf, keyIndex);
			}

			selector = manager.selectorOpen();

			Iterator<Entry<String, Connection>> itc = conns.entrySet().iterator();
			while (itc.hasNext()) {
				Entry<String, Connection> e = itc.next();
				String host = e.getKey();
				Connection conn = e.getValue();
				
				XixiSocket socket = manager.getSocketByHost(host);
				if (socket != null) {
					conn.init(socket, result);
				}
			}

			numConns = conns.size();

			long startTime = System.currentTimeMillis();
			long timeout = manager.getMaxBusyTime();
			long timeRemaining = timeout;

			while (numConns > 0 && timeRemaining > 0) {
				int n = selector.select(timeRemaining);
				if (n > 0) {
					Iterator<SelectionKey> its = selector.selectedKeys().iterator();
					while (its.hasNext()) {
						SelectionKey key = its.next();
						its.remove();
						handleKey(key);
					}
				} else {
					lastError = "multiGet, selector timed out waiting";
					log.error(lastError);
					break;
				}
				timeRemaining = timeout - (System.currentTimeMillis() - startTime);
			}
		} catch (IOException e) {
			log.error("multiGet, exception on " + e);
			e.printStackTrace();
		} finally {
			try {
				manager.selectorClose(selector);
			} catch (IOException e) {
				lastError = "multiGet, close selector exception :" + e;
				log.error(lastError);
				e.printStackTrace();
			}
			Iterator<Connection> itc = conns.values().iterator();
			while (itc.hasNext()) {
				Connection conn = itc.next();
				conn.close();
			}
		}
		return result;
	}

	private void handleKey(SelectionKey key) throws IOException {
		if (key.isValid()) {
			if (key.isReadable()) {
				readResponse(key);
			} else if (key.isWritable()) {
				writeRequest(key);
			}
		}
	}

	private void writeRequest(SelectionKey key) throws IOException {
		ByteBuffer buf = ((Connection) key.attachment()).getOutBuffer();
		if (buf.hasRemaining()) {
			SocketChannel sc = (SocketChannel) key.channel();
			sc.write(buf);
		} else {
			key.interestOps(SelectionKey.OP_READ);
		}
	}

	private void readResponse(SelectionKey key) throws IOException {
		Connection conn = (Connection) key.attachment();
		if (conn.processResponse()) {
			key.cancel();
			numConns--;
		}
	}

	private final class Connection {
		private ByteBuffer outBuffer;
		private XixiSocket socket;
		private SocketChannel channel;
		private boolean isDone = false;
		private ArrayList<String> keys = new ArrayList<String>();
		private ArrayList<byte[]> keyBuffers = new ArrayList<byte[]>();
		private ArrayList<Integer> keyIndexs = new ArrayList<Integer>();
		private int currKeyIndex = 0;
		private ArrayList<CacheItem> result = null;
		
		public void add(String key, byte[] keyBuffer, Integer index) {
			keys.add(key);
			keyBuffers.add(keyBuffer);
			keyIndexs.add(index);
		}

		private void encode() {
			byte[] keyBuf = keyBuffers.get(currKeyIndex);
			if (outBuffer.limit() < keyBuf.length + 12) {
				outBuffer = ByteBuffer.allocateDirect(keyBuf.length + 12);
			}
			outBuffer.put(XIXI_CATEGORY_CACHE);
			outBuffer.put(XIXI_TYPE_GET_REQ);
			outBuffer.putInt(groupID);
			outBuffer.putInt(NO_WATCH);
			outBuffer.putShort((short) keyBuf.length);
			outBuffer.put(keyBuf);
			currKeyIndex++;
			while (currKeyIndex < keyBuffers.size()) {
				keyBuf = keyBuffers.get(currKeyIndex);
				if (outBuffer.limit() - outBuffer.position() < keyBuf.length + 12) {
					break;
				}
				outBuffer.put(XIXI_CATEGORY_CACHE);
				outBuffer.put(XIXI_TYPE_GET_REQ);
				outBuffer.putInt(groupID);
				outBuffer.putInt(NO_WATCH);
				outBuffer.putShort((short) keyBuf.length);
				outBuffer.put(keyBuf);
				currKeyIndex++;
			}
		}
		
		public void init(XixiSocket socket, ArrayList<CacheItem> result) throws IOException {
			this.socket = socket;
			this.result = result;
			outBuffer = socket.getWriteBuffer();//ByteBuffer.allocateDirect(64 * 1024);
			outBuffer.clear();
			
			encode();
			
			outBuffer.flip();
			channel = socket.getChannel();
			channel.configureBlocking(false);
			channel.register(selector, SelectionKey.OP_READ | SelectionKey.OP_WRITE, this);
		}
		
		public ByteBuffer getOutBuffer() throws IOException {
			int limit = outBuffer.limit();
			int pos = outBuffer.position();
			if (limit > pos) {
				return outBuffer;
			}
			if (currKeyIndex >= keyBuffers.size()) {
				channel.register(selector, SelectionKey.OP_READ, this);
				return outBuffer;
			}
			outBuffer.flip();
			
			encode();

			outBuffer.flip();
			return outBuffer;
		}

		public void close() {
			if (socket != null) {
				try {
					if (isDone) {
						channel.configureBlocking(true);
						socket.close();
						socket = null;
						return;
					}
				} catch (IOException e) {
					lastError = "close, exception on close, " + e;
					log.warn(lastError);
				}
	
				socket.trueClose();
				socket = null;
			}
		}

		private static final int STATE_READ_HEAD = 0;
		private static final int STATE_READ_FIXED_BODY = 1;
		private static final int STATE_READ_ERROR = 2;
		private static final int STATE_READ_DAYA = 3;
		private int state = STATE_READ_HEAD;
		private static final int HEADER_LENGTH = 2;
		private ByteBuffer header = ByteBuffer.allocate(HEADER_LENGTH);
		private static final int FIXED_LENGTH = 20;
		private ByteBuffer fixed = ByteBuffer.allocate(FIXED_LENGTH);
		private static final int ERROR_RES_LENGTH = 2;
		private ByteBuffer error_res = ByteBuffer.allocate(ERROR_RES_LENGTH);
		private long cacheID = 0;
		private int flags = 0;
		private int expiration = 0;
		private int dataSize = 0;
		private int offset = 0;
		private ByteBuffer data;
		private int processedCount = 0;
		public boolean processResponse() throws IOException {
			boolean run = true;
			while(run) {
				if (state == STATE_READ_HEAD) {
					channel.read(header);
					if (header.position() == HEADER_LENGTH) {
						header.flip();
						byte category = header.get();
						byte type = header.get();
						if (category == XIXI_CATEGORY_CACHE && type == XIXI_TYPE_GET_RES) {
							state = STATE_READ_FIXED_BODY;
							fixed = ByteBuffer.allocate(FIXED_LENGTH);
						} else {
							state = STATE_READ_ERROR;
							error_res = ByteBuffer.allocate(ERROR_RES_LENGTH);
						}
					} else {
						run = false;
					}
				}
				if (state == STATE_READ_FIXED_BODY) {
					channel.read(fixed);
					if (fixed.position() == FIXED_LENGTH) {
						fixed.flip();
						cacheID = fixed.getLong();
						flags = fixed.getInt();
						expiration = fixed.getInt();
						dataSize = fixed.getInt();
						
						data = ByteBuffer.allocate(dataSize);
						state = STATE_READ_DAYA;
						offset = STATE_READ_HEAD;
					} else {
						run = false;
					}
				}
				if (state == STATE_READ_ERROR) {
					channel.read(error_res);
					if (error_res.position() == ERROR_RES_LENGTH) {
						error_res.flip();
						short reason = error_res.getShort();
						result.set(keyIndexs.get(processedCount).intValue(), null);
						processedCount++;
						lastError = "processResponse, response error reason=" + reason; 
						log.warn(lastError);
						state = 0;
						if (keyBuffers.size() == processedCount) {
							isDone = true;
							run = false;
						} else {
							header = ByteBuffer.allocate(HEADER_LENGTH);
						}
					} else {
						run = false;
					}
				}
				if (state == STATE_READ_DAYA) {
					offset += channel.read(data);
					if (offset == dataSize) {
						String key = keys.get(processedCount);
						
						byte[] buf = data.array();
						try {
							Object obj = transCoder.decode(buf, flags, null);
							CacheItem item = new CacheItem(
									key,
									cacheID,
									expiration,
									groupID,
									flags,
									obj,
									dataSize);
							result.set(keyIndexs.get(processedCount).intValue(), item);
						} catch (IOException e) {
							log.error("processResponse, failed on transCoder.decode flags="
									+ flags + " bufLen=" + buf.length + " " + e);
						}
						processedCount++;
						if (keys.size() == processedCount) {
							isDone = true;
							run = false;
						}
						state = 0;
						data = null;
						header = ByteBuffer.allocate(HEADER_LENGTH);
					}
				} else {
					run = false;
				}
			}
			return isDone;
		}
	}
}
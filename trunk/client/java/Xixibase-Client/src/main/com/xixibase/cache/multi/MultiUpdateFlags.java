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
/*
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
import com.xixibase.cache.Defines;
import com.xixibase.cache.TransCoder;
import com.xixibase.cache.XixiSocket;
import com.xixibase.util.Log;
*/
import com.xixibase.cache.Defines;

public final class MultiUpdateFlags extends Defines {
/*	private static Log log = Log.getLog(MultiUpdateFlags.class.getName());

	private CacheClientManager manager;
	private int groupID;
	private TransCoder transCoder;
	
	private Selector selector;
	private int numConns = 0;
	private byte opFlag = 0;
	private int successCount = 0;
	private String lastError = null;
	
	public MultiUpdateFlags(CacheClientManager manager, int groupID, TransCoder transCoder) {
		this.manager = manager;
		this.groupID = groupID;
		this.transCoder = transCoder;
	}
	
	public String getLastError() {
		return lastError;
	}

	public int multiUpdateFlags(List<MultiUpdateFlagsItem> list, byte subOp) {
		lastError = null;
		opFlag = (byte)(subOp | XIXI_UPDATE_FLAGS_REPLY);
		if (list == null) {
			lastError = "multiUpdateFlags, list == null";
			log.error(lastError);
			return 0;
		}
		if (list.size() == 0) {
			lastError = "multiUpdateFlags, list.size() == 0";
			log.error(lastError);
			return 0;
		}
		Map<String, Connection> conns = new HashMap<String, Connection>();
		try {
			Iterator<MultiUpdateFlagsItem> it = list.iterator();
			int index = 0;
			while (it.hasNext()) {
				MultiUpdateFlagsItem item = it.next();
				if (item == null) {
					lastError = "multiUpdateFlags, item == null";
					log.error(lastError);
					return 0;
				}

				if (item.key == null) {
					lastError = "multiUpdateFlags, item.key == null";
					log.error(lastError);
					return 0;
				}

				String host = manager.getHost(item.key);
				if (host == null) {
					lastError = "multiUpdateFlags, can not get host with the key";
					log.error(lastError);
					return 0;
				}

				Connection conn = conns.get(host);
				if (conn == null) {
					conn = new Connection();
					conns.put(host, conn);
				}
				conn.add(item, new Integer(index));
				index++;
			}

			selector = Selector.open();

			Iterator<Entry<String, Connection>> itc = conns.entrySet().iterator();
			while (itc.hasNext()) {
				Entry<String, Connection> e = itc.next();
				String host = e.getKey();
				Connection conn = e.getValue();
				XixiSocket socket = manager.getSocketByHost(host);

				if (socket != null) {
					conn.init(socket);
				}
			}

			numConns = conns.size();

			long startTime = System.currentTimeMillis();
			long timeout = manager.getMaxBusyTime();
			long timeRemaining = timeout;

			while (numConns > 0 && timeRemaining > 0) {
				int n = selector.select(Math.min(timeout, 5000));
				if (n > 0) {
					Iterator<SelectionKey> its = selector.selectedKeys().iterator();
					while (its.hasNext()) {
						SelectionKey key = its.next();
						its.remove();
						handleKey(key);
					}
				} else {
					lastError = "multiUpdateFlags, selector timed out";
					log.error(lastError);
					break;
				}

				timeRemaining = timeout - (System.currentTimeMillis() - startTime);
			}
		} catch (IOException e) {
			lastError = "multiUpdateFlags, exception on " + e;
			log.error(lastError);
			return 0;
		} finally {
			try {
				if (selector != null) {
					selector.close();
				}
			} catch (IOException ignoreMe) {
			}
			Iterator<Connection> itc = conns.values().iterator();
			while (itc.hasNext()) {
				Connection conn = itc.next();
				conn.close();
			}
		}

		return successCount;
	}

	private void handleKey(SelectionKey key) throws IOException {
		if (key.isReadable()) {
			readResponse(key);
		} else if (key.isWritable()) {
			writeRequest(key);
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
		private ArrayList<MultiUpdateFlagsItem> items = new ArrayList<MultiUpdateFlagsItem>();
		private ArrayList<Integer> itemIndexs = new ArrayList<Integer>();
		private int currKeyIndex = 0;

		public void add(MultiUpdateFlagsItem item, Integer index) {
			items.add(item);
			itemIndexs.add(index);
		}

		MultiUpdateFlagsItem item = null;
		byte[] keyBuf = null;
		private void encode() throws IOException {
			item = items.get(currKeyIndex);
			keyBuf = transCoder.encodeKey(item.key);
			if (keyBuf == null) {
				lastError = "encode, failed to encode key";
				log.error(lastError);
				return;
			}

			int totalLen = 21 + keyBuf.length;
			if (outBuffer.limit() < totalLen) {
				outBuffer = ByteBuffer.allocateDirect(totalLen);
			}
			outBuffer.put(XIXI_CATEGORY_CACHE);
			outBuffer.put(XIXI_TYPE_UPDATE_FLAGS_REQ);
			outBuffer.put(opFlag);
			outBuffer.putLong(item.cacheID);//uint64_t cacheID;
			outBuffer.putInt(groupID);
			outBuffer.putInt(item.flags); // flags
			outBuffer.putShort((short) keyBuf.length); // uint16_t key_length;
			outBuffer.put(keyBuf);

			currKeyIndex++;
			
			while (currKeyIndex < items.size()) {
				item = items.get(currKeyIndex);
				keyBuf = transCoder.encodeKey(item.key);
				if (keyBuf == null) {
					lastError = "encode, failed to encode key";
					log.error(lastError);
					return;
				}
				

				totalLen = 21 + keyBuf.length;
				if (outBuffer.limit() - outBuffer.position() < totalLen) {
					break;
				}
				outBuffer.put(XIXI_CATEGORY_CACHE);
				outBuffer.put(XIXI_TYPE_UPDATE_FLAGS_REQ);
				outBuffer.put(opFlag);
				outBuffer.putLong(item.cacheID);//uint64_t cacheID;
				outBuffer.putInt(groupID);
				outBuffer.putInt(item.flags); // flags
				outBuffer.putShort((short) keyBuf.length); // uint16_t key_length;
				outBuffer.put(keyBuf);

				currKeyIndex++;
			}
		}
		
		public void init(XixiSocket socket) throws IOException {
			this.socket = socket;
			outBuffer = ByteBuffer.allocateDirect(64 * 1024);
			
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
			if (currKeyIndex >= items.size()) {
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
					lastError = "close, failed on close socket, " + e;
					log.warn(lastError);
				}
	
				socket.trueClose();
				socket = null;
			}
		}

		private static final int STATE_READ_HEAD = 0;
		private static final int STATE_READ_FIXED_BODY = 1;
		private static final int STATE_READ_ERROR = 2;
		private int state = STATE_READ_HEAD;
		private static final int HEADER_LENGTH = 2;
		private ByteBuffer header = ByteBuffer.allocate(HEADER_LENGTH);
		private static final int FIXED_LENGTH = 8;
		private ByteBuffer fixed = ByteBuffer.allocate(FIXED_LENGTH);
		private static final int ERROR_RES_LENGTH = 2;
		private ByteBuffer error_res = ByteBuffer.allocate(ERROR_RES_LENGTH);
		private long cacheID = 0;
		private int decode_count = 0;
		public boolean processResponse() throws IOException {
			boolean run = true;
			while(run) {
				if (state == STATE_READ_HEAD) {
					channel.read(header);
					if (header.position() == HEADER_LENGTH) {
						header.flip();
						byte category = header.get();
						byte type = header.get();
						if (category == XIXI_CATEGORY_CACHE && type == XIXI_TYPE_UPDATE_FLAGS_RES) {
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
						items.get(decode_count).newCacheID = cacheID;
						decode_count++;
						successCount++;
						
						if (items.size() == decode_count) {
							isDone = true;
							run = false;
						} else {
							header = ByteBuffer.allocate(HEADER_LENGTH);
							state = STATE_READ_HEAD;
						}
					} else {
						run = false;
					}
				}
				if (state == STATE_READ_ERROR) {
					channel.read(error_res);
					if (error_res.position() == ERROR_RES_LENGTH) {
						error_res.flip();
						short reason = error_res.getShort();

						items.get(decode_count).reason = reason;
						decode_count++;

						lastError = "processResponse, resonpse error reason=" + reason;
						log.error(lastError);
						if (items.size() == decode_count) {
							isDone = true;
							run = false;
						} else {
							header = ByteBuffer.allocate(HEADER_LENGTH);
							state = STATE_READ_HEAD;
						}
					} else {
						run = false;
					}
				}
			}
			return isDone;
		}
	}*/
}

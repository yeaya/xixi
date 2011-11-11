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

package com.xixibase.cache;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import com.xixibase.cache.multi.MultiDelete;
import com.xixibase.cache.multi.MultiDeleteItem;
import com.xixibase.cache.multi.MultiGet;
import com.xixibase.cache.multi.MultiUpdate;
import com.xixibase.cache.multi.MultiUpdateExpiration;
import com.xixibase.cache.multi.MultiUpdateExpirationItem;
import com.xixibase.cache.multi.MultiUpdateItem;
import com.xixibase.util.Log;

public class CacheClientImpl extends Defines {
	private static Log log = Log.getLog(CacheClientImpl.class.getName());
	
	private int groupID = 0;
	private TransCoder transCoder = new ObjectTransCoder();
	private CacheClientManager manager;
	private LocalCache localCache = null;
	private String lastError;

	public String getLastError() {
		return lastError;
	}

	public CacheClientImpl(CacheClientManager manager, int groupID) {
		this.manager = manager;
		this.localCache = manager.getLocalCache();
		this.groupID = groupID;
	}
/*
	public CacheClientImpl(String managerName, int groupID) {
		this(CacheClientManager.getInstance(managerName), groupID);
	}
*/
	public int getGroupID() {
		return groupID;
	}

//	public void setGroupID(int groupID) {
//		this.groupID = groupID;
//	}

	public void setTransCoder(TransCoder transCoder) {
		this.transCoder = transCoder;
	}

	public TransCoder getTransCoder() {
		return transCoder;
	}

	public CacheItem get(String key, int getFlag, int expiration) {
		lastError = null;
		if (key == null) {
			lastError = "get, key == null";
			log.error(lastError);
			return null;
		}

		byte[] keyBuf = transCoder.encodeKey(key);
		if (keyBuf == null) {
			lastError = "get, failed to encode key";
			log.error(lastError);
			return null;
		}

		String host = manager.getHost(key);
// manager.getHost never return null
//		if (host == null) {
//			lastError = "get, failed to get host";
//			log.error(lastError);
//			return null;
//		}

		CacheItem item = null;
		int watchID = 0;
		if ((getFlag & LOCAL_CACHE) == LOCAL_CACHE) {
			if ((getFlag & TOUCH_CACHE) == TOUCH_CACHE) {
				item = localCache.getAndTouch(host, groupID, key, expiration);
			} else {
				item = localCache.get(host, groupID, key);
			}
			if (item != null) {
				return item;
			}
		}
		if ((getFlag & WATCH_CACHE) == WATCH_CACHE) {
			watchID = localCache.getWatchID(host);
		}
		
		XixiSocket socket = manager.getSocket(key);
		if (socket == null) {
			lastError = "get, failed to get socket";
			log.error(lastError);
			return null;
		}

		try {
			ByteBuffer writeBuffer = socket.getWriteBuffer();
			writeBuffer.clear();
			writeBuffer.put(XIXI_CATEGORY_CACHE);
			if ((getFlag & TOUCH_CACHE) == TOUCH_CACHE) {
				writeBuffer.put(XIXI_TYPE_GET_TOUCH_REQ);
				writeBuffer.putInt(groupID); // groupID
				writeBuffer.putInt(watchID); // watchID
				writeBuffer.putInt(expiration);
			} else {
				writeBuffer.put(XIXI_TYPE_GET_REQ);
				writeBuffer.putInt(groupID); // groupID
				writeBuffer.putInt(watchID); // watchID
			}
			writeBuffer.putShort((short) keyBuf.length);
			writeBuffer.put(keyBuf);
			socket.flush();

			byte category = socket.readByte();
			byte type = socket.readByte();
			if (category == XIXI_CATEGORY_CACHE && type == XIXI_TYPE_GET_RES) {
				long cacheID = socket.readLong();//uint64_t cacheID;
				int flags = socket.readInt();
				expiration = socket.readInt();
				int dataSize = socket.readInt();//uint32_t data_length;
				if (dataSize >= 0) {
					byte[] data = socket.read(dataSize);
					int[] objectSize = new int[1];
					Object obj = transCoder.decode(data, flags, objectSize);
					item = new CacheItem(
							key,
							cacheID,
							expiration,
							groupID,
							flags,
							obj,
							objectSize[0]);
					if (watchID != 0) {
						localCache.put(socket.getHost(), key, item);
					}
					return item;
				}
			} else {
				short reason = socket.readShort();
				lastError = "get, response error, reason=" + reason;
				log.debug(lastError);
				if (reason == XIXI_REASON_UNKNOWN_COMMAND) {
					socket.trueClose();
					socket = null;
				}
			}
		} catch (IOException e) {
			lastError = "get, exception=" + e;
			log.error(lastError);
			socket.trueClose();
			socket = null;
		} finally {
			if (socket != null) {
				socket.close();
				socket = null;
			}
		}
		return null;
	}

	public CacheBaseItem getBase(String key) {
		lastError = null;
		if (key == null) {
			lastError = "getBase, key == null";
			log.error(lastError);
			return null;
		}
	
		byte[] keyBuf = transCoder.encodeKey(key);
		if (keyBuf == null) {
			lastError = "getBase, failed to encode key";
			log.error(lastError);
			return null;
		}

		XixiSocket socket = manager.getSocket(key);
		if (socket == null) {
			lastError = "getBase, failed to get socket";
			log.error(lastError);
			return null;
		}

		try {
			ByteBuffer writeBuffer = socket.getWriteBuffer();
			writeBuffer.clear();
			writeBuffer.put(XIXI_CATEGORY_CACHE);
			writeBuffer.put(XIXI_TYPE_GET_BASE_REQ);
			writeBuffer.putInt(groupID); // groupID
			writeBuffer.putShort((short) keyBuf.length);
			writeBuffer.put(keyBuf);
			socket.flush();

			byte category = socket.readByte();
			byte type = socket.readByte();
			if (category == XIXI_CATEGORY_CACHE && type == XIXI_TYPE_GET_BASE_RES) {
				long cacheID = socket.readLong();
				int flags = socket.readInt();
				int expiration = socket.readInt();
				CacheBaseItem item = new CacheBaseItem(
						key,
						cacheID,
						expiration,
						groupID,
						flags);
				return item;
			} else {
				short reason = socket.readShort();
				lastError = "getBase, response error, reason=" + reason;
				log.debug(lastError);
				if (reason == XIXI_REASON_UNKNOWN_COMMAND) {
					socket.trueClose();
					socket = null;
				}
			}
		} catch (IOException e) {
			lastError = "getBase, exception=" + e;
			log.error(lastError);
			socket.trueClose();
			socket = null;
		} finally {
			if (socket != null) {
				socket.close();
				socket = null;
			}
		}
		return null;
	}
/*
	public boolean updateFlags(String key, long cacheID, int groupID, int flags) {
		return updateFlags(XIXI_UPDATE_BASE_SUB_OP_FLAGS, key, cacheID, groupID, flags);
	}
	*/
//	public boolean updateExpiration(String key, long cacheID, int groupID, int expiration) {
//		return updateFlags((byte)2/*XIXI_UPDATE_BASE_SUB_OP_EXPIRATION*/, key, cacheID, groupID, 0, expiration);
//	}

	protected boolean updateFlags(String key, int flags, long cacheID) {
		lastError = null;
		if (key == null) {
			lastError = "updateFlags, key == null";
			log.error(lastError);
			return false;
		}
	
		byte[] keyBuf = transCoder.encodeKey(key);
		if (keyBuf == null) {
			lastError = "updateFlags, failed to encode key";
			log.error(lastError);
			return false;
		}

		XixiSocket socket = manager.getSocket(key);
		if (socket == null) {
			lastError = "updateFlags, failed to get socket";
			log.error(lastError);
			return false;
		}

		try {
			byte subOp = XIXI_UPDATE_FLAGS_REPLY;
			ByteBuffer writeBuffer = socket.getWriteBuffer();
			writeBuffer.clear();
			writeBuffer.put(XIXI_CATEGORY_CACHE);
			writeBuffer.put(XIXI_TYPE_UPDATE_FLAGS_REQ);
			writeBuffer.put(subOp);
			writeBuffer.putLong(cacheID);
			writeBuffer.putInt(groupID);
			writeBuffer.putInt(flags);
			writeBuffer.putShort((short) keyBuf.length);
			writeBuffer.put(keyBuf);
			socket.flush();

			byte category = socket.readByte();
			byte type = socket.readByte();
			if (category == XIXI_CATEGORY_CACHE && type == XIXI_TYPE_UPDATE_FLAGS_RES) {
				socket.readLong(); // rescacheID
				localCache.remove(socket.getHost(), groupID, key);
				return true;
			} else {
				short reason = socket.readShort();
				lastError = "updateFlags, response error, reason=" + reason;
				log.debug(lastError);
				if (reason == XIXI_REASON_UNKNOWN_COMMAND) {
					socket.trueClose();
					socket = null;
				}
			}
		} catch (IOException e) {
			lastError = "updateFlags, exception=" + e;
			log.error(lastError);
			socket.trueClose();
			socket = null;
		} finally {
			if (socket != null) {
				socket.close();
				socket = null;
			}
		}
		return false;
	}
	
	public boolean updateExpiration(String key, int expiration, long cacheID) {
		lastError = null;
		if (key == null) {
			lastError = "updateExpiration, key == null";
			log.error(lastError);
			return false;
		}
	
		byte[] keyBuf = transCoder.encodeKey(key);
		if (keyBuf == null) {
			lastError = "updateExpiration, failed to encode key";
			log.error(lastError);
			return false;
		}

		XixiSocket socket = manager.getSocket(key);
		if (socket == null) {
			lastError = "updateExpiration, failed to get socket";
			log.error(lastError);
			return false;
		}

		try {
			byte subOp = XIXI_UPDATE_EXPIRATION_REPLY;
			ByteBuffer writeBuffer = socket.getWriteBuffer();
			writeBuffer.clear();
			writeBuffer.put(XIXI_CATEGORY_CACHE);
			writeBuffer.put(XIXI_TYPE_UPDATE_EXPIRATION_REQ);
			writeBuffer.put(subOp);
			writeBuffer.putLong(cacheID);
			writeBuffer.putInt(groupID);
			writeBuffer.putInt(expiration);
			writeBuffer.putShort((short) keyBuf.length);
			writeBuffer.put(keyBuf);
			socket.flush();

			byte category = socket.readByte();
			byte type = socket.readByte();
			if (category == XIXI_CATEGORY_CACHE && type == XIXI_TYPE_UPDATE_EXPIRATION_RES) {
				socket.readLong(); // cacheID
				localCache.remove(socket.getHost(), groupID, key);
				return true;
			} else {
				short reason = socket.readShort();
				lastError = "updateExpiration, response error, reason=" + reason;
				log.debug(lastError);
				if (reason == XIXI_REASON_UNKNOWN_COMMAND) {
					socket.trueClose();
					socket = null;
				}
			}
		} catch (IOException e) {
			lastError = "updateExpiration, exception=" + e;
			log.error(lastError);
			socket.trueClose();
			socket = null;
		} finally {
			if (socket != null) {
				socket.close();
				socket = null;
			}
		}
		return false;
	}
	
	public long add(String key, Object value, int expiration, boolean watchFlag) {
		return update(XIXI_UPDATE_SUB_OP_ADD, key, value, expiration, NO_CAS, watchFlag);
	}

	public long append(String key, Object value, long cacheID) {
		return update(XIXI_UPDATE_SUB_OP_APPEND, key, value, NO_EXPIRATION, cacheID, false);
	}

	public long set(String key, Object value, int expiration, long cacheID, boolean watchFlag) {
		return update(XIXI_UPDATE_SUB_OP_SET, key, value, expiration, cacheID, watchFlag);
	}

	public long prepend(String key, Object value, long cacheID) {
		return update(XIXI_UPDATE_SUB_OP_PREPEND, key, value, NO_EXPIRATION, cacheID, false);
	}

	public long replace(String key, Object value, int expiration, long cacheID, boolean watchFlag) {
		return update(XIXI_UPDATE_SUB_OP_REPLACE, key, value, expiration, cacheID, watchFlag);
	}

	private long update(byte subOp, String key, Object value, int expiration, long cacheID, boolean watchFlag) {
		lastError = null;
		if (key == null) {
			lastError = "update, key == null";
			log.error(lastError);
			return NO_CAS;
		}

		byte[] keyBuf = transCoder.encodeKey(key);
		if (keyBuf == null) {
			lastError = "update, failed to encode key";
			log.error(lastError);
			return NO_CAS;
		}

		if (value == null) {
			lastError = "update, value == null";
			log.error(lastError);
			return NO_CAS;
		}

		XixiSocket socket = manager.getSocket(key);
		if (socket == null) {
			lastError = "update, failed to get socket";
			log.error(lastError);
			return NO_CAS;
		}

		try {
			byte op_flag = (byte)(subOp | XIXI_UPDATE_REPLY);
			int watchID = 0;
			if (watchFlag) {
				watchID = localCache.getWatchID(socket.getHost());
			}

			int[] outflags = new int[1];
			byte[] data = transCoder.encode(value, outflags);
			int flags = outflags[0];
			int dataSize = data.length;//output.getSize();

			ByteBuffer writeBuffer = socket.getWriteBuffer();
			writeBuffer.clear();
			writeBuffer.put(XIXI_CATEGORY_CACHE);
			writeBuffer.put(XIXI_TYPE_UPDATE_REQ);
			writeBuffer.put(op_flag);
			writeBuffer.putLong(cacheID);//uint64_t cacheID;
			writeBuffer.putInt(groupID);
			writeBuffer.putInt(flags); // flags
			writeBuffer.putInt(expiration);//			uint32_t expiration;
			writeBuffer.putInt(watchID);
			writeBuffer.putShort((short) keyBuf.length); // uint16_t key_length;

			writeBuffer.putInt(dataSize); // uint32_t data_length;
			writeBuffer.put(keyBuf);

			socket.write(data, 0, dataSize);
			socket.flush();

			byte category = socket.readByte();
			byte type = socket.readByte();
			if (category == XIXI_CATEGORY_CACHE && type == XIXI_TYPE_UPDATE_RES) {
				long newCacheID = socket.readLong();
				localCache.remove(socket.getHost(), groupID, key);
				if (watchID != 0) {
					CacheItem item = new CacheItem(
							key,
							newCacheID,
							expiration,
							groupID,
							flags,
							value,
							dataSize);
					localCache.put(socket.getHost(), key, item);
				}
				return newCacheID;
			} else {
				short reason = socket.readShort();
				lastError = "update, response error, reason=" + reason;
				log.debug(lastError);
				if (reason == XIXI_REASON_UNKNOWN_COMMAND) {
					socket.trueClose();
					socket = null;
				}
			}
		} catch (IOException e) {
			lastError = "update, exception=" + e;
			log.error(lastError);
			socket.trueClose();
			socket = null;
		} finally {
			if (socket != null) {
				socket.close();
				socket = null;
			}
		}

		return NO_CAS;
	}

	public boolean delete(String key, long cacheID) {
		lastError = null;
		if (key == null) {
			lastError = "delete, key == null"; 
			log.error(lastError);
			return false;
		}

		byte[] keyBuf = transCoder.encodeKey(key);
		if (keyBuf == null) {
			lastError = "delete, failed to encode key!";
			log.error(lastError);
			return false;
		}

		XixiSocket socket = manager.getSocket(key);
		if (socket == null) {
			lastError = "delete, failed to get socket";
			log.error(lastError);
			return false;
		}

		try {
			byte op_flag = (byte)(XIXI_DELETE_SUB_OP | XIXI_DELETE_REPLY);
			ByteBuffer writeBuffer = socket.getWriteBuffer();
			writeBuffer.clear();
			writeBuffer.put(XIXI_CATEGORY_CACHE);
			writeBuffer.put(XIXI_TYPE_DELETE_REQ);
			writeBuffer.put(op_flag);
			writeBuffer.putLong(cacheID); // cacheID
			writeBuffer.putInt(groupID); // groupID
			writeBuffer.putShort((short) keyBuf.length);
			writeBuffer.put(keyBuf);
			socket.flush();
			
			byte category = socket.readByte();
			byte type = socket.readByte();
			if (category == XIXI_CATEGORY_CACHE && type == XIXI_TYPE_DELETE_RES) {
				localCache.remove(socket.getHost(), groupID, key);
				return true;
			} else {
				short reason = ObjectTransCoder.decodeShort(socket.read(2));
				lastError = "delete, response error, reason=" + reason;
				log.debug(lastError);
				if (reason == XIXI_REASON_UNKNOWN_COMMAND) {
					socket.trueClose();
					socket = null;
				}
			}
		} catch (IOException e) {
			lastError = "delete, exception=" + e;
			log.error(lastError);
			socket.trueClose();
			socket = null;
		} finally {
			if (socket != null) {
				socket.close();
				socket = null;
			}
		}

		return false;
	}

	public DeltaItem incr(String key, long delta, long cacheID) {
		return delta(key, XIXI_DELTA_SUB_OP_INCR, delta, cacheID);
	}

	public DeltaItem decr(String key, long delta, long cacheID) {
		return delta(key, XIXI_DELTA_SUB_OP_DECR, delta, cacheID);
	}

	private DeltaItem delta(String key, byte subOp, long delta, long cacheID) {
		lastError = null;
		if (key == null) {
			lastError = "delta, key == null";
			log.error(lastError);
			return null;
		}

		byte[] keyBuf = transCoder.encodeKey(key);
		if (keyBuf == null) {
			lastError = "delta, failed to encode key";
			log.error(lastError);
			return null;
		}

		XixiSocket socket = manager.getSocket(key);
		if (socket == null) {
			lastError = "delta, failed to get socket";
			log.error(lastError);
			return null;
		}

		try {
			byte op_flag = (byte)(subOp | XIXI_DELTA_REPLY);
			ByteBuffer writeBuffer = socket.getWriteBuffer();
			writeBuffer.clear();
			writeBuffer.put(XIXI_CATEGORY_CACHE);
			writeBuffer.put(XIXI_TYPE_DETLA_REQ);
			writeBuffer.put(op_flag);
			writeBuffer.putLong(cacheID); // cacheID
			writeBuffer.putInt(groupID); // groupID
			writeBuffer.putLong(delta); // delta
			writeBuffer.putShort((short) keyBuf.length);// key size
			writeBuffer.put(keyBuf);
			socket.flush();

			byte category = socket.readByte();
			byte type = socket.readByte();
			if (category == XIXI_CATEGORY_CACHE && type == XIXI_TYPE_DETLA_RES) {
				cacheID = socket.readLong();//uint64_t ;
				long value = socket.readLong();
				localCache.remove(socket.getHost(), groupID, key);
				DeltaItem item = new DeltaItem();
				item.cacheID = cacheID;
				item.value = value;
				return item;
			} else {
				short reason = socket.readShort();
				lastError = "delta, response error, reason=" + reason;
				log.debug(lastError);
				if (reason == XIXI_REASON_UNKNOWN_COMMAND) {
					socket.trueClose();
					socket = null;
				}
			}
		} catch (IOException e) {
			lastError = "delta, exception=" + e;
			log.error(lastError);
			socket.trueClose();
			socket = null;
		} finally {
			if (socket != null) {
				socket.close();
				socket = null;
			}
		}
		return null;
	}

	public List<CacheItem> multiGet(List<String> keys) {
		MultiGet multi = new MultiGet(this.manager, this.groupID, this.transCoder);
		List<CacheItem> list = multi.multiGet(keys);
		lastError = multi.getLastError();
		return list;
	}

	public int multiSet(List<MultiUpdateItem> items) {
		MultiUpdate multi = new MultiUpdate(this.manager, this.groupID, this.transCoder);
		int ret = multi.multiUpdate(items, XIXI_UPDATE_SUB_OP_SET);
		lastError = multi.getLastError();
		return ret;
	}
	
	public int multiAdd(List<MultiUpdateItem> items) {
		MultiUpdate multi = new MultiUpdate(this.manager, this.groupID, this.transCoder);
		int ret = multi.multiUpdate(items, XIXI_UPDATE_SUB_OP_ADD);
		lastError = multi.getLastError();
		return ret;
	}
	
	public int multiReplace(List<MultiUpdateItem> items) {
		MultiUpdate multi = new MultiUpdate(this.manager, this.groupID, this.transCoder);
		int ret = multi.multiUpdate(items, XIXI_UPDATE_SUB_OP_REPLACE);
		lastError = multi.getLastError();
		return ret;
	}
	
	public int multiAppend(List<MultiUpdateItem> items) {
		MultiUpdate multi = new MultiUpdate(this.manager, this.groupID, this.transCoder);
		int ret = multi.multiUpdate(items, XIXI_UPDATE_SUB_OP_APPEND);
		lastError = multi.getLastError();
		return ret;
	}
	
	public int multiPrepend(List<MultiUpdateItem> items) {
		MultiUpdate multi = new MultiUpdate(this.manager, this.groupID, this.transCoder);
		int ret = multi.multiUpdate(items, XIXI_UPDATE_SUB_OP_PREPEND);
		lastError = multi.getLastError();
		return ret;
	}
	
	public int multiDelete(List<MultiDeleteItem> items) {
		MultiDelete multi = new MultiDelete(this.manager, this.groupID, this.transCoder);
		int ret = multi.multiDelete(items);
		lastError = multi.getLastError();
		return ret;
	}
/*
	public int multiUpdateFlags(List<MultiUpdateFlagsItem> items) {
		MultiupdateFlags multi = new MultiupdateFlags(this.manager, this.groupID, this.transCoder);
		int ret = multi.multiupdateFlags(items, XIXI_UPDATE_BASE_SUB_OP_FLAGS);
		lastError = multi.getLastError();
		return ret;
	}
*/
	public int multiUpdateExpiration(List<MultiUpdateExpirationItem> items) {
		MultiUpdateExpiration multi = new MultiUpdateExpiration(this.manager, this.groupID, this.transCoder);
		int ret = multi.multiUpdateExpiration(items);
		lastError = multi.getLastError();
		return ret;
	}

	public int flush() {
		return flush(null);
	}

	public int flush(String[] servers) {
		lastError = null;
		int count = 0;
		servers = (servers == null) ? manager.getServers() : servers;

		if (servers == null || servers.length <= 0) {
			lastError = "flush, no servers to flush";
			log.error(lastError);
			return count;
		}

		for (int i = 0; i < servers.length; i++) {
			XixiSocket socket = manager.getSocketByHost(servers[i]);
			if (socket == null) {
				lastError = "flush, can not to get socket by host:" + servers[i];
				log.error(lastError);
				continue;
			}
			try {
				ByteBuffer writeBuffer = socket.getWriteBuffer();
				writeBuffer.clear();
				writeBuffer.put(XIXI_CATEGORY_CACHE);
				writeBuffer.put(XIXI_TYPE_FLUSH_REQ);
				writeBuffer.putInt(groupID);
			
				socket.flush();

				byte category = socket.readByte();
				byte type = socket.readByte(); // type
				if (category == XIXI_CATEGORY_CACHE && type == XIXI_TYPE_FLUSH_RES) {
					int flushCount = socket.readInt(); // int flush_count = 
					socket.readLong(); // long flush_size =
					localCache.flush(socket.getHost(), groupID);
					count += flushCount;
				} else {
					short reason = socket.readShort();
					lastError = "flush, response error, reason=" + reason;
					log.debug(lastError);
					if (reason == XIXI_REASON_UNKNOWN_COMMAND) {
						socket.trueClose();
						socket = null;
					}
				}
			} catch (IOException e) {
				lastError = "exception, " + e;
				log.error(lastError);
				socket.trueClose();
				socket = null;
			} finally {
				if (socket != null) {
					socket.close();
					socket = null;
				}
			}
		}

		return count;
	}

	protected boolean statsAddGroup(String[] servers, int groupID) {
		return stats(XIXI_STATS_SUB_OP_ADD_GROUP,  servers, groupID, (byte)0, null);
	}
	
	protected boolean statsRemoveGroup(String[] servers, int groupID) {
		return stats(XIXI_STATS_SUB_OP_REMOVE_GROUP,  servers, groupID, (byte)0, null);
	}
	
	protected boolean statsGetStats(String[] servers, byte class_id, Map<String, Map<String, String>> result) {
		return stats(XIXI_STATS_SUB_OP_GET_STATS_SUM_ONLY,  servers, 0, class_id, result);
	}
/*
	protected boolean statsGetAndClearStats(String[] servers, byte class_id, Map<String, Map<String, String>> result) {
		return stats(XIXI_STATS_SUB_OP_GET_AND_CLEAR_STATS_SUM_ONLY,  servers, 0, class_id, result);
	}
*/
	protected boolean statsGetGroupStats(String[] servers, int groupID, byte class_id, Map<String, Map<String, String>> result) {
		return stats(XIXI_STATS_SUB_OP_GET_STATS_GROUP_ONLY,  servers, groupID, class_id, result);
	}
/*
	protected boolean statsGetAndClearGroupStats(String[] servers, int groupID, byte class_id, Map<String, Map<String, String>> result) {
		return stats(XIXI_STATS_SUB_OP_GET_AND_CLEAR_STATS_GROUP_ONLY,  servers, groupID, class_id, result);
	}
*/
	protected boolean stats(byte op_flag, String[] servers, int groupID, byte class_id, Map<String, Map<String, String>> result) {
		lastError = null;

		servers = (servers == null) ? manager.getServers() : servers;

		if (servers == null || servers.length <= 0) {
			lastError = "stats, no servers to flush";
			log.error(lastError);
			return false;
		}

		boolean ret = true;

		for (int i = 0; i < servers.length; i++) {
			HashMap<String, String> hm = new HashMap<String, String>();
			if (result != null) {
				result.put(servers[i], hm);
			}
			XixiSocket socket = manager.getSocketByHost(servers[i]);
			if (socket == null) {
				lastError = "stats, can not to get socket by host:" + servers[i];
				log.error(lastError);
				ret = false;
				continue;
			}

			ByteBuffer writeBuffer = socket.getWriteBuffer();
			writeBuffer.clear();
			writeBuffer.put(XIXI_CATEGORY_CACHE);
			writeBuffer.put(XIXI_TYPE_STATS_REQ);
			writeBuffer.put(op_flag);
			writeBuffer.put(class_id);
			writeBuffer.putInt(groupID);
			try {
				socket.flush();

				byte category = socket.readByte();
				byte type = socket.readByte(); // type
				if (category == XIXI_CATEGORY_CACHE && type == XIXI_TYPE_STATS_RES) {
					int size = socket.readInt();
					byte[] buf = socket.read(size);
					String str = new String(buf);
					if (op_flag == XIXI_STATS_SUB_OP_GET_STATS_GROUP_ONLY
						//	|| op_flag == XIXI_STATS_SUB_OP_GET_AND_CLEAR_STATS_GROUP_ONLY
							|| op_flag == XIXI_STATS_SUB_OP_GET_STATS_SUM_ONLY
						/*	|| op_flag == XIXI_STATS_SUB_OP_GET_AND_CLEAR_STATS_SUM_ONLY*/) {
						String[] lines = str.split("\n");
						for (int j = 0; j < lines.length; j++) {
							String[] s = lines[j].split("=");
							hm.put(s[0], s[1]);
						}
					} else {
						if (!str.equals("success")) {
							ret = false;
						}
					}
				} else {
					short reason = socket.readShort();
					lastError = "stats, response error, reason=" + reason;
					log.debug(lastError);
					ret = false;
					if (reason == XIXI_REASON_UNKNOWN_COMMAND) {
						socket.trueClose();
						socket = null;
					}
				}
			} catch (IOException e) {
				lastError = "exception, " + e;
				log.error(lastError);
				ret = false;
				socket.trueClose();
				socket = null;
			} finally {
				if (socket != null) {
					socket.close();
					socket = null;
				}
			}
		}

		return ret;
	}

	protected int createWatch(String host, int maxNextCheckInterval) {
		lastError = null;
	
		XixiSocket socket = manager.getSocketByHost(host);
		if (socket == null) {
			lastError = "createWatch, failed on get socket by host";
			log.error(lastError);
			return 0;
		}

		try {
			ByteBuffer writeBuffer = socket.getWriteBuffer();
			writeBuffer.clear();
			writeBuffer.put(XIXI_CATEGORY_CACHE);
			writeBuffer.put(XIXI_CREATE_WATCH_REQ);
			writeBuffer.putInt(groupID);
			writeBuffer.putInt(maxNextCheckInterval);
			socket.flush();
		//	log.debug("localCache createWatch " + host + " watchID2=" + watchID);

			byte category = socket.readByte();
			byte type = socket.readByte();
			if (category == XIXI_CATEGORY_CACHE && type == XIXI_CREATE_WATCH_RES) {
				int watchID = socket.readInt();
				return watchID;
			} else {
				short reason = socket.readShort();
				lastError = "createWatch, response error, reason=" + reason;
				log.debug(lastError);
				if (reason == XIXI_REASON_UNKNOWN_COMMAND) {
					socket.trueClose();
					socket = null;
				}
			}
		} catch (IOException e) {
			lastError = "createWatch, exception=" + e;
			log.error(lastError);
			socket.trueClose();
			socket = null;
		} finally {
			if (socket != null) {
				socket.close();
				socket = null;
			}
		}
		return 0;
	}
	
	protected long[] checkWatch(String host, int watchID, int checkTimeout, int maxNextCheckInterval, long ackCacheID) {
		lastError = null;
		XixiSocket socket = manager.getSocketByHost(host);
		if (socket == null) {
			lastError = "checkWatch, failed to get by host:" + host;
			log.error(lastError);
			return null;
		}
		try {
			ByteBuffer writeBuffer = socket.getWriteBuffer();
			writeBuffer.clear();
			writeBuffer.put(XIXI_CATEGORY_CACHE);
			writeBuffer.put(XIXI_CHECK_WATCH_REQ);
			writeBuffer.putInt(groupID);
			writeBuffer.putInt(watchID);
			writeBuffer.putInt(checkTimeout);
			writeBuffer.putInt(maxNextCheckInterval);
			writeBuffer.putLong(ackCacheID);
			socket.flush();

			byte category = socket.readByte();
			byte type = socket.readByte();
			if (category == XIXI_CATEGORY_CACHE && type == XIXI_CHECK_WATCH_RES) {
				int updateCount = socket.readInt();
				long[] updates = new long[updateCount];
				for (int i = 0; i < updateCount; i++) {
					updates[i] = socket.readLong();
				}
				return updates;
			} else {
				short reason = socket.readShort();
				lastError = "checkWatch, response error, reason=" + reason;
				log.debug(lastError);
				if (reason == XIXI_REASON_UNKNOWN_COMMAND) {
					socket.trueClose();
					socket = null;
				}
			}
		} catch (IOException e) {
			lastError = "checkWatch, e=" + e;
			log.error(lastError);
			socket.trueClose();
			socket = null;
		} finally {
			if (socket != null) {
				socket.close();
				socket = null;
			}
		}
		return null;
	}
}

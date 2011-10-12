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

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.xixibase.cache.multi.MultiDeleteItem;
import com.xixibase.cache.multi.MultiUpdateItem;

public class CacheClient extends Defines {
	private CacheClientImpl client;

	public CacheClient(CacheClientManager manager, int groupID) {
		client = new CacheClientImpl(manager, groupID);
	}
/*
	public CacheClient(String managerName, int groupID) {
		client = new CacheClientImpl(managerName, groupID);
	}
*/
	public int getGroupID() {
		return client.getGroupID();
	}

//	public void setGroupID(int groupID) {
//		client.setGroupID(groupID);
//	}
	
	public String getLastError() {
		return client.getLastError();
	}

	public void setTransCoder(TransCoder transCoder) {
		client.setTransCoder(transCoder);
	}

	public TransCoder getTransCoder() {
		return client.getTransCoder();
	}

	public boolean delete(String key) {
		return client.delete(key, Defines.NO_CAS);
	}

	public boolean delete(String key, long cacheID) {
		return client.delete(key, cacheID);
	}

	public long set(String key, Object value) {
		return client.set(key, value, Defines.NO_EXPIRATION, Defines.NO_CAS, false);
	}

	public long set(String key, Object value, int expiration) {
		return client.set(key, value, expiration, Defines.NO_CAS, false);
	}

	public long set(String key, Object value, int expiration, long cacheID) {
		return client.set(key, value, expiration, cacheID, false);
	}

	public long setW(String key, Object value) {
		return client.set(key, value, Defines.NO_EXPIRATION, Defines.NO_CAS, true);
	}

	public long setW(String key, Object value, int expiration) {
		return client.set(key, value, expiration, Defines.NO_CAS, true);
	}

	public long setW(String key, Object value, int expiration, long cacheID) {
		return client.set(key, value, expiration, cacheID, true);
	}

	public long add(String key, Object value) {
		return client.add(key, value, Defines.NO_EXPIRATION, false);
	}

	public long add(String key, Object value, int expiration) {
		return client.add(key, value, expiration, false);
	}

	public long addW(String key, Object value) {
		return client.add(key, value, Defines.NO_EXPIRATION, true);
	}

	public long addW(String key, Object value, int expiration) {
		return client.add(key, value, expiration, true);
	}

	public long replace(String key, Object value) {
		return client.replace(key, value, Defines.NO_EXPIRATION, Defines.NO_CAS, false);
	}

	public long replace(String key, Object value, int expiration) {
		return client.replace(key, value, expiration, Defines.NO_CAS, false);
	}

	public long replace(String key, Object value, int expiration, long cacheID) {
		return client.replace(key, value, expiration, cacheID, false);
	}

	public long replaceW(String key, Object value) {
		return client.replace(key, value, Defines.NO_EXPIRATION, Defines.NO_CAS, true);
	}

	public long replaceW(String key, Object value, int expiration) {
		return client.replace(key, value, expiration, Defines.NO_CAS, true);
	}

	public long replaceW(String key, Object value, int expiration, long cacheID) {
		return client.replace(key, value, expiration, cacheID, true);
	}

	public long createDelta(String key, long delta) {
		return add(key, "" + delta, Defines.NO_EXPIRATION);
	}
	
	public long createDelta(String key, long delta, int expiration) {
		return add(key, "" + delta, expiration);
	}
	
	public long setDelta(String key, long delta) {
		return set(key, "" + delta, Defines.NO_EXPIRATION, Defines.NO_CAS);
	}
	
	public long setDelta(String key, long delta, int expiration) {
		return set(key, "" + delta, expiration, Defines.NO_CAS);
	}
	
	public long setDelta(String key, long delta, int expiration, long cacheID) {
		return set(key, "" + delta, expiration, cacheID);
	}

	public DeltaItem incr(String key) {
		return client.incr(key, 1, Defines.NO_CAS);
	}

	public DeltaItem incr(String key, long delta) {
		return client.incr(key, delta, Defines.NO_CAS);
	}

	public DeltaItem incr(String key, long delta, long cacheID) {
		return client.incr(key, delta, cacheID);
	}

	public DeltaItem decr(String key) {
		return client.decr(key, 1, Defines.NO_CAS);
	}

	public DeltaItem decr(String key, long delta) {
		return client.decr(key, delta, Defines.NO_CAS);
	}

	public DeltaItem decr(String key, long delta, long cacheID) {
		return client.decr(key, delta, cacheID);
	}

	public Object get(String key) {
		CacheItem item = client.get(key, 0, 0);
		if (item != null) {
			return item.getValue();
		}
		return null;
	}

	public Object getL(String key) {
		CacheItem item = client.get(key, LOCAL_CACHE, 0);
		if (item != null) {
			return item.getValue();
		}
		return null;
	}

	public Object getW(String key) {
		CacheItem item = client.get(key, WATCH_CACHE, 0);
		if (item != null) {
			return item.getValue();
		}
		return null;
	}

	public Object getLW(String key) {
		CacheItem item = client.get(key, LOCAL_CACHE | WATCH_CACHE, 0);
		if (item != null) {
			return item.getValue();
		}
		return null;
	}

	public CacheBaseItem getBase(String key) {
		return client.getBase(key);
	}

	public boolean keyExists(String key) {
		return client.getBase(key) != null;
	}

	public CacheItem gets(String key) {
		return client.get(key, 0, 0);
	}

	public CacheItem getsL(String key) {
		return client.get(key, LOCAL_CACHE, 0);
	}

	public CacheItem getsW(String key) {
		return client.get(key, WATCH_CACHE, 0);
	}

	public CacheItem getsLW(String key) {
		return client.get(key, LOCAL_CACHE | WATCH_CACHE, 0);
	}
	
	public CacheItem getAndTouch(String key, int expiration) {
		return client.get(key, TOUCH_CACHE, expiration);
	}

	public CacheItem getAndTouchL(String key, int expiration) {
		return client.get(key, LOCAL_CACHE | TOUCH_CACHE, expiration);
	}

	public CacheItem getAndTouchW(String key, int expiration) {
		return client.get(key, WATCH_CACHE | TOUCH_CACHE, expiration);
	}

	public CacheItem getAndTouchLW(String key, int expiration) {
		return client.get(key, LOCAL_CACHE | WATCH_CACHE | TOUCH_CACHE,
				expiration);
	}

	public List<CacheItem> multiGet(List<String> keys) {
		return client.multiGet(keys);
	}
	
	public int multiSet(List<MultiUpdateItem> items) {
		return client.multiSet(items);
	}
	
	public int multiAdd(List<MultiUpdateItem> items) {
		return client.multiAdd(items);
	}
	
	public int multiReplace(List<MultiUpdateItem> items) {
		return client.multiReplace(items);
	}
	
	public int multiAppend(List<MultiUpdateItem> items) {
		return client.multiAppend(items);
	}
	
	public int multiPrepend(List<MultiUpdateItem> items) {
		return client.multiPrepend(items);
	}
	
	public int multiDelete(List<MultiDeleteItem> items) {
		return client.multiDelete(items);
	}
	
	public int flush() {
		return client.flush();
	}

	public int flush(String[] servers) {
		return client.flush(servers);
	}

	public long append(String key, Object value) {
		return client.append(key, value, Defines.NO_CAS);
	}

	public long append(String key, Object value, long cacheID) {
		return client.append(key, value, cacheID);
	}

	public long prepend(String key, Object value) {
		return client.prepend(key, value, Defines.NO_CAS);
	}

	public long prepend(String key, Object value, long cacheID) {
		return client.prepend(key, value, cacheID);
	}
	
	public boolean statsAddGroup(String[] servers, int groupID) {
		return client.statsAddGroup(servers, groupID);
	}
	
	public boolean statsRemoveGroup(String[] servers, int groupID) {
		return client.statsRemoveGroup(servers, groupID);
	}
	
	public Map<String, Map<String, String>> statsGetStats(String[] servers, byte class_id) {
		Map<String, Map<String, String>> result = new HashMap<String, Map<String, String>>();
		if (client.statsGetStats( servers, class_id, result)) {
			return result;
		} else {
			return null;
		}
	}
/*
	public Map<String, Map<String, String>> statsGetAndClearStats(String[] servers, byte class_id) {
		Map<String, Map<String, String>> result = new HashMap<String, Map<String, String>>();
		if (client.statsGetAndClearStats(servers, class_id, result)) {
			return result;
		} else {
			return null;
		}
	}
*/
	public Map<String, Map<String, String>> statsGetGroupStats(String[] servers, int groupID, byte class_id) {
		Map<String, Map<String, String>> result = new HashMap<String, Map<String, String>>();
		if (client.statsGetGroupStats(servers, groupID, class_id, result)) {
			return result;
		} else {
			return null;
		}
	}
/*
	public Map<String, Map<String, String>> statsGetAndClearGroupStats(String[] servers, int groupID, byte class_id) {
		Map<String, Map<String, String>> result = new HashMap<String, Map<String, String>>();
		if (client.statsGetAndClearGroupStats(servers, groupID, class_id, result)) {
			return result;
		} else {
			return null;
		}
	}
*/
}

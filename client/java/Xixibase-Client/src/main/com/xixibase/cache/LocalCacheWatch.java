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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.Map.Entry;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

import com.xixibase.cache.multi.MultiUpdateBaseItem;
import com.xixibase.util.CurrentTick;
import com.xixibase.util.Log;

class LocalCacheWatch extends Thread {
	private static Log log = Log.getLog(LocalCacheWatch.class.getName());
	
	private String host;
	private CacheClientManager manager;
	private int watchID = 0;
	private boolean runFlag = false;
	private HashMap<Object, CacheItem> inactiveCacheMap = new HashMap<Object, CacheItem>();
	private ConcurrentHashMap<Object, CacheItem> activeCacheMap = new ConcurrentHashMap<Object, CacheItem>();
	private HashMap<Long, Object> cacheIDMap = new HashMap<Long, Object>();
	private AtomicLong cacheSize = null;
	private AtomicInteger cacheCount = null;
	private long ackCacheID = 0;
	private int maxNextCheckInterval = 600;
	private CacheClientImpl cc = null;
	private LocalCacheTouch cacheTouch = null;

	public LocalCacheWatch(String host, CacheClientManager manager,
			AtomicLong cacheSize, AtomicInteger cacheCount) {
		this.host = host;
		this.manager = manager;
		this.cacheSize = cacheSize;
		this.cacheCount = cacheCount;
	}
	
	public int getWatchID() {
		return watchID;
	}
	
	public void init() {
		runFlag = true;
		cc = new CacheClientImpl(manager);
		cacheTouch = new LocalCacheTouch(manager);
		this.start();
		cacheTouch.start();
	}

	public void shutdown() {
		runFlag = false;
		cacheTouch.shutdown();
		clear();
	}

	private synchronized void clear() {
		watchID = 0;
		Iterator<CacheItem> it = inactiveCacheMap.values().iterator();
		while (it.hasNext()) {
			CacheItem item = it.next();
			cacheSize.addAndGet(-item.valueSize);
			cacheCount.getAndDecrement();
		}
		inactiveCacheMap.clear();
		it = activeCacheMap.values().iterator();
		while (it.hasNext()) {
			CacheItem item = it.next();
			cacheSize.addAndGet(-item.valueSize);
			cacheCount.getAndDecrement();
		}
		activeCacheMap.clear();

		cacheIDMap.clear();
	}

	public synchronized void dropInactive(int maxCount) {
		log.debug("dropInactive host=" + host + " inactiveSize=" + inactiveCacheMap.size()
				+ " activeSize=" + activeCacheMap.size());
		LinkedList<CacheItem> list = new LinkedList<CacheItem>();
		Iterator<CacheItem> it = inactiveCacheMap.values().iterator();
		
		while (maxCount > 0 && it.hasNext()) {
			CacheItem item = it.next();
			list.add(item);
			maxCount--;
		}
		while (!list.isEmpty()) {
			CacheItem item = list.pop();
			Object key = cacheIDMap.remove(new Long(item.cacheID));
			if (key != null) {
				item = inactiveCacheMap.remove(key);
				if (item != null) {
					cacheSize.addAndGet(-item.valueSize);
					cacheCount.getAndDecrement();
				}
			}
		}

		Iterator<Entry<Object, CacheItem>> ite = activeCacheMap.entrySet().iterator();
		while (ite.hasNext()) {
			Entry<Object, CacheItem> e = ite.next();
			Object key = e.getKey();
			CacheItem item = e.getValue();
			inactiveCacheMap.put(key, item);
		}
		activeCacheMap.clear();
	}

	public CacheItem get(Object key) {
		CacheItem item = activeCacheMap.get(key);
		if (item != null) {
			return item;
		}
		synchronized (this) {
			item = inactiveCacheMap.remove(key);
			if (item != null) {
				activeCacheMap.put(key, item);
			}
		}
		return item;
	}
	
	public CacheItem getAndTouch(String key, int expiration) {
		CacheItem item = get(key);
		if (item != null) {
			cacheTouch.touch(key, item, expiration);
		}
		return item;
	}

	public synchronized void put(Object key, CacheItem value) {
		CacheItem oldItem = inactiveCacheMap.put(key, value);
		if (oldItem != null) {
			cacheSize.addAndGet(-oldItem.valueSize);
			cacheCount.getAndDecrement();
			cacheIDMap.remove(new Long(oldItem.cacheID));
		} else {
			oldItem = activeCacheMap.remove(key);
			if (oldItem != null) {
				cacheSize.addAndGet(-oldItem.valueSize);
				cacheCount.getAndDecrement();
				cacheIDMap.remove(new Long(oldItem.cacheID));
			}
		}
		cacheSize.addAndGet(value.valueSize);
		cacheCount.getAndIncrement();
		cacheIDMap.put(new Long(value.cacheID), key);
	}

	public synchronized CacheItem remove(Object key) {
		CacheItem item = activeCacheMap.remove(key);
		if (item != null) {
			cacheSize.addAndGet(-item.valueSize);
			cacheCount.getAndDecrement();
			cacheIDMap.remove(new Long(item.cacheID));
		} else {
			item = inactiveCacheMap.remove(key);
			if (item != null) {
				cacheSize.addAndGet(-item.valueSize);
				cacheCount.getAndDecrement();
				cacheIDMap.remove(new Long(item.cacheID));
			}
		}
		return item;
	}

	protected synchronized void update(long[] cacheIDList) {
		//	log.debug("update count=" + updated.length);
		for (int i = 0; i < cacheIDList.length; i++) {
			Long cacheID = new Long(cacheIDList[i]);
			Object key = cacheIDMap.remove(cacheID);
			if (key != null) {
				CacheItem item = activeCacheMap.remove(key);
				if (item != null) {
					if (item.cacheID != cacheIDList[i]) {
						activeCacheMap.put(key, item);
					} else {
						cacheSize.addAndGet(-item.valueSize);
						cacheCount.getAndDecrement();
					}
				} else {
					item = inactiveCacheMap.remove(key);
					if (item != null) {
						if (item.cacheID != cacheIDList[i]) {
							inactiveCacheMap.put(key, item);
						} else {
							cacheSize.addAndGet(-item.valueSize);
							cacheCount.getAndDecrement();
						}					
					}
				}
			}
		}
	}
	
	public void run() {
		log.info("LocalCacheWatch " + host + " run");
		while (runFlag) {
			while (watchID == 0 && runFlag) {
				watchID = cc.createWatch(host, maxNextCheckInterval);
				log.debug("LocalCacheWatch " + host + " watchID=" + watchID);
				if (watchID == 0 && runFlag) {
					try {
						Thread.sleep(1000);
					} catch (InterruptedException e) {
						e.printStackTrace();
					}
				}
			}
	//		log.error("localCache updater " + host + " watchID=" + watchID);
			if (!runFlag) {
				break;
			}
			long beginTime = System.currentTimeMillis();

			long[] cacheIDList = cc.checkWatch(host, watchID, maxNextCheckInterval, ackCacheID);
			long endTime = System.currentTimeMillis();
			if (cacheIDList == null) {
				if (!runFlag) {
					break;
				}
				cacheIDList = cc.checkWatch(host, watchID, maxNextCheckInterval, ackCacheID);
			}
			if (cacheIDList != null) {
				if (cacheIDList.length > 0) {
					update(cacheIDList);
					ackCacheID = cacheIDList[0];
				} else {
					if (endTime - beginTime < 200) {
						try {
							Thread.sleep(200);
						} catch (InterruptedException e) {
							e.printStackTrace();
						}
					}
				}
			} else {
				log.error("LocalCacheWatch can not check with server:" + host + " watchID=" + watchID
						+ ", clear all local cache");
				clear();
			}
		}
	}
}

class LocalCacheTouch extends Thread {
	private ConcurrentHashMap<String, CacheItem> touchMap = new ConcurrentHashMap<String, CacheItem>();
	private CacheClientImpl cc = null;
	private boolean runFlag = false;
	LocalCacheTouch(CacheClientManager manager) {
		cc = new CacheClientImpl(manager);
	}
	
	public void shutdown() {
		runFlag = false;
	}
	
	protected void touch(String key, CacheItem item, int expiration) {
		item.setExpiration(expiration);
		touchMap.put(key, item);
	}
	
	protected int touchToServer() {
		Iterator<String> it = touchMap.keySet().iterator();
		ArrayList<MultiUpdateBaseItem> lists = new ArrayList<MultiUpdateBaseItem>();
		while (it.hasNext()) {
			String key = it.next();
			CacheItem item = touchMap.remove(key);//e.getValue();
			if (item != null) {
				MultiUpdateBaseItem mitem = new MultiUpdateBaseItem();
				lists.add(mitem);
				mitem.key = key;
				mitem.cacheID = item.cacheID;
				long curr_time = CurrentTick.get();
				if (item.getExpireTime() > curr_time) {
					long expiration = item.getExpiration();
					if (expiration <= 0xFFFFFFFFL) {
						mitem.expiration = (int)expiration;
					} else {
						mitem.expiration = (int)0xFFFFFFFFL;
					}
				} else {
					mitem.expiration = 0;
				}
			}
		}
		return cc.multiUpdateExpiration(lists);
	}
	
	public void run() {
		runFlag = true;
		while (runFlag) {
			int count = 0;
			while (!touchMap.isEmpty()) {
				count = touchToServer();
			}
			if (count == 0) {
				try {
					Thread.sleep(10);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
			}
		}
	}
}
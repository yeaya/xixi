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

import java.util.Iterator;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

import com.xixibase.util.Log;

public class LocalCache {
	private static Log log = Log.getLog(LocalCache.class.getName());

	private ConcurrentHashMap<String, LocalCacheWatch> watchMap = new ConcurrentHashMap<String, LocalCacheWatch>();
	private CacheClientManager manager;
	private AtomicLong cacheSize = new AtomicLong(0);
	private AtomicInteger cacheCount = new AtomicInteger(0);
	private long maxCacheSize;
	private long warningCacheSize;
	private double warningCacheRate = 0.7;
	private int maxDropCount = 100;
	
	protected LocalCache(CacheClientManager manager, long maxCacheSize) {
		this.manager = manager;
		this.maxCacheSize = maxCacheSize;
		this.maxCacheSize = maxCacheSize;
		this.warningCacheSize = (long)(maxCacheSize * warningCacheRate);
	}

	public long getMaxCacheSize() {
		return maxCacheSize;
	}
	
	public void setMaxCacheSize(long maxCacheSize) {
		this.maxCacheSize = maxCacheSize;
		this.warningCacheSize = (long)(maxCacheSize * warningCacheRate);
		log.info("setMaxCacheSize, maxCacheSize=" + maxCacheSize);
	}
	
	public double getWarningCacheRate() {
		return warningCacheRate;
	}
	
	public void setWarningCacheRate(double warningCacheRate) {
		this.warningCacheRate = warningCacheRate;
		this.warningCacheSize = (long)(maxCacheSize * warningCacheRate);
	}
	
	public long getCacheSize() {
		return cacheSize.get();
	}
	
	public int getCacheCount() {
		return cacheCount.get();
	}

	public int getWatchID(String host) {
		LocalCacheWatch updater = watchMap.get(host);
		if (updater != null) {
			return updater.getWatchID();
		}
		return 0;
	}

	protected void start() {
		if (watchMap.size() > 0) {
			log.warn("start, LocalCache " + manager.getName() + " was already started.");
			return;
		}
		log.info("start, maxCacheSize=" + maxCacheSize);
		String[] hosts = manager.getServers();
		for (int i = 0; i < hosts.length; i++) {
			String host = hosts[i];
			LocalCacheWatch updater = new LocalCacheWatch(host, manager,
					cacheSize, cacheCount);
			watchMap.put(host, updater);
			log.info("start, localCache updater " + host + " start");
			updater.init();
		}
	}
	
	protected void stop() {
		Iterator<LocalCacheWatch> it = watchMap.values().iterator();
		while (it.hasNext()) {
			LocalCacheWatch updater = it.next();
			updater.shutdown();
		}
		watchMap.clear();
	}

	public void dropInactive(int maxDropCount) {
		Iterator<LocalCacheWatch> it = watchMap.values().iterator();
		while (it.hasNext()) {
			LocalCacheWatch updater = it.next();
			updater.dropInactive(maxDropCount);
		}
	}
	
	public CacheItem get(String host, int groupID, String key) {
		LocalCacheWatch updater = watchMap.get(host);
		if (updater != null) {
			CacheItem item = updater.get(groupID, key);
			return item;
		}
		return null;
	}

	public CacheItem get(int groupID, String key) {
		String host = manager.getHost(key);
		return get(host, groupID, key);
	}

	public CacheItem getAndTouch(String host, int groupID, String key, int expiration) {
		LocalCacheWatch updater = watchMap.get(host);
		if (updater != null) {
			CacheItem item = updater.getAndTouch(groupID, key, expiration);
			return item;
		}
		return null;
	}
/*
	public CacheItem getAndTouch(int groupID, String key, int expiration) {
		String host = manager.getHost(key);
		return getAndTouch(host, groupID, key, expiration);
	}
*/	
	public void put(String host, String key, CacheItem value) {
		LocalCacheWatch watch = watchMap.get(host);
		if (watch != null) {
			if (cacheSize.longValue() > warningCacheSize) {
				dropInactive(maxDropCount);
				if (cacheSize.longValue() + value.itemSize < maxCacheSize) {
					watch.put(key, value);
				}
			} else {
				watch.put(key, value);
			}
		}
	}
/*
	public void put(String key, CacheItem value) {
		String host = manager.getHost(key);
		put(host, key, value);
	}
*/
	public CacheItem remove(String host, int groupID, String key) {
		LocalCacheWatch updater = watchMap.get(host);
		if (updater != null) {
			CacheItem item = updater.remove(groupID, key);
			return item;
		}
		return null;
	}
	
	public CacheItem remove(int groupID, String key) {
		String host = manager.getHost(key);
		return remove(host, groupID, key);
	}
}
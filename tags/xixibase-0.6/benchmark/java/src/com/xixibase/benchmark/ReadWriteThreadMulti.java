package com.xixibase.benchmark;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.CyclicBarrier;
import java.util.concurrent.atomic.AtomicLong;

import com.xixibase.cache.CacheClient;
import com.xixibase.cache.CacheItem;
import com.xixibase.cache.multi.MultiUpdateItem;

import net.rubyeye.memcached.BaseReadWriteThread;
import net.rubyeye.memcached.benchmark.Constants;

public class ReadWriteThreadMulti extends BaseReadWriteThread {
	CacheClient cc;
	ArrayList<MultiUpdateItem> multiList = new ArrayList<MultiUpdateItem>();
	HashMap<String, String> map = null;
	int count;
	int writeOffset = 0;
	int readOffset = 0;
	
	public ReadWriteThreadMulti(CacheClient cc, int repeats,
			CyclicBarrier barrier, int offset, int length, AtomicLong miss,
			AtomicLong fail, AtomicLong hit) {
		super(repeats, barrier, offset, length, miss, fail, hit);
		this.cc = cc;
		count = (int) (this.repeats * Constants.WRITE_RATE) + 1;
	}

	public boolean set(int i, String s) throws Exception {
		
		MultiUpdateItem item = new MultiUpdateItem();
		item.key = String.valueOf(i);
		item.value = s;
		multiList.add(item);
		int c = count - writeOffset;
		if (c > 100) {
			c = 100;
		}
		if (multiList.size() == c) {
			cc.multiSet(multiList);
			writeOffset += c;
			multiList.clear();
		}
		return true;
//		return cc.set(String.valueOf(i), s);
	}

	public String get(int n) throws Exception {
		String key = String.valueOf(n);
		if (map != null) {
			String v = map.get(key);
			if (v != null) {
				return v;
			}
			map = null;
		}
		if (map == null) {
			map = new HashMap<String, String>();
			int c = count - readOffset;
			if (c > 100) {
				c = 100;
			}
			ArrayList<String> keys = new ArrayList<String>();
			for (int i = 0; i < c; i++) {
				keys.add(String.valueOf(n + i));
			}
			readOffset += c;
			if (readOffset == count) {
				readOffset = 0;
			}
			List<CacheItem> cacheList = cc.multiGet(keys);
			if (cacheList != null) {
				Iterator<CacheItem> it = cacheList.iterator();
				int index = 0;
				while (it.hasNext()) {
					CacheItem item = it.next();
					map.put(keys.get(index++), (String)item.getValue());
				}
			} else {
				return null;
			}
		}
		return map.get(key);
//		String result = (String)this.cc.get(String.valueOf(n));
	//	return result;
	}
}

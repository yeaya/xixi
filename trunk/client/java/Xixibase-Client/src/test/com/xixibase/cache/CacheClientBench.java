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
import java.util.List;

import com.xixibase.cache.multi.MultiDeleteItem;
import com.xixibase.cache.multi.MultiUpdateItem;

public class CacheClientBench  {
	int start = 1;
	int runs = 50000;
	String keyBase = "key";
	String object = "value";
	CacheClient cc;
	
	public CacheClientBench(String servers, int start, int runs) {
		init(servers);
		this.start = start;
		this.runs = runs;
	}
	
	protected void init(String servers) {
		String[] serverlist = servers.split(",");

		String name = "test";
		CacheClientManager manager = CacheClientManager.getInstance(name);
		manager.setInitConn(1);
		manager.setNagle(false);
		manager.initialize(serverlist);
		manager.enableLocalCache();
		manager.getLocalCache().setMaxCacheSize(128 * 1024 * 1024);
		
		cc = manager.createClient();
		
		System.out.println("keybaselen=" + keyBase.length() + " objlen=" + object.length());
	}

	public boolean runIt() {
		boolean ret = true;
		ret &= set();
		ret &= getW();
		ret &= getL();
		ret &= multiGet();
		ret &= delete();
		ret &= multiSet();
		ret &= multiGetForSet();
		ret &= multiDelete();
		ret &= multiSet();
		ret &= flush();
		
		CacheClientManager.getInstance("test").shutdown();
		return ret;
	}

	boolean set() {
		boolean r = true;
		long begin = System.currentTimeMillis();
		for (int i = start; i < start + runs; i++) {
			long ret = cc.set(keyBase + i, object);
			if (ret == 0) {
				r = false;
				System.out.println("set error index=" + i + " obj=" + object);
			}
		}
		long end = System.currentTimeMillis();
		long time = end - begin;
		System.out.println(runs + " sets: " + time + "ms");
		return r;
	}
	
	boolean getW() {
		boolean r = true;
		long begin = System.currentTimeMillis();
		String obj = "";
		for (int i = start; i < start + runs; i++) {
			obj = (String)cc.getW(keyBase + i);
			if (!object.equals(obj)) {
				r = false;
				System.out.println("getW error index=" + i + " obj=" + obj);
			}
		}
		long end = System.currentTimeMillis();
		long time = end - begin;
		System.out.println(runs + " getW: " + time + "ms");
		return r;
	}
	
	boolean getL() {
		boolean r = true;
		long begin = System.currentTimeMillis();
		Object obj = "";
		for (int i = start; i < start + runs; i++) {
			obj = (String)cc.getL(keyBase + i);
			if (!object.equals(obj)) {
				r = false;
				System.out.println("getL error index=" + i + " obj=" + obj);
			}
		}
		long end = System.currentTimeMillis();
		long time = end - begin;
		System.out.println(runs + " getL: " + time + "ms");
		return r;
	}
	
	boolean multiGet() {
		ArrayList<String> keys = new ArrayList<String>(runs);
		for (int i = start; i < start + runs; i++) {
			keys.add(keyBase + i);
		}
		long begin = System.currentTimeMillis();
		List<CacheItem> m = cc.multiGet(keys);
		long end = System.currentTimeMillis();
		long time = end - begin;
		int missCount = 0;
		for (int i = start; i < start + runs; i++) {
			String key = keyBase + i;
			if (m.get(i - start) == null) {
				System.out.println("can not find the key=" + key);
				missCount++;
			}
		}
		System.out.println(runs + " multiGet: " + time + "ms" + " count=" + m.size()
				+ " missCount=" + missCount);
		return missCount == 0;
	}
	
	boolean multiSet() {
		ArrayList<MultiUpdateItem> items = new ArrayList<MultiUpdateItem>(runs);
		String value = object.toString() + 1;
		for (int i = start; i < start + runs; i++) {
			MultiUpdateItem item = new MultiUpdateItem();
			items.add(item);
			item.key = keyBase + i;
			item.value = value;
			item.cacheID = 0;
		}
		long begin = System.currentTimeMillis();
		int count = cc.multiSet(items);
		long end = System.currentTimeMillis();
		long time = end - begin;
		int missCount = 0;
		for (int i = 0; i < items.size(); i++) {
			MultiUpdateItem item = items.get(i);
			if (item.getNewCacheID() == 0) {
				missCount++;
			}
		}
		
		System.out.println(runs + " multiSet: " + time + "ms" + " count=" + count
				+ " missCount=" + missCount);
		return missCount == 0;
	}
	
	boolean multiGetForSet() {
		ArrayList<String> keys = new ArrayList<String>(runs);
		for (int i = start; i < start + runs; i++) {
			keys.add(keyBase + i);
		}
		long begin = System.currentTimeMillis();
		List<CacheItem> items = cc.multiGet(keys);
		long end = System.currentTimeMillis();
		long time = end - begin;
		int missCount = 0;
		String value = object.toString() + 1;
		for (int i = start; i < start + runs; i++) {
			String key = keyBase + i;
			CacheItem item = items.get(i - start);
			if (item == null || !item.getValue().equals(value)) {
				System.out.println("not key=" + key);
				missCount++;
			}
		}
		System.out.println(runs + " multiGetForSet: " + time + "ms" + " count=" + items.size()
				+ " missCount=" + missCount);
		return missCount == 0;
	}
	
	boolean multiDelete() {
		ArrayList<MultiDeleteItem> items = new ArrayList<MultiDeleteItem>(runs);
		for (int i = start; i < start + runs; i++) {
			MultiDeleteItem item = new MultiDeleteItem();
			items.add(item);
			item.key = keyBase + i;
			item.cacheID = 0;
		}
		long begin = System.currentTimeMillis();
		int count = cc.multiDelete(items);
		long end = System.currentTimeMillis();
		long time = end - begin;
		
		System.out.println(runs + " multiDelete: " + time + "ms" + " count=" + count);
		return true;
	}
	
	boolean delete() {
		long begin = System.currentTimeMillis();
		for (int i = start; i < start + runs; i++) {
			cc.delete(keyBase + i);
		}
		long end = System.currentTimeMillis();
		long time = end - begin;
		System.out.println(runs + " deletes: " + time + "ms");
		return true;
	}

	boolean flush() {
		long begin = System.currentTimeMillis();
		int count = cc.flush();
		long end = System.currentTimeMillis();
		long time = end - begin;
		System.out.println(runs + " flush: " + time + "ms");
		return count == runs;
	}
	
	public static void main(String[] args) {
		String servers = System.getProperty("hosts");
		if (args.length >= 1) {
			servers = args[0];
		}
		CacheClientBench bench = new CacheClientBench(servers, 1, 50000);
		bench.runIt();
	}
}

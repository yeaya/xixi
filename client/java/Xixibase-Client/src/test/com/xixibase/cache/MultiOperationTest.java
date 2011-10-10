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

import junit.framework.TestCase;


public class MultiOperationTest extends TestCase {
	private static final String managerName1 = "manager1";
	private static CacheClient cc1 = null;

	static String servers;
	static {
		servers = System.getProperty("hosts");
		if (servers == null) {
			servers = "localhost:7788";
		}
		String[] serverlist = servers.split(",");

		CacheClientManager mgr1 = CacheClientManager.getInstance(managerName1);
		mgr1.setSocketWriteBufferSize(64 * 1024);
		mgr1.initialize(serverlist);
		mgr1.enableLocalCache();
	}

	protected void setUp() throws Exception {
		super.setUp();
		cc1 = new CacheClient(managerName1);
	}

	protected void tearDown() throws Exception {
		super.tearDown();
		assertNotNull(cc1);
		cc1.flush();
	}

	public void testMultiAdd() {
		int max = 100;
		ArrayList<MultiUpdateItem> multi = new ArrayList<MultiUpdateItem>();
		ArrayList<String> keys = new ArrayList<String>();
		ArrayList<String> values = new ArrayList<String>();
		for (int i = 0; i < max; i++) {
			String key = Integer.toString(i);
			String value = "value" + i;
			MultiUpdateItem item = new MultiUpdateItem();
			item.key = key;
			item.value = value;
			multi.add(item);
			keys.add(key);
			values.add(value);
		}

		int ret = cc1.multiAdd(multi);
		assertEquals(max, ret);
		List<CacheItem> results = cc1.multiGet(keys);
		for (int i = 0; i < max; i++) {
			CacheItem item = results.get(i);
			assertEquals(item.getValue(), values.get(i));
		}
	}
	
	public void testMultiAppend() {
		int max = 100;
		ArrayList<MultiUpdateItem> multi = new ArrayList<MultiUpdateItem>();
		ArrayList<String> keys = new ArrayList<String>();
		ArrayList<String> values = new ArrayList<String>();
		for (int i = 0; i < max; i++) {
			String key = Integer.toString(i);
			String value = "value" + i;
			MultiUpdateItem item = new MultiUpdateItem();
			item.key = key;
			item.value = value;
			multi.add(item);
			keys.add(key);
			values.add(value);
		}

		int ret = cc1.multiAdd(multi);
		assertEquals(max, ret);
		
		for (int i = 0; i < max; i++) {
			multi.get(i).value = "append";
		}
		
		ret = cc1.multiAppend(multi);
		assertEquals(max, ret);
		
		List<CacheItem> results = cc1.multiGet(keys);
		for (int i = 0; i < max; i++) {
			CacheItem item = results.get(i);
			assertEquals(item.getValue(), values.get(i) + "append");
		}
	}
	
	public void testMultiPrepend() {
		int max = 100;
		ArrayList<MultiUpdateItem> multi = new ArrayList<MultiUpdateItem>();
		ArrayList<String> keys = new ArrayList<String>();
		ArrayList<String> values = new ArrayList<String>();
		for (int i = 0; i < max; i++) {
			String key = Integer.toString(i);
			String value = "value" + i;
			MultiUpdateItem item = new MultiUpdateItem();
			item.key = key;
			item.value = value;
			multi.add(item);
			keys.add(key);
			values.add(value);
		}

		int ret = cc1.multiAdd(multi);
		assertEquals(max, ret);
		
		ArrayList<MultiUpdateItem> multi2 = new ArrayList<MultiUpdateItem>();
		for (int i = 0; i < max; i++) {
			MultiUpdateItem item = new MultiUpdateItem();
			item.key = multi.get(i).key;
			item.value = "prepend";
			multi2.add(item);
		}
		
		ret = cc1.multiPrepend(multi2);
		assertEquals(max, ret);
		
		List<CacheItem> results = cc1.multiGet(keys);
		for (int i = 0; i < max; i++) {
			CacheItem item = results.get(i);
			assertEquals(item.getValue(), "prepend" + values.get(i));
			assertTrue(item.cacheID != multi.get(i).cacheID);
		}
	}
	
	public void testMultiSet() {
		int max = 100;
		ArrayList<String> keys = new ArrayList<String>();
		for (int i = 0; i < max; i++) {
			String key = Integer.toString(i);
			keys.add(key);
			cc1.set(key, "value" + i);
		}

		List<CacheItem> results = cc1.multiGet(keys);
		for (int i = 0; i < max; i++) {
			CacheItem item = results.get(i);
			assertEquals(item.getValue(), "value" + i);
		}
	}

	public void testMultiDelete() {
		int max = 100;
		ArrayList<MultiUpdateItem> multi = new ArrayList<MultiUpdateItem>();
		ArrayList<String> keys = new ArrayList<String>();
		ArrayList<String> values = new ArrayList<String>();
		for (int i = 0; i < max; i++) {
			String key = Integer.toString(i);
			String value = "value" + i;
			MultiUpdateItem item = new MultiUpdateItem();
			item.key = key;
			item.value = value;
			multi.add(item);
			keys.add(key);
			values.add(value);
		}

		int ret = cc1.multiAdd(multi);
		assertEquals(max, ret);
		ArrayList<MultiDeleteItem> md = new ArrayList<MultiDeleteItem>();
		for (int i = 0; i < multi.size(); i++) {
			MultiUpdateItem item = multi.get(i);
			MultiDeleteItem ditem = new MultiDeleteItem();
			ditem.key = item.key;
			ditem.cacheID = item.cacheID;
			md.add(ditem);
		}
		List<CacheItem> results = cc1.multiGet(keys);
		for (int i = 0; i < max; i++) {
			CacheItem item = results.get(i);
			assertEquals(item.getValue(), values.get(i));
		}
		cc1.multiDelete(md);
		results = cc1.multiGet(keys);
		for (int i = 0; i < max; i++) {
			CacheItem item = results.get(i);
			assertNull(item);
		}
	}
	
	public void testMultiError() {
		int ret = cc1.multiAdd(null);
		assertEquals(ret, 0);
		assertNotNull(cc1.getLastError());
		int max = 100;
		ArrayList<MultiUpdateItem> multi = new ArrayList<MultiUpdateItem>();
		ret = cc1.multiAdd(multi);
		assertEquals(ret, 0);
		assertNotNull(cc1.getLastError());
		ArrayList<String> keys = new ArrayList<String>();
		ArrayList<String> values = new ArrayList<String>();
		for (int i = 0; i < max; i++) {
			String key = Integer.toString(i);
			String value = "value" + i;
			MultiUpdateItem item = new MultiUpdateItem();
			item.key = key;
			item.value = value;
			multi.add(item);
			keys.add(key);
			values.add(value);
		}

		ret = cc1.multiAdd(multi);
		assertEquals(max, ret);
		ArrayList<MultiDeleteItem> md = new ArrayList<MultiDeleteItem>();
		for (int i = 0; i < multi.size(); i++) {
			MultiUpdateItem item = multi.get(i);
			MultiDeleteItem ditem = new MultiDeleteItem();
			ditem.key = item.key;
			ditem.cacheID = item.cacheID;
			md.add(ditem);
		}
		List<CacheItem> results = cc1.multiGet(keys);
		for (int i = 0; i < max; i++) {
			CacheItem item = results.get(i);
			assertEquals(item.getValue(), values.get(i));
		}
		cc1.multiDelete(md);
		results = cc1.multiGet(keys);
		for (int i = 0; i < max; i++) {
			CacheItem item = results.get(i);
			assertNull(item);
		}
	}
}
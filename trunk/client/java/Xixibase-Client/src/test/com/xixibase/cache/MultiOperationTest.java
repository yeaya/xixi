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

import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Properties;

import com.xixibase.cache.multi.MultiDeleteItem;
import com.xixibase.cache.multi.MultiUpdateItem;

import junit.framework.TestCase;


public class MultiOperationTest extends TestCase {
	private static final String managerName1 = "MultiOperationTest";
	private static CacheClient cc1 = null;
	private static CacheClientManager mgr1 = null;

	static String servers;
	static {
		servers = System.getProperty("hosts");
		if (servers == null) {
			try {
				InputStream in = new BufferedInputStream(new FileInputStream("test.properties"));
				Properties p = new Properties(); 
				p.load(in);
				in.close();
				servers = p.getProperty("hosts");
			} catch (IOException e) {
				e.printStackTrace();
			} 
		}
		String[] serverlist = servers.split(",");

		mgr1 = CacheClientManager.getInstance(managerName1);
		mgr1.setSocketWriteBufferSize(64 * 1024);
		mgr1.initialize(serverlist);
		mgr1.enableLocalCache();
	}

	protected void setUp() throws Exception {
		super.setUp();
		cc1 = mgr1.createClient();
	}

	protected void tearDown() throws Exception {
		super.tearDown();
		assertNotNull(cc1);
		cc1.flush();
	}

	public void testMultiGet() {

		String[] allKeys = { "key1", "key2", "key3", "key4", "key5", "key6", "key7" };
		String[] setKeys = { "key1", "key3", "key5", "key7" };

		for (String key : setKeys) {
			cc1.set(key, key + "xixi");
		}
		ArrayList<String> allKeyList = new ArrayList<String>();
		for (String key : allKeys) {
			allKeyList.add(key);
		}

		List<CacheItem> results = cc1.multiGet(allKeyList);

		HashMap<String, CacheItem> hm = new HashMap<String, CacheItem>();
		for (int i = 0; i < allKeyList.size(); i++) {
			hm.put(allKeyList.get(i), results.get(i));
		}
		assert allKeys.length == results.size();
		for (int i = 0; i < setKeys.length; i++) {
			String key = setKeys[i];
			String val = (String) hm.get(key).getValue();
			assertEquals(key + "xixi", val);
		}
	}

	public void testMultiGetError() {
		assertNull(cc1.multiGet(null));
		ArrayList<String> allKeyList = new ArrayList<String>();
		assertEquals(0, cc1.multiGet(allKeyList).size());
		
		String[] allKeys = { "key1", "key2", "key4", "key5", "key6", "key7" };
		String[] setKeys = { "key1", "key3", "key5", "key7" };

		for (String key : setKeys) {
			cc1.set(key, key + "xixi");
		}
		
		allKeyList.add(null);
		for (String key : allKeys) {
			allKeyList.add(key);
		}
		allKeyList.add(null);

		List<CacheItem> results = cc1.multiGet(allKeyList);

		HashMap<String, CacheItem> hm = new HashMap<String, CacheItem>();
		for (int i = 0; i < allKeyList.size(); i++) {
			hm.put(allKeyList.get(i), results.get(i));
		}
		assertEquals(allKeys.length + 2, results.size());
		for (int i = 0; i < setKeys.length; i++) {
			String key = setKeys[i];
			if (!key.equals("key3")) {
				String val = (String) hm.get(key).getValue();
				assertEquals(key + "xixi", val);
			}
		}
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
	
	public void testMultiSet() {
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

		int ret = cc1.multiSet(multi);
		assertEquals(max, ret);
		List<CacheItem> results = cc1.multiGet(keys);
		for (int i = 0; i < max; i++) {
			CacheItem item = results.get(i);
			assertEquals(item.getValue(), values.get(i));
		}
	}
	
	public void testMultiReplace() {
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

		int ret = cc1.multiSet(multi);
		assertEquals(max, ret);
		List<CacheItem> results = cc1.multiGet(keys);
		for (int i = 0; i < max; i++) {
			CacheItem item = results.get(i);
			assertEquals(item.getValue(), values.get(i));
		}
		
		ArrayList<MultiUpdateItem> multi2 = new ArrayList<MultiUpdateItem>();
		ArrayList<String> keys2 = new ArrayList<String>();
		ArrayList<String> values2 = new ArrayList<String>();
		for (int i = 0; i < max; i++) {
			String key = Integer.toString(i);
			String value = "value2" + i;
			MultiUpdateItem item = new MultiUpdateItem();
			item.key = key;
			item.value = value;
			multi2.add(item);
			keys2.add(key);
			values2.add(value);
		}

		ret = cc1.multiReplace(multi2);
		assertEquals(max, ret);
		List<CacheItem> results2 = cc1.multiGet(keys2);
		for (int i = 0; i < max; i++) {
			CacheItem item = results2.get(i);
			assertEquals(item.getValue(), values2.get(i));
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
	
	public void testMultiFlags() {
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
		
	//	ArrayList<MultiUpdateBaseItem> multibase = new ArrayList<MultiUpdateBaseItem>();
	//	for (int i = 0; i < max; i++) {
	//		MultiUpdateBaseItem item = new MultiUpdateBaseItem();
	//	}
		
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

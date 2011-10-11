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

import junit.framework.TestCase;

public class LocalCacheTest extends TestCase {

	static String servers;
	static {
		servers = System.getProperty("hosts");
		if (servers == null) {
			servers = "localhost:7788";
		}
	}

	protected void setUp() throws Exception {
		super.setUp();
	}

	protected void tearDown() throws Exception {
		super.tearDown();
	}
	
	public void testLocalCache() throws InterruptedException {
		CacheClientManager mgr = CacheClientManager.getInstance("LocalCacheTest");
		String[] serverlist = servers.split(",");
		mgr.initialize(serverlist);
		mgr.enableLocalCache();
		Thread.sleep(50);
		LocalCache lc = mgr.getLocalCache();
		CacheClient cc = mgr.createClient();
		cc.flush();
		assertEquals(lc.getCacheCount(), 0);
		cc.setW("xixi", "value");
		assertEquals(lc.get(cc.getGroupID(), "xixi").getValue(), "value");
		assertEquals(cc.get("xixi"), "value");
		assertEquals(cc.getL("xixi"), "value");
		assertEquals(cc.getW("xixi"), "value");
		assertEquals(cc.getLW("xixi"), "value");
		assertEquals(lc.get(cc.getGroupID(), "xixi").getValue(), "value");
		assertEquals(lc.getCacheCount(), 1);
		assertEquals(lc.getCacheSize(), 1029);
		assertEquals(lc.getMaxCacheSize(), 64 * 1024 * 1024);
		lc.setMaxCacheSize(1024 * 1024);
		assertEquals(lc.getMaxCacheSize(), 1024 * 1024);
		lc.setWarningCacheRate(0.8);
		assertEquals(lc.getWarningCacheRate(), 0.8);
		
		mgr.disableLocalCache();
		assertEquals(cc.get("xixi"), "value");
		assertEquals(cc.getL("xixi"), "value");
		assertEquals(cc.getW("xixi"), "value");
		assertEquals(cc.getLW("xixi"), "value");
		

		mgr.shutdown();
	}
	
	public void testSetW() throws InterruptedException {
		CacheClientManager mgr = CacheClientManager.getInstance("LocalCacheTest");
		String[] serverlist = servers.split(",");
		mgr.initialize(serverlist);
		mgr.enableLocalCache();
		Thread.sleep(50);
		LocalCache lc = mgr.getLocalCache();
		CacheClient cc = mgr.createClient();
		cc.flush();
		assertEquals(lc.getCacheCount(), 0);
		cc.setW("xixi", "value", 1);
		assertEquals(lc.get(cc.getGroupID(), "xixi").getValue(), "value");
		assertEquals(cc.get("xixi"), "value");
		assertEquals(cc.getL("xixi"), "value");
		assertEquals(cc.getW("xixi"), "value");
		assertEquals(cc.getLW("xixi"), "value");
		assertEquals(lc.get(cc.getGroupID(), "xixi").getValue(), "value");
		assertEquals(lc.getCacheCount(), 1);
		assertEquals(lc.getCacheSize(), 1029);
		assertEquals(lc.getMaxCacheSize(), 64 * 1024 * 1024);
		lc.setMaxCacheSize(1024 * 1024);
		assertEquals(lc.getMaxCacheSize(), 1024 * 1024);
		lc.setWarningCacheRate(0.8);
		assertEquals(lc.getWarningCacheRate(), 0.8);
		
		Thread.sleep(2000);
		
		assertNull(lc.get(cc.getGroupID(), "xixi"));
		assertNull(cc.get("xixi"));
		assertNull(cc.getL("xixi"));
		assertNull(cc.getW("xixi"));
		assertNull(cc.getLW("xixi"));
		assertEquals(lc.getCacheCount(), 0);
		assertEquals(lc.getCacheSize(), 0);
		assertEquals(lc.getMaxCacheSize(), 1024 * 1024);
		lc.setWarningCacheRate(0.8);
		assertEquals(lc.getWarningCacheRate(), 0.8);
		
		mgr.shutdown();
	}
	
	public void testSetW2() throws InterruptedException {
		CacheClientManager mgr = CacheClientManager.getInstance("LocalCacheTest");
		String[] serverlist = servers.split(",");
		mgr.initialize(serverlist);
		mgr.enableLocalCache();
		Thread.sleep(50);
		LocalCache lc = mgr.getLocalCache();
		CacheClient cc = mgr.createClient();
		cc.flush();
		assertEquals(lc.getCacheCount(), 0);
		long ret = cc.setW("xixi", "value", 1, 123);
		assertTrue(ret != 0);
		ret = cc.setW("xixi", "value", 1, 0);
		assertTrue(ret != 0);
		CacheItem item = cc.gets("xixi");
		assertEquals("value", item.getValue());
		assertEquals(lc.get(cc.getGroupID(), "xixi").getValue(), "value");
		assertEquals(cc.get("xixi"), "value");
		assertEquals(cc.getL("xixi"), "value");
		assertEquals(cc.getW("xixi"), "value");
		assertEquals(cc.getLW("xixi"), "value");
		assertEquals(lc.getCacheCount(), 1);
		assertEquals(lc.getCacheSize(), 1029);
		assertEquals(lc.getMaxCacheSize(), 64 * 1024 * 1024);
		lc.setMaxCacheSize(1024 * 1024);
		assertEquals(lc.getMaxCacheSize(), 1024 * 1024);
		lc.setWarningCacheRate(0.8);
		assertEquals(lc.getWarningCacheRate(), 0.8);
		ret = cc.setW("xixi", "value2", 1, item.getCacheID());
		assertTrue(ret != 0);
		
		Thread.sleep(2000);
		
		assertNull(cc.get("xixi"));
		assertNull(lc.get(cc.getGroupID(), "xixi"));
		assertNull(cc.getL("xixi"));
		assertNull(cc.getW("xixi"));
		assertNull(cc.getLW("xixi"));
		assertEquals(lc.getCacheCount(), 0);
		assertEquals(lc.getCacheSize(), 0);
		assertEquals(lc.getMaxCacheSize(), 1024 * 1024);
		lc.setWarningCacheRate(0.8);
		assertEquals(lc.getWarningCacheRate(), 0.8);
		
		mgr.shutdown();
	}
	
	public void testAddW() throws InterruptedException {
		CacheClientManager mgr = CacheClientManager.getInstance("LocalCacheTest");
		String[] serverlist = servers.split(",");
		mgr.initialize(serverlist);
		mgr.enableLocalCache();
		Thread.sleep(50);
		LocalCache lc = mgr.getLocalCache();
		CacheClient cc = mgr.createClient();
		cc.flush();
		assertEquals(lc.getCacheCount(), 0);
		cc.addW("xixi", "value");
		assertEquals(lc.get(cc.getGroupID(), "xixi").getValue(), "value");
		assertEquals(cc.get("xixi"), "value");
		assertEquals(cc.getL("xixi"), "value");
		assertEquals(cc.getW("xixi"), "value");
		assertEquals(cc.getLW("xixi"), "value");
		assertEquals(lc.get(cc.getGroupID(), "xixi").getValue(), "value");
		assertEquals(lc.getCacheCount(), 1);
		assertEquals(lc.getCacheSize(), 1029);
		assertEquals(lc.getMaxCacheSize(), 64 * 1024 * 1024);
		lc.setMaxCacheSize(1024 * 1024);
		assertEquals(lc.getMaxCacheSize(), 1024 * 1024);
		lc.setWarningCacheRate(0.8);
		assertEquals(lc.getWarningCacheRate(), 0.8);
		
		Thread.sleep(2000);
		
		assertEquals(lc.get(cc.getGroupID(), "xixi").getValue(), "value");
		assertEquals(cc.get("xixi"), "value");
		assertEquals(cc.getL("xixi"), "value");
		assertEquals(cc.getW("xixi"), "value");
		assertEquals(cc.getLW("xixi"), "value");
		assertEquals(lc.get(cc.getGroupID(), "xixi").getValue(), "value");
		assertEquals(lc.getCacheCount(), 1);
		assertEquals(lc.getCacheSize(), 1029);
		assertEquals(lc.getMaxCacheSize(), 1024 * 1024);
		lc.setWarningCacheRate(0.8);
		assertEquals(lc.getWarningCacheRate(), 0.8);
		
		mgr.shutdown();
	}
	
	public void testAddW2() throws InterruptedException {
		CacheClientManager mgr = CacheClientManager.getInstance("LocalCacheTest");
		String[] serverlist = servers.split(",");
		mgr.initialize(serverlist);
		mgr.enableLocalCache();
		Thread.sleep(50);
		LocalCache lc = mgr.getLocalCache();
		CacheClient cc = mgr.createClient();
		cc.flush();
		assertEquals(lc.getCacheCount(), 0);
		long ret = cc.addW("xixi", "value", 1);
		assertTrue(ret != 0);
		CacheItem item = cc.gets("xixi");
		assertEquals("value", item.getValue());
		assertEquals(lc.get(cc.getGroupID(), "xixi").getValue(), "value");
		assertEquals(cc.get("xixi"), "value");
		assertEquals(cc.getL("xixi"), "value");
		assertEquals(cc.getW("xixi"), "value");
		assertEquals(cc.getLW("xixi"), "value");
		assertEquals(lc.getCacheCount(), 1);
		assertEquals(lc.getCacheSize(), 1029);
		assertEquals(lc.getMaxCacheSize(), 64 * 1024 * 1024);
		lc.setMaxCacheSize(1024 * 1024);
		assertEquals(lc.getMaxCacheSize(), 1024 * 1024);
		lc.setWarningCacheRate(0.8);
		assertEquals(lc.getWarningCacheRate(), 0.8);
		ret = cc.addW("xixi", "value2", 1);
		assertTrue(ret == 0);
		item = cc.gets("xixi");
		assertEquals("value", item.getValue());
		assertEquals(lc.get(cc.getGroupID(), "xixi").getValue(), "value");
		assertEquals(cc.get("xixi"), "value");
		assertEquals(cc.getL("xixi"), "value");
		assertEquals(cc.getW("xixi"), "value");
		assertEquals(cc.getLW("xixi"), "value");
		assertEquals(lc.getCacheCount(), 1);
		assertEquals(lc.getCacheSize(), 1029);
		
		Thread.sleep(2000);
		
		assertNull(cc.get("xixi"));
		assertNull(lc.get(cc.getGroupID(), "xixi"));
		assertNull(cc.getL("xixi"));
		assertNull(cc.getW("xixi"));
		assertNull(cc.getLW("xixi"));
		assertEquals(lc.getCacheCount(), 0);
		assertEquals(lc.getCacheSize(), 0);
		assertEquals(lc.getMaxCacheSize(), 1024 * 1024);
		lc.setWarningCacheRate(0.8);
		assertEquals(lc.getWarningCacheRate(), 0.8);
		
		mgr.shutdown();
	}
	
	public void testReplaceW() throws InterruptedException {
		CacheClientManager mgr = CacheClientManager.getInstance("LocalCacheTest");
		String[] serverlist = servers.split(",");
		mgr.initialize(serverlist);
		mgr.enableLocalCache();
		Thread.sleep(50);
		LocalCache lc = mgr.getLocalCache();
		CacheClient cc = mgr.createClient();
		cc.flush();
		assertEquals(lc.getCacheCount(), 0);
		long ret = cc.replaceW("xixi", "value");
		assertEquals(0, ret);
		ret = cc.add("xixi", "1");
		assertTrue(ret != 0);
		ret = cc.replaceW("xixi", "value");
		assertTrue(ret != 0);
		assertEquals(lc.get(cc.getGroupID(), "xixi").getValue(), "value");
		assertEquals(cc.get("xixi"), "value");
		assertEquals(cc.getL("xixi"), "value");
		assertEquals(cc.getW("xixi"), "value");
		assertEquals(cc.getLW("xixi"), "value");
		assertEquals(lc.get(cc.getGroupID(), "xixi").getValue(), "value");
		assertEquals(lc.getCacheCount(), 1);
		assertEquals(lc.getCacheSize(), 1029);
		assertEquals(lc.getMaxCacheSize(), 64 * 1024 * 1024);
		lc.setMaxCacheSize(1024 * 1024);
		assertEquals(lc.getMaxCacheSize(), 1024 * 1024);
		lc.setWarningCacheRate(0.8);
		assertEquals(lc.getWarningCacheRate(), 0.8);
		
		Thread.sleep(2000);
		
		assertEquals(lc.get(cc.getGroupID(), "xixi").getValue(), "value");
		assertEquals(cc.get("xixi"), "value");
		assertEquals(cc.getL("xixi"), "value");
		assertEquals(cc.getW("xixi"), "value");
		assertEquals(cc.getLW("xixi"), "value");
		assertEquals(lc.get(cc.getGroupID(), "xixi").getValue(), "value");
		assertEquals(lc.getCacheCount(), 1);
		assertEquals(lc.getCacheSize(), 1029);
		assertEquals(lc.getMaxCacheSize(), 1024 * 1024);
		lc.setWarningCacheRate(0.8);
		assertEquals(lc.getWarningCacheRate(), 0.8);
		
		mgr.shutdown();
	}
	
	public void testReplaceW2() throws InterruptedException {
		CacheClientManager mgr = CacheClientManager.getInstance("LocalCacheTest");
		String[] serverlist = servers.split(",");
		mgr.initialize(serverlist);
		mgr.enableLocalCache();
		Thread.sleep(50);
		LocalCache lc = mgr.getLocalCache();
		CacheClient cc = mgr.createClient();
		cc.flush();
		assertEquals(lc.getCacheCount(), 0);
		long ret = cc.replaceW("xixi", "value", 1);
		assertEquals(0, ret);
		ret = cc.add("xixi", "1");
		assertTrue(ret != 0);
		ret = cc.replaceW("xixi", "value", 1);
		assertTrue(ret != 0);
		CacheItem item = cc.gets("xixi");
		assertEquals("value", item.getValue());
		assertEquals(lc.get(cc.getGroupID(), "xixi").getValue(), "value");
		assertEquals(cc.get("xixi"), "value");
		assertEquals(cc.getL("xixi"), "value");
		assertEquals(cc.getW("xixi"), "value");
		assertEquals(cc.getLW("xixi"), "value");
		assertEquals(lc.getCacheCount(), 1);
		assertEquals(lc.getCacheSize(), 1029);
		assertEquals(lc.getMaxCacheSize(), 64 * 1024 * 1024);
		lc.setMaxCacheSize(1024 * 1024);
		assertEquals(lc.getMaxCacheSize(), 1024 * 1024);
		lc.setWarningCacheRate(0.8);
		assertEquals(lc.getWarningCacheRate(), 0.8);
		ret = cc.addW("xixi", "value2", 1);
		assertTrue(ret == 0);
		item = cc.gets("xixi");
		assertEquals("value", item.getValue());
		assertEquals(lc.get(cc.getGroupID(), "xixi").getValue(), "value");
		assertEquals(cc.get("xixi"), "value");
		assertEquals(cc.getL("xixi"), "value");
		assertEquals(cc.getW("xixi"), "value");
		assertEquals(cc.getLW("xixi"), "value");
		assertEquals(lc.getCacheCount(), 1);
		assertEquals(lc.getCacheSize(), 1029);
		
		Thread.sleep(2000);
		
		assertNull(cc.get("xixi"));
		assertNull(lc.get(cc.getGroupID(), "xixi"));
		assertNull(cc.getL("xixi"));
		assertNull(cc.getW("xixi"));
		assertNull(cc.getLW("xixi"));
		assertEquals(lc.getCacheCount(), 0);
		assertEquals(lc.getCacheSize(), 0);
		assertEquals(lc.getMaxCacheSize(), 1024 * 1024);
		lc.setWarningCacheRate(0.8);
		assertEquals(lc.getWarningCacheRate(), 0.8);
		
		mgr.shutdown();
	}
	
	public void testReplaceW3() throws InterruptedException {
		CacheClientManager mgr = CacheClientManager.getInstance("LocalCacheTest");
		String[] serverlist = servers.split(",");
		mgr.initialize(serverlist);
		mgr.enableLocalCache();
		Thread.sleep(50);
		LocalCache lc = mgr.getLocalCache();
		CacheClient cc = mgr.createClient();
		cc.flush();
		assertEquals(lc.getCacheCount(), 0);
		long ret = cc.add("xixi", "value");
		assertTrue(ret != 0);
		ret = cc.replaceW("xixi", "value2", 1, 123);
		assertEquals(0, ret);
		CacheItem item = cc.gets("xixi");
		assertEquals("value", item.getValue());
		assertNull(lc.get(cc.getGroupID(), "xixi"));
		assertEquals(cc.get("xixi"), "value");
		assertEquals(cc.getL("xixi"), "value");
		assertEquals(cc.getW("xixi"), "value");
		assertEquals(cc.getLW("xixi"), "value");
		assertEquals(lc.getCacheCount(), 1);
		assertEquals(lc.getCacheSize(), 1029);
		assertEquals(lc.getMaxCacheSize(), 64 * 1024 * 1024);
		lc.setMaxCacheSize(1024 * 1024);
		assertEquals(lc.getMaxCacheSize(), 1024 * 1024);
		lc.setWarningCacheRate(0.8);
		assertEquals(lc.getWarningCacheRate(), 0.8);
		ret = cc.replaceW("xixi", "value2", 1, item.cacheID);
		assertTrue(ret != 0);
		item = cc.gets("xixi");
		assertEquals("value2", item.getValue());
		assertEquals(lc.get(cc.getGroupID(), "xixi").getValue(), "value2");
		assertEquals(cc.get("xixi"), "value2");
		assertEquals(cc.getL("xixi"), "value2");
		assertEquals(cc.getW("xixi"), "value2");
		assertEquals(cc.getLW("xixi"), "value2");
		assertEquals(lc.getCacheCount(), 1);
		assertEquals(lc.getCacheSize(), 1030);
		
		Thread.sleep(2000);
		
		assertNull(cc.get("xixi"));
		assertNull(lc.get(cc.getGroupID(), "xixi"));
		assertNull(cc.getL("xixi"));
		assertNull(cc.getW("xixi"));
		assertNull(cc.getLW("xixi"));
		assertEquals(lc.getCacheCount(), 0);
		assertEquals(lc.getCacheSize(), 0);
		assertEquals(lc.getMaxCacheSize(), 1024 * 1024);
		lc.setWarningCacheRate(0.8);
		assertEquals(lc.getWarningCacheRate(), 0.8);
		
		mgr.shutdown();
	}
}

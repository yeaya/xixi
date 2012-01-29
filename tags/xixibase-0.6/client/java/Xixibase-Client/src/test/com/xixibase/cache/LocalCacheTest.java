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
import java.util.Properties;

import com.xixibase.util.CurrentTick;

import junit.framework.TestCase;

public class LocalCacheTest extends TestCase {

	static String servers;
	static boolean enableSSL = false;
	static {
		servers = System.getProperty("hosts");
		enableSSL = System.getProperty("enableSSL") != null && System.getProperty("enableSSL").equals("true");
		if (servers == null) {
			try {
				InputStream in = new BufferedInputStream(new FileInputStream("test.properties"));
				Properties p = new Properties(); 
				p.load(in);
				in.close();
				servers = p.getProperty("hosts");
				enableSSL = p.getProperty("enableSSL") != null && p.getProperty("enableSSL").equals("true");
			} catch (IOException e) {
				e.printStackTrace();
			} 
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
		mgr.initialize(serverlist, enableSSL);
		mgr.enableLocalCache();
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
		assertEquals(cc.getsL("xixi").getValue(), "value");
		assertEquals(cc.getW("xixi"), "value");
		assertEquals(cc.getLW("xixi"), "value");
		assertEquals(lc.get(cc.getGroupID(), "xixi").getValue(), "value");
		assertEquals(lc.getCacheCount(), 1);
		assertEquals(525, lc.getCacheSize());
		assertEquals(lc.getMaxCacheSize(), 64 * 1024 * 1024);
		assertNull(lc.remove("errhost", cc.getGroupID(), "xixi"));
		assertNull(lc.remove(cc.getGroupID() + 1, "xixi"));
		assertEquals("value", lc.remove(cc.getGroupID(), "xixi").getValue());
		assertNull(lc.get(cc.getGroupID(), "xixi"));
		assertEquals(cc.getsLW("xixi").getValue(), "value");
		assertEquals(lc.get(cc.getGroupID(), "xixi").getValue(), "value");
		lc.setMaxCacheSize(1024 * 1024);
		assertEquals(lc.getMaxCacheSize(), 1024 * 1024);
		lc.setWarningCacheRate(0.8);
		assertEquals(lc.getWarningCacheRate(), 0.8);
		
		cc.setW("xixi2", "0315");
		assertEquals("0315", lc.remove(cc.getGroupID(), "xixi2").getValue());
		
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
		mgr.initialize(serverlist, enableSSL);
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
		assertEquals(525, lc.getCacheSize());
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
		mgr.initialize(serverlist, enableSSL);
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
		assertEquals(525, lc.getCacheSize());
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
		mgr.initialize(serverlist, enableSSL);
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
		assertEquals(525, lc.getCacheSize());
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
		assertEquals(525, lc.getCacheSize());
		assertEquals(lc.getMaxCacheSize(), 1024 * 1024);
		lc.setWarningCacheRate(0.8);
		assertEquals(lc.getWarningCacheRate(), 0.8);
		
		mgr.shutdown();
	}
	
	public void testAddW2() throws InterruptedException {
		CacheClientManager mgr = CacheClientManager.getInstance("LocalCacheTest");
		String[] serverlist = servers.split(",");
		mgr.initialize(serverlist, enableSSL);
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
		assertEquals(525, lc.getCacheSize());
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
		assertEquals(525, lc.getCacheSize());
		
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
		mgr.initialize(serverlist, enableSSL);
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
		assertEquals(525, lc.getCacheSize());
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
		assertEquals(525, lc.getCacheSize());
		assertEquals(lc.getMaxCacheSize(), 1024 * 1024);
		lc.setWarningCacheRate(0.8);
		assertEquals(lc.getWarningCacheRate(), 0.8);
		
		mgr.shutdown();
	}
	
	public void testReplaceW2() throws InterruptedException {
		CacheClientManager mgr = CacheClientManager.getInstance("LocalCacheTest");
		String[] serverlist = servers.split(",");
		mgr.initialize(serverlist, enableSSL);
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
		assertEquals(525, lc.getCacheSize());
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
		assertEquals(525, lc.getCacheSize());
		
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
		mgr.initialize(serverlist, enableSSL);
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
		assertEquals(525, lc.getCacheSize());
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
		assertEquals(526, lc.getCacheSize());
		
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
	
	public void testGetTouchL() throws InterruptedException {
		CacheClientManager mgr = CacheClientManager.getInstance("LocalCacheTest");
		String[] serverlist = servers.split(",");
		mgr.initialize(serverlist, enableSSL);
		mgr.enableLocalCache();
		Thread.sleep(50);
		LocalCache lc = mgr.getLocalCache();
		CacheClient cc = mgr.createClient();
		cc.flush();
		
		long beginTime = CurrentTick.get();
		cc.set("xixi", "session", 100);
		CacheItem item = cc.getsL("xixi");
		assertNotNull(item);
		long d = item.getExpireTime() - beginTime;
		assertEquals(100, d);

		Thread.sleep(2000);
		
		item = cc.gets("xixi");
		assertNotNull(item);
		d = item.getExpiration();
		assertEquals(98, d);
		
		item = cc.getAndTouchL("xixi", 100);
		assertNotNull(item);
		assertNull(lc.get(cc.getGroupID(), "xixi"));
		d = item.getExpiration();
		assertEquals(100, d);
	
		Thread.sleep(50);
		
		item = cc.gets("xixi");
		assertNotNull(item);
		d = item.getExpiration();
		assertTrue(d <= 100 && d >= 99);
		
		mgr.shutdown();
	}
	
	public void testGetTouchL2() throws InterruptedException {
		CacheClientManager mgr = CacheClientManager.getInstance("LocalCacheTest");
		String[] serverlist = servers.split(",");
		mgr.initialize(serverlist, enableSSL);
		mgr.enableLocalCache();
		Thread.sleep(50);
		LocalCache lc = mgr.getLocalCache();
		CacheClient cc = mgr.createClient();
		cc.flush();
		
		long beginTime = CurrentTick.get();
		cc.set("xixi", "session", 100);
		CacheItem item = cc.getsW("xixi");
		assertNotNull(item);
		long d = item.getExpireTime() - beginTime;
		assertEquals(100, d);

		Thread.sleep(2000);
		
		item = cc.gets("xixi");
		assertNotNull(item);
		d = item.getExpiration();
		assertEquals(98, d);
		
		item = cc.getAndTouchL("xixi", 100);
		assertNotNull(item);
		assertNotNull(lc.get(cc.getGroupID(), "xixi"));
		assertNull(lc.getAndTouch("errHost", cc.getGroupID(), "xixi", 1));
		d = item.getExpiration();
		assertEquals(100, d);
	
		Thread.sleep(50);
		
		item = cc.gets("xixi");
		assertNotNull(item);
		d = item.getExpiration();
		assertTrue(d <= 100 && d >= 98);
		
		mgr.shutdown();
	}
	
	public void testGetTouchW() throws InterruptedException {
		CacheClientManager mgr = CacheClientManager.getInstance("LocalCacheTest");
		String[] serverlist = servers.split(",");
		mgr.initialize(serverlist, enableSSL);
		mgr.enableLocalCache();
		Thread.sleep(50);
		LocalCache lc = mgr.getLocalCache();
		CacheClient cc = mgr.createClient();
		cc.flush();
		
		long beginTime = CurrentTick.get();
		cc.set("xixi", "session", 100);
		CacheItem item = cc.getsW("xixi");
		assertNotNull(item);
		long d = item.getExpireTime() - beginTime;
		assertEquals(100, d);

		Thread.sleep(2000);
		
		item = cc.gets("xixi");
		assertNotNull(item);
		d = item.getExpiration();
		assertEquals(98, d);
		
		item = cc.getAndTouchW("xixi", 100);
		assertNotNull(item);
		d = item.getExpiration();
		assertEquals(100, d);
		
		assertEquals(100, lc.get(cc.getGroupID(), "xixi").getExpiration());
	
		Thread.sleep(50);
		
		item = cc.gets("xixi");
		assertNotNull(item);
		d = item.getExpiration();
		assertTrue(d <= 100 && d >= 99);
		
		mgr.shutdown();
	}
	
	public void testGetTouchLW() throws InterruptedException {
		CacheClientManager mgr = CacheClientManager.getInstance("LocalCacheTest");
		String[] serverlist = servers.split(",");
		mgr.initialize(serverlist, enableSSL);
		mgr.enableLocalCache();
		Thread.sleep(50);
		LocalCache lc = mgr.getLocalCache();
		CacheClient cc = mgr.createClient();
		cc.flush();
		
		long beginTime = CurrentTick.get();
		cc.set("xixi", "session", 100);
		CacheItem item = cc.getsW("xixi");
		assertNotNull(item);
		long d = item.getExpireTime() - beginTime;
		assertEquals(100, d);

		Thread.sleep(2000);
		
		item = cc.gets("xixi");
		assertNotNull(item);
		d = item.getExpiration();
		assertEquals(98, d);
		
		item = cc.getAndTouchLW("xixi", 100);
		assertNotNull(item);
		d = item.getExpiration();
		assertEquals(100, d);
		
		assertEquals(100, lc.get(cc.getGroupID(), "xixi").getExpiration());
	
		Thread.sleep(50);
		
		item = cc.gets("xixi");
		assertNotNull(item);
		d = item.getExpiration();
		assertTrue(d <= 100 && d >= 99);
		
		mgr.shutdown();
	}
	
	public void testGetTouchLW2() throws InterruptedException {
		CacheClientManager mgr = CacheClientManager.getInstance("LocalCacheTest");
		String[] serverlist = servers.split(",");
		mgr.initialize(serverlist, enableSSL);
		mgr.enableLocalCache();
		Thread.sleep(50);
		LocalCache lc = mgr.getLocalCache();
		CacheClient cc = mgr.createClient();
		cc.flush();
		
		long beginTime = CurrentTick.get();
		cc.set("xixi", "session", 3);
		CacheItem item = cc.getsW("xixi");
		assertNotNull(item);
		long d = item.getExpireTime() - beginTime;
		assertEquals(3, d);

		Thread.sleep(2000);
		
		item = cc.gets("xixi");
		assertNotNull(item);
		d = item.getExpiration();
		assertEquals(1, d);
		
		item = cc.getAndTouchLW("xixi", 0);
		assertNotNull(item);
		d = item.getExpiration();
		assertEquals(0, d);
		
		assertEquals(0, lc.get(cc.getGroupID(), "xixi").getExpiration());
	
		Thread.sleep(50);
		
		item = cc.gets("xixi");
		assertNotNull(item);
		d = item.getExpiration();
		assertEquals(0, d);
	
		item = cc.getAndTouchLW("xixi", 2);
		assertNotNull(item);
		d = item.getExpiration();
		assertEquals(2, d);
		
		Thread.sleep(3000);
		d = item.getExpiration();
		assertEquals(-1, d);
		item = cc.gets("xixi");
		assertNull(item);
		
		mgr.shutdown();
	}
	
	public void testDropInactive() throws InterruptedException {
		CacheClientManager mgr = CacheClientManager.getInstance("testDropInactive");
		String[] serverlist = servers.split(",");
		String[] serverlist2 = new String[1];
		serverlist2[0] = serverlist[0];
		mgr.initialize(serverlist, enableSSL);
		mgr.enableLocalCache();
		mgr.getLocalCache().setMaxCacheSize(64 * 1024);
		mgr.getLocalCache().setWarningCacheRate(0.5);
		Thread.sleep(50);
		LocalCache lc = mgr.getLocalCache();
		CacheClient cc = mgr.createClient();
		cc.flush();
		
		cc.set("xixi", "0315");
		assertEquals("0315", cc.getsW("xixi").getValue());
		assertEquals("0315", cc.getsL("xixi").getValue());
		assertEquals(1, lc.getCacheCount());
		for (int i = 0; i < 100; i++) {
			cc.setW("xixi" + i, "0315");
		}
		assertEquals(39, lc.getCacheCount());

		mgr.shutdown();
	}
	
	public void testCacheUpdate() throws InterruptedException {
		CacheClientManager mgr = CacheClientManager.getInstance("testDropInactive");
		String[] serverlist = servers.split(",");
		mgr.initialize(serverlist, enableSSL);
		mgr.enableLocalCache();
		mgr.getLocalCache().setMaxCacheSize(64 * 1024);
		mgr.getLocalCache().setWarningCacheRate(0.6);
		Thread.sleep(50);
		CacheClient cc = mgr.createClient();
		cc.flush();
		
		long cacheID = cc.setW("xixi", "0315");
		assertTrue(cacheID != 0);
		cc.set("xixi", "20080315");
		CacheItem item = cc.getsW("xixi");
		assertTrue(item.getCacheID() != cacheID);
		cc.flush();

		cacheID = cc.setW("xixi", "0315");
		assertTrue(cacheID != 0);
		item = cc.getsL("xixi");
		cc.set("xixi", "20080315");
		item = cc.getsW("xixi");
		assertTrue(item.getCacheID() != cacheID);

		mgr.shutdown();
	}
}

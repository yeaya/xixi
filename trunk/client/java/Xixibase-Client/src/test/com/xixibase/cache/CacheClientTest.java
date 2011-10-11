package com.xixibase.cache;

import java.io.IOException;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.lang.Integer;

import com.xixibase.util.CurrentTick;

import junit.framework.TestCase;

public class CacheClientTest extends TestCase {
	private static final String managerName1 = "manager1";
	private static CacheClientManager mgr1 = null;
	private static CacheClient cc1 = null;
	
	static String servers;
	static String[] serverlist;
	static {
		servers = System.getProperty("hosts");
		if (servers == null) {
			servers = "localhost:7788";
		}
		serverlist = servers.split(",");

		mgr1 = CacheClientManager.getInstance(managerName1);
		mgr1.setSocketWriteBufferSize(64 * 1024);
		mgr1.initialize(serverlist);
		mgr1.enableLocalCache();
	}

	protected void setUp() throws Exception {
		super.setUp();
		CacheClientManager mgr1 = CacheClientManager.getInstance(managerName1);
		cc1 = mgr1.createClient();
	}

	protected void tearDown() throws Exception {
		super.tearDown();
		assertNotNull(cc1);
		cc1.flush();
	}

	public void testFlush() {
		int groupID = cc1.getGroupID();
		cc1.setGroupID(3);
		cc1.set("xixi1", "bar1");
		cc1.setGroupID(15);
		cc1.set("xixi2", "bar2");
		cc1.setGroupID(3);
		int count = cc1.flush();
		assertEquals(1, count);
		cc1.setGroupID(15);
		count = cc1.flush();
		assertEquals(1, count);
		cc1.setGroupID(3);
		assertFalse(cc1.keyExists("xixi1"));
		cc1.setGroupID(15);
		assertFalse(cc1.keyExists("xixi2"));
		cc1.setGroupID(groupID);
	}
	
	public void testFlush2() {
		assertTrue(serverlist.length > 1);
		ArrayList<String> keys = new ArrayList<String>();
		ArrayList<String> values = new ArrayList<String>();
		for (int i = 0; i < 100; i++) {
			String key = "xixi" + i;
			String value = "value" + i;
			keys.add(key);
			values.add(value);
			cc1.set(key, value);
		}
		int lastCount = 0;
		for (int i = 0; i < serverlist.length; i++) {
			String[] sl = new String[1];
			sl[0] = serverlist[i];
			int flushCount = cc1.flush(sl);
			int count = 0;
			for (int j = 0; j < 100; j++) {
				if (cc1.get("xixi" + j) == null) {
					count++;				
				}
			}
			assertEquals(flushCount, count - lastCount);
			assertTrue(count > 0);
			assertTrue(count > lastCount);
			lastCount = count;
		}
		assertEquals(100, lastCount);
	}
	
	public void testGetError() {
		assertNull(cc1.get(null));
		long ret = cc1.set("xixi", "value");
		assertTrue(ret != 0);
		assertEquals("value", cc1.get("xixi"));
		assertNull(cc1.get("xixi2"));
		String cn = cc1.getTransCoder().getEncodingCharsetName();
		cc1.getTransCoder().setSanitizeKeys(true);
		cc1.getTransCoder().setEncodingCharsetName("errorCharsetName");
		assertNull(cc1.get("xixi"));
		mgr1.shutdown();
		cc1.getTransCoder().setEncodingCharsetName(cn);
		assertNull(cc1.get("xixi"));
		
		mgr1 = CacheClientManager.getInstance(managerName1);
		mgr1.setSocketWriteBufferSize(64 * 1024);
		mgr1.initialize(serverlist);
		mgr1.enableLocalCache();
	}
	
	public void testGroup() {
		int groupID = cc1.getGroupID();
		cc1.setGroupID(3);
		cc1.set("xixi", "group3");
		cc1.setGroupID(15);
		cc1.set("xixi", "group15");

		cc1.setGroupID(3);
		assertEquals("group3", cc1.get("xixi"));
		cc1.setGroupID(15);
		assertEquals("group15", cc1.get("xixi"));
		
		cc1.setGroupID(3);
		int count = cc1.flush();
		assertEquals(1, count);
		cc1.setGroupID(15);
		count = cc1.flush();
		assertEquals(1, count);
		
		cc1.setGroupID(groupID);
		
		assertNull(cc1.getLastError());
	}

	public void testObjectTransCoderEncodeIntLong() {
		int oint = 0x12345678;
		byte[] b = ObjectTransCoder.encodeInt(oint);
		int intValue = ObjectTransCoder.decodeInt(b);
		assertEquals(intValue, oint);
		
		long olong = 0x1234567890123456L;
		b = ObjectTransCoder.encodeLong(olong);
		long longValue = ObjectTransCoder.decodeLong(b);
		assertEquals(longValue, olong);
	}
	
	public void testTransCoder() {
		TransCoder oldCoder = cc1.getTransCoder();
		TransCoder coder = new ObjectTransCoder() {
			@Override
			public byte[] encode(final Object object, int[] flags) throws IOException {
				return super.encode(object, flags);
			}
		};
		cc1.setTransCoder(coder);
		cc1.set("xixi", "testTransCoder");
		assertEquals("testTransCoder", cc1.get("xixi"));
		cc1.setTransCoder(oldCoder);
	}

	public void testDelete() {
		cc1.set("xixi", Boolean.TRUE);
		Boolean b = (Boolean) cc1.get("xixi");
		assertEquals(b.booleanValue(), true);
		cc1.delete("xixi");
		assertEquals(null, cc1.get("xixi"));
		assertFalse(cc1.delete(null));
	}

	public void testDelete2() {
		cc1.set("xixi", new Long(2008));
		CacheItem item = cc1.gets("xixi");
		assertEquals(item.getValue(), new Long(2008));
		assertTrue(item.cacheID != 0);
		boolean ret = cc1.delete("xixi", item.cacheID + 1);
		assertFalse(ret);
		ret = cc1.delete("xixi", item.cacheID);
		assertTrue(ret);
		assertEquals(null, cc1.get("xixi"));
		assertFalse(cc1.delete(null));
	}
	
	public void testSetBoolean() {
		cc1.set("xixi", Boolean.TRUE);
		Boolean b = (Boolean) cc1.get("xixi");
		assertEquals(b.booleanValue(), true);
	}

	public void testSetIntegers() {
		int[] value = new int[5];
		for (int i = 0; i < value.length; i++) {
			value[i] = i;
		}
		cc1.set("xixi", value);
		int[] value2 = (int[]) cc1.get("xixi");
		assertEquals(value.length, value2.length);
		for (int i = 0; i < value.length; i++) {
			assertEquals(value[i], value2[i]);
		}
	}
	
	public void testSetInteger() {
		cc1.set("xixi", new Integer(Integer.MAX_VALUE));
		Integer i = (Integer) cc1.get("xixi");
		assertEquals(i.intValue(), Integer.MAX_VALUE);
	}
	
	public void testSetDate() {
		Date d1 = new Date();
		cc1.set("xixi", d1);
		Date d2 = (Date) cc1.get("xixi");
		assertEquals(d1, d2);
	}
	
	public void testSetString() {
		String value = "test of string encoding";
		CacheClientManager mgr = CacheClientManager.getInstance("test1");
		mgr.setNagle(false);
		mgr.initialize(serverlist, null,
				new XixiWeightMap<Integer>(false, XixiWeightMap.CRC32_HASH));
		CacheClient cc = new CacheClient("test1");
		cc.set("xixi", value);
		String s = (String) cc.get("xixi");
		assertEquals(s, value);
		cc.set("xixi1", value);
		s = (String) cc.get("xixi1");
		assertEquals(s, value);
		mgr.shutdown();
		mgr = CacheClientManager.getInstance("test2");
		mgr.setNagle(false);
		mgr.initialize(serverlist, null,
				new XixiWeightMap<Integer>(false, XixiWeightMap.MD5_HASH));
		cc = new CacheClient("test2");
		cc.set("xixi", value);
		s = (String) cc.get("xixi");
		assertEquals(s, value);
		cc.set("xixi2", value);
		s = (String) cc.get("xixi2");
		assertEquals(s, value);
		mgr.shutdown();
	}
	
	public void testCacheItem() {
		long cacheID = cc1.add("xixi", "ke-ai");
		assertTrue(cacheID != 0);
		CacheItem item = cc1.gets("xixi");
		assertEquals(item.getCacheID(), cacheID);
		long time = item.getExpiration();
		assertEquals(0, time);
		assertTrue(item.getFlags() > 0);
		assertEquals(item.getGroupID(), 0);
		assertEquals(item.getOption1(), 0);
		assertEquals(item.getOption2(), 0);
		assertEquals(item.getValue(), "ke-ai");
	}
	
	public void testGetBase() {
		cc1.getTransCoder().setOption1((short)0x1234);
		cc1.getTransCoder().setOption2((byte)0xF1);
		cc1.set("xixi", "base");
		CacheBaseItem item = cc1.getBase("xixi");
		assertTrue(item.cacheID != 0);
		assertEquals((short)0x1234, item.getOption1());
		assertEquals((byte)0xF1, item.getOption2());
		cc1.delete("xixi");
		assertNull(cc1.getBase("xixi"));
	}
	
	public void testKeyExsits() {
		cc1.set("xixi", "base");
		assertTrue(cc1.keyExists("xixi"));
		cc1.delete("xixi");
		assertFalse(cc1.keyExists("xixi"));
	}

	public void testWeightMap() {
		String input = "test of string encoding";
		cc1.set("xixi", input);
		String s = (String) cc1.get("xixi");
		assertEquals(s, input);

		CacheClientManager mgr = CacheClientManager.getInstance("test1");
		mgr.setNagle(false);
		mgr.initialize(serverlist, null,
				new XixiWeightMap<Integer>(false, XixiWeightMap.CRC32_HASH));
		cc1 = new CacheClient("test1");
		cc1.set("xixi", input);
		s = (String) cc1.get("xixi");
		assertEquals(s, input);
		cc1.set("xixi1", input);
		s = (String) cc1.get("xixi1");
		assertEquals(s, input);
		mgr.shutdown();
		
		mgr = CacheClientManager.getInstance("test2");
		mgr.setNagle(false);
		mgr.initialize(serverlist, null,
				new XixiWeightMap<Integer>(true, XixiWeightMap.NATIVE_HASH));
		cc1 = new CacheClient("test2");
		cc1.set("xixi", input);
		s = (String) cc1.get("xixi");
		assertEquals(s, input);
		cc1.set("xixi2", input, 10);
		s = (String) cc1.get("xixi2");
		assertEquals(s, input);
		mgr.shutdown();
		
		mgr = CacheClientManager.getInstance("test3");
		mgr.setNagle(false);
		Integer[] weights = new Integer[2];
		weights[0] = new Integer(2);
		weights[1] = new Integer(8);
		mgr.initialize(serverlist, weights,
				new XixiWeightMap<Integer>(true, XixiWeightMap.MD5_HASH));
		cc1 = new CacheClient("test3");
		cc1.set("xixi", input);
		s = (String) cc1.get("xixi");
		assertEquals(s, input);
		cc1.set("xixi2", input, 10);
		s = (String) cc1.get("xixi2");
		assertEquals(s, input);
		mgr.shutdown();
	}

	public void testSetChar() {
		cc1.set("xixi", new Character('Y'));
		Character c = (Character) cc1.get("xixi");
		assertEquals(c.charValue(), 'Y');
	}

	public void testSetByte() {
		cc1.set("xixi", new Byte((byte) 127));
		Byte b = (Byte) cc1.get("xixi");
		assertEquals(b.byteValue(), (byte) 127);
	}

	public void testSetStringBuffer() {
		cc1.set("xixi", new StringBuffer("hello"));
		StringBuffer o = (StringBuffer) cc1.get("xixi");
		assertEquals(o.toString(), "hello");
	}
	
	public void testSetStringBuilder() {
		cc1.set("xixi", new StringBuilder("hello"));
		StringBuilder o = (StringBuilder) cc1.get("xixi");
		assertEquals(o.toString(), "hello");
	}
	
	public void testGetCacheItem() {
		cc1.set("xixi", new StringBuilder("hello"));
		CacheItem item = cc1.gets("xixi");
		assertNotNull(item);
		assertTrue(item.cacheID != 0);
		assertEquals(item.value.toString(), "hello");
	}

	public void testSetShort() {
		cc1.set("xixi", new Short((short) 100));
		Short o = (Short) cc1.get("xixi");
		assertEquals(o.shortValue(), (short) 100);
	}

	public void testSetLong() {
		cc1.set("xixi", new Long(Long.MAX_VALUE));
		Long o = (Long) cc1.get("xixi");
		assertEquals(o.longValue(), Long.MAX_VALUE);
	}

	public void testSetDouble() {
		cc1.set("xixi", new Double(1.1));
		Double o = (Double) cc1.get("xixi");
		assertEquals(o.doubleValue(), 1.1);
	}

	public void testSetFloat() {
		cc1.set("xixi", new Float(1.1f));
		Float o = (Float) cc1.get("xixi");
		assertEquals(o.floatValue(), 1.1f);
	}

	public void testSetByteArray() {
		byte[] b = new byte[10];
		for (int i = 0; i < 10; i++)
			b[i] = (byte) i;

		cc1.set("xixi", b);
		assertTrue(Arrays.equals((byte[]) cc1.get("xixi"), b));
	}

	public void testSetObj() {
		SerialItem item = new SerialItem("xixi", 0);
		cc1.set("xixi", item);
		SerialItem item2 = (SerialItem) cc1.get("xixi");
		assertEquals(item, item2);
	}

	public void testSetStringObjectDateInteger() throws InterruptedException {
		String expected, actual;
		cc1.set("xixi", "bar", 1);
		expected = "bar";
		actual = (String) cc1.get("xixi");
		assertEquals(expected, actual);

		Thread.sleep(2000);

		boolean res = cc1.keyExists("xixi");
		assertFalse(res);
	}

	public void testSetStringObjectInteger() {
		String expected, actual;
		cc1.set("xixi", "bar");
		expected = "bar";
		actual = (String) cc1.get("xixi");
		assertEquals(expected, actual);
	}
	
	public void testSetExp() throws InterruptedException {
		cc1.set("xixi", new Integer(100), 1);

		Thread.sleep(2000);

		assertNull(cc1.get("xixi"));
	}

	public void testGetTouch() throws InterruptedException {
		long beginTime = CurrentTick.get();
		cc1.set("xixi", "session", 100);
		CacheItem item = cc1.gets("xixi");
		assertNotNull(item);
		long d = item.getExpireTime() - beginTime;
		assertEquals(d, 100);

		Thread.sleep(2000);

		item = cc1.gets("xixi");
		assertNotNull(item);
		d = item.getExpiration();
		assertEquals(d, 98);
		
		item = cc1.getAndTouch("xixi", 100);
		assertNotNull(item);
		d = item.getExpiration();
		assertEquals(d, 100);
		
		item = cc1.gets("xixi");
		assertNotNull(item);
		d = item.getExpiration();
		assertEquals(d, 100);
		assertNull(cc1.getLastError());
		
		item = cc1.getAndTouch("xixi", Defines.NO_EXPIRATION);
		assertNotNull(item);
		d = item.getExpiration();
		assertEquals(0, d);
		
		item = cc1.gets("xixi");
		assertNotNull(item);
		d = item.getExpiration();
		assertEquals(0, d);
		assertNull(cc1.getLastError());
	}

	public void testGetTouchL() throws InterruptedException {
		long beginTime = CurrentTick.get();
		cc1.set("xixi", "session", 100);
		CacheItem item = cc1.getsW("xixi");
		assertNotNull(item);
		long d = item.getExpireTime() - beginTime;
		assertEquals(100, d);

		Thread.sleep(2000);
		
		item = cc1.gets("xixi");
		assertNotNull(item);
		d = item.getExpiration();
		assertEquals(98, d);
		
		item = cc1.getAndTouchL("xixi", 100);
		assertNotNull(item);
		d = item.getExpiration();
		assertEquals(100, d);
	
		Thread.sleep(50);
		
		item = cc1.gets("xixi");
		assertNotNull(item);
		d = item.getExpiration();
		assertTrue(d <= 100 && d >= 99);
	}

	public void testDelta() {
		assertNull(cc1.incr(null));
		long ret = cc1.createDelta("xixi", 0);
		assertTrue(ret != 0);
		assertEquals(cc1.get("xixi"), new Long(0).toString());
		DeltaItem ditem = cc1.incr("xixi");
		ditem = cc1.incr("xixi", 5L);
		ditem = cc1.decr("xixi", 2L);
		assertEquals(ditem.value, 4);
		ditem = cc1.decr("xixi");
		assertEquals(ditem.value, 3);
		cc1.setDelta("xixi", 9);
		ditem = cc1.incr("xixi", 0L);
		assertEquals(9, ditem.value);
	}

	public void testDelta2() throws InterruptedException {
		long ret = cc1.createDelta("xixi", 0, 1);
		assertTrue(ret != 0);
		assertEquals(cc1.get("xixi"), new Long(0).toString());
		DeltaItem ditem = cc1.incr("xixi");
		ditem = cc1.incr("xixi", 5L);
		ditem = cc1.decr("xixi", 2L);
		assertEquals(4, ditem.value);
		
		Thread.sleep(2000);
		
		ditem = cc1.incr("xixi", 0L);
		assertNull(ditem);
		
		cc1.setDelta("xixi", 9, 1);
		ditem = cc1.incr("xixi", 0L);
		assertEquals(9, ditem.value);
		
		Thread.sleep(2000);
		
		ditem = cc1.incr("xixi", 0L);
		assertNull(ditem);
	}

	public void testDelta3() throws InterruptedException {
		long ret = cc1.createDelta("xixi", 0, 1);
		assertTrue(ret != 0);
		assertEquals(cc1.get("xixi"), new Long(0).toString());
		DeltaItem ditem = cc1.incr("xixi");
		ditem = cc1.incr("xixi", 5L);
		ditem = cc1.decr("xixi", 2L);
		assertEquals(ditem.value, 4L);
		assertTrue(ditem.cacheID != 0);
		
		assertNull(cc1.incr("xixi", 8, 123));
		ditem = cc1.incr("xixi", 8, ditem.cacheID);
		assertEquals(12, ditem.value);
		assertTrue(ditem.cacheID != 0);

		assertNull(cc1.decr("xixi", 7, 123));
		ditem = cc1.decr("xixi", 7, ditem.cacheID);
		assertEquals(5, ditem.value);
		assertTrue(ditem.cacheID != 0);
		
		ret = cc1.setDelta("xixi", 9, 1, 123);
		assertEquals(0, ret);
		CacheItem item = cc1.gets("xixi");
		ret = cc1.setDelta("xixi", 9, 1, item.getCacheID());
		assertTrue(ret != 0);
		
		ditem = cc1.incr("xixi", 0L);
		assertEquals(9, ditem.value);
		
		Thread.sleep(2000);
		
		ditem = cc1.incr("xixi", 0L);
		assertNull(ditem);
	}
	
	public void testAdd() {
		cc1.delete("xixi");
		assertEquals(null, cc1.get("xixi"));
		cc1.set("xixi", "bar");
		String tt = (String) cc1.get("xixi");
		assertEquals("bar", tt);
		cc1.add("xixi", "bar2");
		String tt2 = (String) cc1.get("xixi");
		assertEquals("bar", tt2);
		assertEquals(cc1.add(null, "bar2"), 0);
		assertEquals(cc1.add("xixi", null), 0);
	}

	public void testAddStringObjectDate() throws InterruptedException {
		String expected, actual;
		cc1.add("xixi", "bar", 1);
		actual = (String) cc1.get("xixi");
		expected = "bar";
		assertEquals(expected, actual);

		Thread.sleep(2000);

		assertFalse(cc1.keyExists("xixi"));
	}

	public void testAddStringObjectDateInteger() throws InterruptedException {
		String expected, actual;
		cc1.add("xixi", "bar", 1);
		actual = (String) cc1.get("xixi");
		expected = "bar";
		assertEquals(expected, actual);

		Thread.sleep(2000);

		actual = (String) cc1.get("xixi");
		assertNull(actual);
	}

	public void testReplaceStringObject() {
		String expected, actual;
		cc1.set("xixi", "bar1");
		cc1.replace("xixi", "bar2");
		expected = "bar2";
		actual = (String) cc1.get("xixi");
		assertEquals(expected, actual);
	}

	public void testReplaceStringObjectDate() throws InterruptedException {
		String expected, actual;
		cc1.set("xixi", "bar1");
		cc1.replace("xixi", "bar2", 1);
		expected = "bar2";
		actual = (String) cc1.get("xixi");
		assertEquals(expected, actual);

		Thread.sleep(2000);

		assertFalse(cc1.keyExists("xixi"));
	}

	public void testReplaceStringObjectDateInteger() throws InterruptedException {
		String expected, actual;
		cc1.set("xixi", "bar1");
		cc1.replace("xixi", "bar2", 1);
		expected = "bar2";
		actual = (String) cc1.get("xixi");
		assertEquals(expected, actual);

		Thread.sleep(2000);

		assertNull(cc1.get("xixi"));
	}

	public void testReplaceStringObjectInteger() {
		String expected, actual;
		cc1.set("xixi", "bar1", CacheClient.NO_EXPIRATION);
		cc1.replace("xixi", "bar2", CacheClient.NO_EXPIRATION, CacheClient.NO_CAS);
		expected = "bar2";
		actual = (String) cc1.get("xixi");
		assertEquals(expected, actual);
	}

	public void testAppend() {
		String value = "aa";
		cc1.append("aa", value);
		assertEquals(0, cc1.append(null, value));
		assertEquals(0, cc1.append("aa", null));
		assertEquals(cc1.get("aa"), null);
		cc1.add("aa", value);
		assertEquals(cc1.get("aa"), value);
		assertNotSame(cc1.append("aa", "bb"), 0);
		assertEquals(cc1.get("aa"), value + "bb");
	}

	public void testAppendStringObjectInteger() {
		String actual, expected;
		cc1.add("xixi", "abc");
		actual = (String) cc1.get("xixi");
		expected = "abc";
		assertEquals(expected, actual);

		cc1.append("xixi", "def");
		actual = (String) cc1.get("xixi");
		expected = "abcdef";
		assertEquals(expected, actual);
	}

	public void testPrepend() {
		String value = "aa";
		cc1.prepend("aa", value);
		assertEquals(cc1.get("aa"), null);
		cc1.add("aa", value);
		assertEquals(cc1.get("aa"), value);
		long ret = cc1.prepend("aa", "bb");
		assertTrue(ret != 0);
		assertEquals(cc1.get("aa"), "bb" + value);
	}

	public void testPrependStringObjectInteger() {
		String expected, actual;
		cc1.set("xixi", "def");
		cc1.prepend("xixi", "abc");
		expected = "abcdef";
		actual = (String) cc1.get("xixi");
		assertEquals(expected, actual);
	}

	public void testSetWithVersion() {
		String value = "aa";
		cc1.set("aa", value);
		CacheItem item = cc1.gets("aa");
		assertEquals(value, item.getValue());
		
		cc1.set("aa", "bb", CacheClient.NO_EXPIRATION, item.getCacheID());
		item = cc1.gets("aa");
		assertEquals("bb", item.getValue());
		cc1.set("aa", "cc1");
		assertEquals("cc1", cc1.get("aa"));
		cc1.set("aa", "dd", CacheClient.NO_EXPIRATION, item.getCacheID());
		assertEquals("cc1", cc1.get("aa"));
	}

	public void testStringObjectIntegerLongWithVersion() {
		String expected, actual;
		long ret = cc1.set("xixi", "bar");
		assertTrue(ret != 0);
		CacheItem item = cc1.gets("xixi");
		expected = "bar";
		actual = (String) item.getValue();
		assertEquals(expected, actual);

		ret = cc1.set("xixi", "bar1", CacheClient.NO_EXPIRATION, item.getCacheID());
		expected = "bar1";
		actual = (String) cc1.get("xixi");
		assertEquals(expected, actual);

		ret = cc1.set("xixi", "bar2");
		expected = "bar2";
		actual = (String) cc1.get("xixi");
		assertEquals(expected, actual);

		ret = cc1.set("xixi", "bar3", CacheClient.NO_EXPIRATION, item.getCacheID());
		assertTrue(ret == 0);
	}

	public void testStringObjectDateLongWithVersion() throws InterruptedException {
		String expected, actual;
		cc1.set("xixi", "bar");
		CacheItem item = cc1.gets("xixi");
		expected = "bar";
		actual = (String) item.getValue();
		assertEquals(expected, actual);

		cc1.set("xixi", "bar1", 1, item.getCacheID());
		expected = "bar1";
		actual = (String) cc1.get("xixi");
		assertEquals(expected, actual);

		Thread.sleep(2000);
		
		assertNull(cc1.get("xixi"));

		cc1.set("xixi", "bar2");
		expected = "bar2";
		actual = (String) cc1.get("xixi");
		assertEquals(expected, actual);

		long ret = cc1.set("xixi", "bar3", 1, item.getCacheID());
		assertTrue(ret == 0);
	}

	public void testStringObjectDateIntegerLongWithVersion() throws InterruptedException {
		String expected, actual;
		cc1.set("xixi", "bar");
		CacheItem item = cc1.gets("xixi");
		expected = "bar";
		actual = (String) item.getValue();
		assertEquals(expected, actual);

		cc1.set("xixi", "bar1", 1, item.getCacheID());
		expected = "bar1";
		actual = (String) cc1.get("xixi");
		assertEquals(expected, actual);

		Thread.sleep(2000);

		actual = (String) cc1.get("xixi");
		assertNull(actual);

		cc1.set("xixi", "bar2");
		expected = "bar2";
		actual = (String) cc1.get("xixi");
		assertEquals(expected, actual);

		long ret = cc1.set("xixi", "bar3", 1, item.getCacheID());
		assertTrue(ret == 0);
	}

	public void testBigData() {
		cc1.getTransCoder().setCompressionThreshold(0x7FFFFFFF);
		SerialItem item = new SerialItem("testBigData", 10240);
		for (int i = 0; i < 10; ++i) {
			cc1.set("xixi" + i, item);
			SerialItem item2 = (SerialItem)cc1.get("xixi" + i);
			assertEquals(item, item2);
		}
		item = new SerialItem("testBigData2", 1024 * 1024 * 2);
		long ret = cc1.set("xixi", item);
		assertTrue(ret != 0);
		SerialItem item2 = (SerialItem)cc1.get("xixi");
		assertEquals(item, item2);
	}

	public static final class SerialItem implements Serializable {
		private static final long serialVersionUID = -1331512331067654578L;
		
		private String name;
		private byte[] buffer;
//		private HashMap<String, Integer> map = new HashMap<String, Integer>();

		public SerialItem(String name, int size) {
			this.name = name;
			buffer = new byte[size];
			for (int i = 0; i < size; i++) {
				buffer[i] = (byte)(i & 0xFF);
			}
		}

		public String getName() {
			return name;
		}

		public boolean equals(Object obj) {
			if (this == obj) {
				return true;
			}
			if (!(obj instanceof SerialItem)) {
				return false;
			}
			SerialItem item = (SerialItem)obj;
			if (!name.equals(item.getName())) {
				return false;
			}
			if (buffer.length != item.buffer.length) {
				return false;
			}
			for (int i = 0; i < buffer.length; i++) {
				if (buffer[i] != item.buffer[i]) {
					return false;
				}
			}
			return true;
		}
	}
	
	public void testStatsAddGroup() {
		boolean ret = cc1.statsAddGroup(null, 315);
		assertTrue(ret);
		ret = cc1.statsRemoveGroup(null, 315);
		assertTrue(ret);
	}
	
	public void testBench() {
		CacheClientBench ccb = new CacheClientBench(servers, 1, 100);
		assertTrue(ccb.runIt());
	}
}

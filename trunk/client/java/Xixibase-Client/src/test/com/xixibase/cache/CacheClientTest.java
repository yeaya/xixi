package com.xixibase.cache;

import java.io.IOException;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.lang.Integer;

import com.xixibase.util.CurrentTick;

import junit.framework.TestCase;

public class CacheClientTest extends TestCase {
	private static final String managerName1 = "manager1";
	private static final String managerName2 = "manager2";
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

		CacheClientManager mgr2 = CacheClientManager.getInstance(managerName2);
		mgr2.setSocketWriteBufferSize(64 * 1024);
		mgr2.initialize(serverlist);
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

	public void testSetString() {
		String value = "test of string encoding";
		String[] serverlist = servers.split(",");

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
		assertTrue(time < 0xFFFFFFFFL);
		assertTrue(time > 0xEFFFFFFFL);
		assertTrue(item.getFlags() > 0);
		assertEquals(item.getGroupID(), 0);
		assertEquals(item.getOption1(), 0);
		assertEquals(item.getOption2(), 0);
		assertEquals(item.getValue(), "ke-ai");
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
		String[] serverlist = servers.split(",");

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
	}

	public void testGetTouchL() throws InterruptedException {
		long beginTime = CurrentTick.get();
		cc1.set("xixi", "session", 100);
		CacheItem item = cc1.getsW("xixi");
		assertNotNull(item);
		long d = item.getExpireTime() - beginTime;
		assertEquals(100, d);
		try {
			Thread.sleep(2000);
		} catch (Exception ex) {
		}
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
		assertEquals(100, d);
	}

	public void testIncr() {
		assertNull(cc1.incr(null));
		long i = 0;
		long ret = cc1.addOrIncr("xixi", i); // now == 0
		assertEquals(ret, 0);
		assertEquals(cc1.get("xixi"), new Long(i).toString());
		ret = cc1.incr("xixi"); // xixi now == 1
		ret = cc1.incr("xixi", (long) 5); // xixi now == 6
		long j = cc1.decr("xixi", (long) 2); // xixi now == 4
		assertEquals(j, 4);
	}

	public void testIncrStringLongInteger() {
		long expected, actual;
		cc1.addOrIncr("xixi", 1);
		actual = cc1.incr("xixi", 5);
		expected = 6;
		assertEquals(expected, actual);
	}

	public void testDecrString() {
		long expected, actual;
		actual = cc1.addOrIncr("xixi");
		cc1.incr("xixi", 5);

		expected = 4;
		actual = cc1.decr("xixi");
		assertEquals(expected, actual);
	}

	public void testDecrStringLongInteger() {
		long expected, actual;
		actual = cc1.addOrIncr("xixi", 1);
		cc1.incr("xixi", 5);

		expected = 3;
		actual = cc1.decr("xixi", 3);
		assertEquals(expected, actual);
	}

	public void testSetDate() {
		Date d1 = new Date();
		cc1.set("xixi", d1);
		Date d2 = (Date) cc1.get("xixi");
		assertEquals(d1, d2);
	}

	public void testAddOrIncr() {
		long j;
		j = cc1.addOrIncr("xixi"); // xixi now == 0
		assertEquals(0, j);
		j = cc1.incr("xixi"); // xixi now == 1
		j = cc1.incr("xixi", (long) 5); // xixi now == 6

		j = cc1.addOrIncr("xixi", 1); // xixi now 7

		j = cc1.decr("xixi", (long) 3); // xixi now == 4
		assertEquals(4, j);
	}

	public void testAddOrDecrString() {

		long expected, actual;
		actual = cc1.addOrDecr("xixi");
		expected = 0;
		assertEquals(expected, actual);

		expected = 5;
		actual = cc1.addOrIncr("xixi", 5);
		assertEquals(expected, actual);

		actual = cc1.addOrDecr("xixi");
		expected = 5;
		assertEquals(expected, actual);
	}

	public void testAddOrDecrStringLong() {

		long expected, actual;
		actual = cc1.addOrDecr("xixi", 2);
		expected = 2;
		assertEquals(expected, actual);

		expected = 7;
		actual = cc1.addOrIncr("xixi", 5);
		assertEquals(expected, actual);

		actual = cc1.addOrDecr("xixi", 3);
		expected = 4;
		assertEquals(expected, actual);
	}

	public void testAddOrDecrStringLongInteger() {

		long expected, actual;
		int hashcode = 10;
		actual = cc1.addOrDecr("xixi", 2, hashcode);
		expected = 2;
		assertEquals(expected, actual);

		expected = 7;
		actual = cc1.addOrIncr("xixi", 5, hashcode);
		assertEquals(expected, actual, hashcode);

		actual = cc1.addOrDecr("xixi", 3, hashcode);
		expected = 4;
		assertEquals(expected, actual);
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

	public void testMultiKey() {

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
			assertEquals(item, cc1.get("xixi" + i));
		}
		item = new SerialItem("testBigData2", 1024 * 1024 * 2);
		long ret = cc1.set("xixi", item);
		assertTrue(ret != 0);
		assertEquals(item, cc1.get("xixi"));
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
			if (name.equals(item.getName())) {
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
}

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
import java.util.Properties;

import org.apache.log4j.BasicConfigurator;

interface Runable {
	public boolean run();
}

class TestWork extends Thread {
	
	ArrayList<Runable> list_ = new ArrayList<Runable>();
	TestWork(ArrayList<Runable> list) {
		list_.addAll(list);
	}
	
	public void run() { 
		while (list_.size() > 0) {
			for (int i = 0; i < list_.size(); i++) {
				Runable r = (Runable)list_.get(i);
				if (!r.run()) {
					list_.remove(i);
					break;
				}
			}
		}
	} 
}

class TestCase1 implements Runable {
	CacheClient cc = null;
	long id = 0;
	int keyCount = 0; 
	int maxSetCount = 0;
	int maxGetCount = 0;
	
	int cachepos = 0;

	static long index = 0;
	static long totalset = 0;
	static long totalget = 0;
	static long setpersec = 0;
	static long getpersec = 0;
	static long lasttotalreporttime = 0;
	
	String key = "key";
	String data = "data";
	public static void report(boolean flags, int setcount, int getcount) {
		synchronized(TestCase1.class) {
			totalset += setcount;
			totalget += getcount;
			setpersec += setcount;
			getpersec += getcount;
			long curtime = System.currentTimeMillis() / 1000;
			if (curtime != lasttotalreporttime || flags) {
				System.out.println("testCase1 " + index++ + " " + curtime + " set " + setpersec + "/" + totalset + " get " + getpersec + "/" + totalget);
				lasttotalreporttime = curtime;
				setpersec = 0;
				getpersec = 0;
			}
		}
	}
	TestCase1(long id, int keyCount, int maxSetCount, int maxGetCount) {
		this.id = id;
		key = "id." + id;// + " " + key;
		for (int i = 0; i < 3; i++) {
			key += "+";
		}
		for (int i = 0; i < 10; i++) {
			data += "-";
		}
		this.keyCount = keyCount;
		this.maxSetCount = maxSetCount;
		this.maxGetCount = maxGetCount;
		
		String mgrName = "stresstest";
		CacheClientManager mgr = CacheClientManager.getInstance(mgrName);
		cc = mgr.createClient();
	}
	
	public boolean run() {
		int setcount = 0;
		int getcount = 0;
		if (maxSetCount > 0) {
			setcount = set();
		}

		if (maxGetCount > 0) {
			getcount = get();
		}
		report(false, setcount, getcount);
		if (maxSetCount == 0 && maxGetCount == 0) {
			return false;
		}
		return true;
	}

	int bachCount = 200;
	int set() {
		int setCount = 0;
		int count = bachCount;
		if (keyCount - cachepos < count) {
			count = keyCount - cachepos;
			if (count == 0) {
				cachepos = 0;
				if (keyCount - cachepos >= bachCount) {
					count = 0;
				} else {
					count = keyCount - cachepos;
				}
			}
		}
		
		if (keyCount == 0) {
			count = 0;
		}
		
		for (int i = 0; i < count && maxSetCount > 0; i++, cachepos++, maxSetCount--) {
			String mykey = key + cachepos;
			if (cc.set(mykey, data) == 0) {
				System.out.println("id=" + id + " set error, key=" + mykey);
				break;
			}
			
			setCount++;
		}
		return setCount;
	}
	
	int readpos = 0;
	int get() {
		int getCount = 0;
		if (readpos >= cachepos) {
			readpos = 0;
		}

		for (int i = 0; i < bachCount && readpos < cachepos && maxGetCount > 0; i++, readpos++, maxGetCount--) {
			String mykey = key + readpos;
			if (cc.get(mykey) == null) {
				System.out.println("id=" + id + " get error, key=" + mykey);
				if (cc.get(mykey) == null) {
					System.out.println("again id=" + id + " get error, key=" + mykey);
				} else {
					System.out.println("again id=" + id + " get ok, key=" + mykey);
				}
			}
			getCount++;
		}
		return getCount;
	}
}

public class StressTest {
	
	public StressTest() {
	}

	public boolean runIt(String servers, boolean enableSSL, int threadCount, int keyCount,
			int maxSetCount, int maxGetCount) {
		System.out.println("StressTest.runIt thread=" + threadCount + " keyCount=" + keyCount
				+ " maxSetCount=" + maxSetCount + " maxGetCount=" + maxGetCount);
		String[] serverlist = servers.split(",");

		CacheClientManager mgr = CacheClientManager.getInstance("stresstest");

		mgr.setInitConn(10);

		mgr.setNoDelay(true);
		mgr.initialize(serverlist, enableSSL);
		
		CacheClient cc = mgr.createClient();
		cc.flush();
		
		ArrayList<TestWork> worklist = new ArrayList<TestWork>();
		
		for (int i = 0; i < threadCount; i++) {
			ArrayList<Runable> list1 = new ArrayList<Runable>();
			list1.add(new TestCase1(i, keyCount, maxSetCount, maxGetCount));
			TestWork work1 = new TestWork(list1);
			work1.start();
			worklist.add(work1);
		}

		try {
			for (int j = 0; j < worklist.size(); j++) {
				TestWork work = worklist.get(j);
				work.join();
			}
		} catch (InterruptedException e) { 

		}
		TestCase1.report(true, 0, 0);
		
		mgr.shutdown();
		
		return false;
	}
	
	public static void main(String[] args) {
		BasicConfigurator.configure();
		String servers = System.getProperty("hosts");
		boolean enableSSL = System.getProperty("enableSSL") != null && System.getProperty("enableSSL").equals("true");
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
				return;
			}
		}
		
		StressTest st = new StressTest();
		int threadCount = 2;
		int keyCount = 1000;
		int setCount = 1000 * 3;
		int getCount = 1000 * 10;
//	st.runIt(myservers, threadCount, keyCount, setCount, getCount);
		
		threadCount = 16;
		keyCount = 100000;
		setCount = 100000 * 1;
		getCount = 100000 * 3;
		st.runIt(servers, enableSSL, threadCount, keyCount, setCount, getCount);
	}
}

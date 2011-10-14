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
	long setcount = 0;
	long getcount = 0;
	long lastreporttime = 0;
	
	static long index = 0;
	static long totalset = 0;
	static long totalget = 0;
	static long setpersec = 0;
	static long getpersec = 0;
	static long lasttotalreporttime = 0;
	static String lock = new String();
	
	String key = "key";
	String data = "data";
	static boolean runflag = true;
	void calc(boolean flags) {
//		if (setcount + getcount > 100) {
			synchronized(lock) {
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
				setcount = 0;
				getcount = 0;
			}
	/*	} else {
			try {
				Thread.sleep(1);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}*/
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
	//	System.out.println("testCase1 run"); 
		
		if (!runflag) {
			return false;
		}
		
		if (totalset >= maxSetCount && totalget >= maxGetCount) {
			calc(true);
			return false;
		}

		if (totalset < maxSetCount) {
			step1();
		}

		if (totalget < maxGetCount) {
			step2();
		}
	//	checkall();
		calc(false);
		return true;
	}

	int bachCount = 200;
	void step1() {
	//	int start = cachepos;
		int count = bachCount;
		if (keyCount - cachepos < count) {
			count = keyCount - cachepos;
			if (count == 0) {
			//	cachepos = 0;
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
		
	//	String data = "dd";
		for (int i = 0; runflag && i < count; i++, cachepos++) {
			String mykey = key + cachepos;
			if (cc.set(mykey, data) == 0) {
			//	runflag = false;
				System.out.println("id=" + id + " set error, key=" + mykey);
				break;
			} else if (cc.get(mykey) == null) {
			//	runflag = false;
				System.out.println("id=" + id + " get error, key=" + mykey);
				if (cc.get(mykey) == null) {
					System.out.println("again id=" + id + " get error, key=" + mykey);
				} else {
					System.out.println("again id=" + id + " get ok, key=" + mykey);
				}
				break;
			}
			getcount++;
			
			setcount++;
		//	cacheItemCount--;
		}
	//	cachedItemCount += count;
	}
	
	int readpos = 0;
	void step2() {
		if (readpos >= cachepos) {
			readpos = 0;
		//	System.out.println("cachepos=" + cachepos);
		}
//		int start = cachepos;
	//	int count = bachCount;
//		if (count > cachepos) {
	//		count = cachepos;
		//}
		for (int i = 0; runflag && i < bachCount && readpos < cachepos; i++, readpos++) {
			String mykey = key + readpos;
	//	for (int i = 0; runflag && i < cachepos; i++) {
	//		String mykey = key + i;
			if (cc.get(mykey) == null) {
			//	runflag = false;
				System.out.println("id=" + id + " get error, key=" + mykey);
				if (cc.get(mykey) == null) {
					System.out.println("again id=" + id + " get error, key=" + mykey);
				} else {
					System.out.println("again id=" + id + " get ok, key=" + mykey);
				}
			}
			getcount++;
		}
	}
	
	void checkall() {
		if (cachepos >= 0) {
	
			for (int i = 0; i < cachepos; i++) {
				String mykey = key + i;
				if (cc.get(mykey) == null) {
				//	runflag = false;
					System.out.println("id=" + id + " get error, key=" + mykey);
					if (cc.get(mykey) == null) {
						System.out.println("again id=" + id + " get error, key=" + mykey);
					} else {
						System.out.println("again id=" + id + " get ok, key=" + mykey);
					}
				}
				getcount++;
			}
		}
	}
}

public class StressTest {
	
	public StressTest() {
	}

	public boolean runIt(String servers, int threadCount, int keyCount,
			int maxSetCount, int maxGetCount) {
		String[] serverlist = servers.split(",");

		CacheClientManager mgr = CacheClientManager.getInstance("stresstest");
		mgr.setSocketWriteBufferSize(64 * 1024);//(1 * 1024 * 1024);

		mgr.setInitConn(10);

		mgr.setNoDelay(false);
		mgr.initialize(serverlist);
		
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
		
		mgr.shutdown();
		
		return false;
	}
	
	public static void main(String[] args) {
		
		BasicConfigurator.configure();
		String myservers;
		if (args.length < 1) {
			System.out.println("parameter: server_address(localhost:7788)");
			myservers = "localhost:7788";
		} else {
			myservers = args[0];
		}
		StressTest st = new StressTest();
		int threadCount = 16;
		int keyCount = 100000;
		int setCount = 100000 * 3;
		int getCount = 100000 * 10;

		st.runIt(myservers, threadCount, keyCount, setCount, getCount);
	}
}

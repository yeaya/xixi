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
import java.util.concurrent.atomic.AtomicLong;

interface RunableLocalCache {
	public void run();
}
class TestWorkLocalCache extends Thread {
	
	ArrayList<RunableLocalCache> list_ = new ArrayList<RunableLocalCache>();
	TestWorkLocalCache(ArrayList<RunableLocalCache> list) {
		list_.addAll(list);
	}
	
	public void run() { 
		while (list_.size() > 0) {
			for (int i = 0; i < list_.size(); i++) {
				RunableLocalCache r = list_.get(i);
				r.run();
			}
		}
	} 
}

class TestCaseLocalCache1 implements RunableLocalCache {
	public static final int OP_FLAG_GET_LOCAL = 1;
	public static final int OP_FLAG_GET_REMOTE = 2;
	public static final int OP_FLAG_GET_WATCH = 4;
	public static final int OP_FLAG_UPDATE = 8;
	public static final int OP_FLAG_WATCH = 16;
	CacheClient cc = null;
	long id = 0;
	static int cacheItemCount = 0; 
	static int updatepos = 0;
	long setcount = 0;
	long getcount = 0;
	long misscount = 0;
	long notFoundCount = 0;
	long lastreporttime = 0;
		
	static AtomicLong totalset2 = new AtomicLong(0);
	static AtomicLong totalget2 = new AtomicLong(0);
	static AtomicLong totalmiss2 = new AtomicLong(0);
	static AtomicLong totalNotFound2 = new AtomicLong(0);
	static AtomicLong setpersec2 = new AtomicLong(0);
	static AtomicLong getpersec2 = new AtomicLong(0);
	static AtomicLong misspersec2 = new AtomicLong(0);
	static AtomicLong notFoundCountPerSec2 = new AtomicLong(0);
	
	static long index = 0;
	long totalset = 0;
	long totalget = 0;
	long totalmiss = 0;
	long totalNotFound = 0;
	long setpersec = 0;
	long getpersec = 0;
	long misspersec = 0;
	long notFoundCountPerSec = 0;
	long lasttotalreporttime = 0;

	String key = "key";
	String data = "data";
	int operationType = 0;
	static int updateFinishedPos = 0;
	static String[] updateMap;
	static LocalCache localCache;
	public static void initStatic(int count, LocalCache lc) {
		cacheItemCount = count;
		updateMap = new String[cacheItemCount];
		localCache = lc;
	}
	void report() {
		totalset += setcount;
		totalget += getcount;
		totalmiss += misscount;
		totalNotFound += notFoundCount;
		setpersec += setcount;
		getpersec += getcount;
		misspersec += misscount;
		notFoundCountPerSec += this.notFoundCount;
		
		totalset2.addAndGet(setcount);
		totalget2.addAndGet(getcount);
		totalmiss2.addAndGet(misscount);
		totalNotFound2.addAndGet(notFoundCount);
		setpersec2.addAndGet(setcount);
		getpersec2.addAndGet(getcount);
		misspersec2.addAndGet(misscount);
		notFoundCountPerSec2.addAndGet(this.notFoundCount);
		
		long curtime = System.currentTimeMillis() / 1000;
		if (curtime != lasttotalreporttime) {
//			System.out.println("testCase1 id=" + id + " index=" + index + " " + curtime
//				+ " set " + setpersec + "/" + totalset
//				+ " get " + getpersec + "/" + totalget
//				+ " miss " + misspersec + "/" + totalmiss
//				+ " notfound " + notFoundCountPerSec + "/" + totalNotFound
//				+ " lc=" + localCache.getCacheSize());
			lasttotalreporttime = curtime;
			setpersec = 0;
			getpersec = 0;
			misspersec = 0;
			notFoundCountPerSec = 0;
			
			if ((operationType & OP_FLAG_UPDATE) == OP_FLAG_UPDATE) {
				System.out.println("sum index=" + index + " " + curtime
						+ " set " + setpersec2.get() + "/" + totalset2.get()
						+ " get " + getpersec2.get() + "/" + totalget2.get()
						+ " miss " + misspersec2.get() + "/" + totalmiss2.get()
						+ " notfound " + notFoundCountPerSec2.get() + "/" + totalNotFound2.get()
						+ " lc=" + localCache.getCacheCount() + "/" + localCache.getCacheSize());
				setpersec2.set(0);
				getpersec2.set(0);
				misspersec2.set(0);
				notFoundCountPerSec2.set(0);
				index++;
			}
		}
		if (setcount + getcount + misscount == 0) {
			try {
				Thread.sleep(1);
			} catch (InterruptedException e) {
				e.printStackTrace();
				System.out.println(e.getMessage());
			}
		}
		
		setcount = 0;
		getcount = 0;
		misscount = 0;
		notFoundCount = 0;
	}
	
	TestCaseLocalCache1(String mgrName, long id, int operationType) {
		this.id = id;
		key = "key";// + " " + key;
		for (int i = 0; i < 3; i++) {
			key += "+";
		}
		for (int i = 0; i < 5; i++) {
			data += "-";
		}
		this.operationType = operationType;
		
		CacheClientManager mgr = CacheClientManager.getInstance(mgrName);
		cc = mgr.createClient();
	}
	public void run() {
		if ((operationType & OP_FLAG_GET_LOCAL) == OP_FLAG_GET_LOCAL) {
			this.getLocal();
		}
		if ((operationType & OP_FLAG_GET_REMOTE) == OP_FLAG_GET_REMOTE) {
			this.getRemote();
		}
		if ((operationType & OP_FLAG_GET_WATCH) == OP_FLAG_GET_WATCH) {
			this.getWatch();
		}
		if ((operationType & OP_FLAG_UPDATE) == OP_FLAG_UPDATE) {
			this.updateCache();
		}
		if ((operationType & OP_FLAG_WATCH) == OP_FLAG_WATCH) {
			this.updateWatch();
		}
		report();
	}

	void getLocal() {
		if (readpos >= updateFinishedPos) {
			readpos = 0;
		}

		for (int i = 0; i < bachCount && readpos < updateFinishedPos; i++, readpos++) {
			String mykey = key + readpos;
			String value = (String)cc.getL(mykey);
			if (value == null) {
				notFoundCount++;
				System.out.println("getCacheLocal id=" + id + " get error, key=" + mykey);
				if (cc.get(mykey) == null) {
					System.out.println("getCacheLocal again id=" + id + " get error, key=" + mykey);
				} else {
					System.out.println("getCacheLocal again id=" + id + " get ok, key=" + mykey);
				}
			} else if (!value.equals(updateMap[readpos])) {
				misscount++;
			}
			getcount++;
		}
	}

	int readpos = 0;
	void getRemote() {
		
		if (readpos >= updateFinishedPos) {
			readpos = 0;
		//	System.out.println("cachepos=" + cachepos);
		}
		for (int i = 0; i < bachCount && readpos < updateFinishedPos; i++, readpos++) {
			String mykey = key + readpos;
			if (cc.get(mykey) == null) {
				this.notFoundCount++;
				System.out.println("getCache id=" + id + " get error, key=" + mykey);
				if (cc.get(mykey) == null) {
					System.out.println("getCache again id=" + id + " get error, key=" + mykey);
				} else {
					System.out.println("getCache again id=" + id + " get ok, key=" + mykey);
				}
			}
			getcount++;
		}
	}

	void getWatch() {
		if (readpos >= updateFinishedPos) {
			readpos = 0;
		}

		for (int i = 0; i < bachCount && readpos < updateFinishedPos; i++, readpos++) {
			String mykey = key + readpos;
			String value = (String)cc.getLW(mykey);
			if (value == null) {
				notFoundCount++;
				System.out.println("getCacheLocal id=" + id + " get error, key=" + mykey);
				if (cc.get(mykey) == null) {
					System.out.println("getCacheLocal again id=" + id + " get error, key=" + mykey);
				} else {
					System.out.println("getCacheLocal again id=" + id + " get ok, key=" + mykey);
				}
			} else if (!value.equals(updateMap[readpos])) {
				misscount++;
			}
			getcount++;
		}
	}

	int bachCount = 200;
	int updateReplayCount = 3;
	public void setUpdateReplayCount(int updateReplayCount) {
		this.updateReplayCount = updateReplayCount;
	}
	void updateCache() {
		int count = bachCount;
		if (count > cacheItemCount - updatepos) {
			count = cacheItemCount - updatepos;
			if (count == 0) {
				if (updateReplayCount > 0) {
					updatepos = 0;
					updateReplayCount--;
				}
				if (cacheItemCount - updatepos >= bachCount) {
					count = bachCount;
				} else {
					count = cacheItemCount - updatepos;
				}
			}
		}

	//	String data = "dd";
		for (int i = 0; i < count; i++, updatepos++) {
			String mykey = key + updatepos;
			String value = mykey + System.currentTimeMillis();
			if (cc.set(mykey, value) == 0) {
				System.out.println("updateCache id=" + id + " set error, key=" + mykey);
				break;
			}
			updateMap[updatepos] = value;
		
			setcount++;
		}
		if (updateFinishedPos < updatepos) {
			updateFinishedPos = updatepos;
		}
	}
	
	/*
	public static final int OP_FLAG_WATCH = 16;
	*/
//	int bachCount = 200;
	void updateWatch() {
	//	int start = cachepos;
		int count = bachCount;
		if (count > cacheItemCount - updatepos) {
			count = cacheItemCount - updatepos;
			if (count == 0) {
				updatepos = 0;
				if (cacheItemCount - updatepos >= bachCount) {
					count = bachCount;
				} else {
					count = cacheItemCount - updatepos;
				}
			}
		}

	//	String data = "dd";
		for (int i = 0; i < count; i++, updatepos++) {
			String mykey = key + updatepos;
			String value = mykey + System.currentTimeMillis();
			if (cc.setW(mykey, value) == 0) {
				System.out.println("updateCache id=" + id + " set error, key=" + mykey);
				break;
			}
			updateMap[updatepos] = value;
			setcount++;
		}
		if (updateFinishedPos < updatepos) {
			updateFinishedPos = updatepos;
		}
	}
}

public class StressTestLocalCache {
	protected static CacheClient cc = null;
	private static String[] serverlist;

	long setUpTime = System.currentTimeMillis();
	long tearDownTime = System.currentTimeMillis();
	
	public static void main(String[] args) throws InterruptedException {
		String myservers;
		if (args.length < 1) {
			System.out.println("parameter: server_address(localhost:7788)");
			myservers = "localhost:7788";
		} else {
			myservers = args[0];
		}
		String servers = myservers;
		serverlist = servers.split(",");

		String pooName = "stresstestLocalCache";
		CacheClientManager mgr = CacheClientManager.getInstance(pooName);
		mgr.setSocketWriteBufferSize(64 * 1024);//(1 * 1024 * 1024);
		mgr.setInitConn(10);

		mgr.setNoDelay(false);
		mgr.initialize(serverlist);
		mgr.enableLocalCache();
		LocalCache localCache = mgr.getLocalCache();
		localCache.setMaxCacheSize(512 * 1024 * 1024);
		
		CacheClient cc = mgr.createClient();
		cc.flush();
		Thread.sleep(500);
		
		ArrayList<TestWorkLocalCache> worklist = new ArrayList<TestWorkLocalCache>();
		
		int getThreadCount = 16;
		int keyCount = 200000;
		int updateReplayCount = 2;
		System.out.println("StressTestLocalCache getThreadCount=" + getThreadCount
				+ " keyCount=" + keyCount + " updateReplayCount=" + updateReplayCount);
		
		TestCaseLocalCache1.initStatic(keyCount, localCache);
		ArrayList<RunableLocalCache> list1 = new ArrayList<RunableLocalCache>();
		int id = 0;
		TestCaseLocalCache1 tcc = new TestCaseLocalCache1(pooName, id++, TestCaseLocalCache1.OP_FLAG_UPDATE);
		tcc.setUpdateReplayCount(updateReplayCount);
		list1.add(tcc);
		TestWorkLocalCache work1 = new TestWorkLocalCache(list1);
		work1.start();
		worklist.add(work1);
		
		for (int i = 0; i < 1; i++) {
			ArrayList<RunableLocalCache> list2 = new ArrayList<RunableLocalCache>();
			list2.add(new TestCaseLocalCache1(pooName, id++, TestCaseLocalCache1.OP_FLAG_WATCH));
			TestWorkLocalCache work2 = new TestWorkLocalCache(list2);
	//		work2.start();
			worklist.add(work2);
		}
		
		for (int i = 0; i < getThreadCount; i++) {
			ArrayList<RunableLocalCache> list3 = new ArrayList<RunableLocalCache>();
			list3.add(new TestCaseLocalCache1(pooName, id++, TestCaseLocalCache1.OP_FLAG_GET_WATCH));
			TestWorkLocalCache work3 = new TestWorkLocalCache(list3);
			work3.start();
			worklist.add(work3);
		}
		
		for (int i = 0; i < 1; i++) {
			ArrayList<RunableLocalCache> list3 = new ArrayList<RunableLocalCache>();
			list3.add(new TestCaseLocalCache1(pooName, id++, TestCaseLocalCache1.OP_FLAG_GET_LOCAL));
			TestWorkLocalCache work3 = new TestWorkLocalCache(list3);
		//	work3.start();
			worklist.add(work3);
		}
		
		for (int i = 0; i < 1; i++) {
			ArrayList<RunableLocalCache> list3 = new ArrayList<RunableLocalCache>();
			list3.add(new TestCaseLocalCache1(pooName, id++, TestCaseLocalCache1.OP_FLAG_GET_REMOTE));
			TestWorkLocalCache work3 = new TestWorkLocalCache(list3);
	//		work3.start();
			worklist.add(work3);
		}

		try {
			for (int j = 0; j < worklist.size(); j++) {
				TestWorkLocalCache work = (TestWorkLocalCache)worklist.get(j);
				work.join();
			}
		} catch (InterruptedException e) { 

		} 
	}
}

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

import java.io.IOException;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.HashMap;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;

import com.xixibase.util.Log;
import com.xixibase.util.WeightMap;

public class CacheClientManager {
	private static Log log = Log.getLog(CacheClientManager.class.getName());
	private static ConcurrentHashMap<String, CacheClientManager> managers = new ConcurrentHashMap<String, CacheClientManager>();

	private volatile boolean initialized = false;
	private int defaultGroupID = 0;
	
	private int initConn = 1;
	private int maxActiveConn = 16;
	private long maxBusyTime = 1000 * 30;
	private int socketTimeout = 1000 * 30;
	private int socketConnectTimeout = 1000 * 3;

	private boolean nagle = false;

	private String[] servers;
	private WeightMap<Integer> weightMap;

	private ArrayList<ConcurrentLinkedQueue<XixiSocket>> activeSocketPool = new ArrayList<ConcurrentLinkedQueue<XixiSocket>>();
	private ArrayList<LinkedList<XixiSocket>> inactiveSocketPool = new ArrayList<LinkedList<XixiSocket>>();
	private HashMap<String, Integer> hostIndexMap;

	private int socketWriteBufferSize = 32768; // 32K, 65536; //64K
	private LocalCache localCache = null;
	private String name;
	private MaintainThread maintainThread;

	protected CacheClientManager(String name) {
		this.name = name;
		localCache = new LocalCache(this, 64 * 1024 * 1024);
	}

	public String getName() {
		return name;
	}

	public static CacheClientManager getInstance(String name) {
		CacheClientManager manager = managers.get(name);

		if (manager == null) {
			synchronized (managers) {
				manager = managers.get(name);
				if (manager == null) {
					manager = new CacheClientManager(name);
					managers.put(name, manager);
				}
			}
		}

		return manager;
	}

	public static CacheClientManager getInstance() {
		return getInstance("default");
	}
	
	public int getDefaultGroupID() {
		return defaultGroupID;
	}

	public void setDefaultGroupID(int defaultGroupID) {
		this.defaultGroupID = defaultGroupID;
	}

	public int getDefaultPort() {
		return 7788;
	}

	public boolean initialize(String[] servers) {
		return initialize(servers, null, null);
	}
	
	public boolean initialize(String[] servers, Integer[] weights) {
		return initialize(servers, weights, null);
	}

	public synchronized boolean initialize(String[] servers, Integer[] weights, WeightMap<Integer> weightMap) {
		if (servers == null) {
			log.error("initialize, servers == null");
			return false;
		} else if (servers.length == 0) {
			log.error("initialize, servers.length == 0");
			return false;
		}
		
		this.servers = new String[servers.length];
		System.arraycopy(servers, 0, this.servers, 0, servers.length);
		
		if (weightMap != null) {
			this.weightMap = weightMap;
		} else {
			this.weightMap = new XixiWeightMap<Integer>();
		}
		hostIndexMap = new HashMap<String, Integer>();
		Integer[] values = new Integer[servers.length];
		for (int i = 0; i < servers.length; i++) {
			values[i] = new Integer(i);
		}
		this.weightMap.set(values, weights);
		
		for (int i = 0; i < servers.length; i++) {
			activeSocketPool.add(new ConcurrentLinkedQueue<XixiSocket>());
			inactiveSocketPool.add(new LinkedList<XixiSocket>());
			hostIndexMap.put(servers[i], new Integer(i));
			for (int j = 0; j < initConn; j++) {
				XixiSocket socket = createSocket(servers[i]);
				if (socket == null) {
					break;
				}
				addSocket(servers[i], socket);
			}
		}
		this.initialized = true;
		maintainThread = new MaintainThread();
		maintainThread.start();
		return true;
	}
	
	public void shutdown() {
		managers.remove(name);

		initialized = false;
		disableLocalCache();
		closeSocketPool();

		activeSocketPool.clear();
		inactiveSocketPool.clear();
		hostIndexMap.clear();
		hostIndexMap = null;
		weightMap.clear();
		weightMap = null;
		maintainThread = null;
	}
	
	public void enableLocalCache() {
		localCache.start();
	}
	
	public void disableLocalCache() {
		localCache.stop();
	}
	
	public LocalCache getLocalCache() {
		return localCache;
	}

	public final boolean isInitialized() {
		return initialized;
	}

	public final String[] getServers() {
		return this.servers;
	}

	public final void setInitConn(int initConn) {
		this.initConn = initConn;
	}

	public final int getInitConn() {
		return this.initConn;
	}

	public final void setMaxBusyTime(long maxBusyTime) {
		this.maxBusyTime = maxBusyTime;
	}

	public final long getMaxBusyTime() {
		return this.maxBusyTime;
	}

	public final void setSocketTimeout(int socketTimeout) {
		this.socketTimeout = socketTimeout;
	}

	public final int getSocketTimeout() {
		return this.socketTimeout;
	}

	public final void setSocketConnectTimeout(int socketConnectTimeout) {
		this.socketConnectTimeout = socketConnectTimeout;
	}

	public final int getSocketConnectTimeout() {
		return this.socketConnectTimeout;
	}

	public final void setNagle(boolean nagle) {
		this.nagle = nagle;
	}

	public final boolean getNagle() {
		return this.nagle;
	}

	public final WeightMap<Integer> getWeightMaper() {
		return weightMap;
	}

	public void setMaxActiveConn(int maxActiveConn) {
		this.maxActiveConn = maxActiveConn;
	}

	public int getMaxActiveConn() {
		return maxActiveConn;
	}

	public void setSocketWriteBufferSize(int bufferSize) {
		this.socketWriteBufferSize = bufferSize;
	}

	public int getSocketWriteBufferSize() {
		return socketWriteBufferSize;
	}
	
	public CacheClient createClient() {
		return new CacheClient(this, defaultGroupID);
	}
	
	public CacheClient createClient(int groupID) {
		return new CacheClient(this, groupID);
	}

	protected final XixiSocket createSocket(String host) {
		XixiSocket socket = null;
		try {
			socket = new TCPSocket(this, host, socketWriteBufferSize, socketTimeout,
					socketConnectTimeout, nagle);
		} catch (Exception e) {
			log.error("manager.createSocket, failed to create Socket for host: " + host
					+ " e=" + e.toString());
			socket = null;
		}

		return socket;
	}

	public final String getHost(String key) {
		if (!this.initialized) {
			log.error("getHost, manager is not initialized.");
			return null;
		}
		
		Integer hostIndex = weightMap.get(key);
		if (hostIndex != null) {
			return servers[hostIndex.intValue()];
		}
		return null;
	}

	public final XixiSocket getSocket(String key) {
		if (!this.initialized) {
			log.error("getSocket, manager is not initialized");
			return null;
		}

		XixiSocket socket = null;
		Integer hostIndex = weightMap.get(key);
		if (hostIndex != null) {
			ConcurrentLinkedQueue<XixiSocket> sockets = activeSocketPool.get(hostIndex.intValue());
			socket = sockets.poll();
			if (socket == null) {
				synchronized (inactiveSocketPool) {
					LinkedList<XixiSocket> list = inactiveSocketPool.get(hostIndex.intValue());
					socket = list.pollFirst();
				}
				if (socket == null) {
					socket = createSocket(servers[hostIndex.intValue()]);
				}
			}
		}
		return socket;
	}

	public final XixiSocket getSocketByHost(String host) {
		if (!this.initialized) {
			log.error("getSocketByHost, manager is not initialized");
			return null;
		}
		if (host == null) {
			log.error("getSocketByHost, host == null");
			return null;
		}
		
		XixiSocket socket = null;
		Integer index = hostIndexMap.get(host);
		if (index != null) {
			ConcurrentLinkedQueue<XixiSocket> sockets = activeSocketPool.get(index.intValue());
			socket = sockets.poll();
			if (socket == null) {
				synchronized (inactiveSocketPool) {
					LinkedList<XixiSocket> list = inactiveSocketPool.get(index.intValue());
					socket = list.pollFirst();
				}
				if (socket == null) {
					socket = createSocket(host);
				}
			}
		}
		return socket;
	}

	protected final boolean addSocket(String host, XixiSocket socket) {
		if (this.initialized) {
			ArrayList<ConcurrentLinkedQueue<XixiSocket>> activePool = activeSocketPool;
			if (activePool != null) {
				Integer index = hostIndexMap.get(host);
				if (index != null) {
					ConcurrentLinkedQueue<XixiSocket> sockets = activePool.get(index.intValue());
					if (sockets.size() < maxActiveConn) {
						sockets.add(socket);
						return true;
					} else {
						socket.setLastActiveTime(System.currentTimeMillis());
						synchronized (inactiveSocketPool) {
							inactiveSocketPool.get(index.intValue()).addFirst(socket);
							return true;
						}
					}
				}
			}
		}
		return false;
	}
	
	protected final void maintainInactiveSocket() {
		long currTime = System.currentTimeMillis();
		synchronized (inactiveSocketPool) {
			for (int i = 0; i < inactiveSocketPool.size(); i++) {
				LinkedList<XixiSocket> list = inactiveSocketPool.get(i);

				int count = 0;
				while (!list.isEmpty()) {
					XixiSocket socket = list.getLast();
					if (socket.getLastActiveTime() + 30000 < currTime) {
				//		System.out.println("maintainInactiveSocket, last=" + socket.getLastActiveTime()
				//				+ " curr=" + currTime);
						list.pollLast();
						try {
							socket.trueClose();
						} catch (IOException e) {
							log.error("maintainInactiveSocket, failed to close socket: " + e.getMessage());
						}
						count++;
						if (count >= 2) {
							break;
						}
					} else {
						break;
					}
				}
			}
		}
	}

	protected final void closeSocketPool() {
		for (int i = 0; i < activeSocketPool.size(); i++) {
			ConcurrentLinkedQueue<XixiSocket> sockets = activeSocketPool.get(i);
			XixiSocket socket = sockets.poll();
			while (socket != null) {
				try {
					socket.trueClose();
				} catch (IOException e) {
					log.error("failed to close socket: " + e.getMessage());
				}
				socket = sockets.poll();
			}
		}
		
		synchronized (inactiveSocketPool) {
			for (int i = 0; i < inactiveSocketPool.size(); i++) {
				LinkedList<XixiSocket> list = inactiveSocketPool.get(i);
				XixiSocket socket = list.pollFirst();
				while (socket != null) {
					try {
						socket.trueClose();
					} catch (IOException e) {
						log.error("failed to close socket: " + e.getMessage());
					}
					socket = list.pollFirst();
				}
			}
		}
	}
	
	class MaintainThread extends Thread {
		public void run() {
			while (initialized) {
				maintainInactiveSocket();
		//		long activeSize = localCache.getActiveCacheSize();
		//		long size = localCache.getCacheSize();
		//		System.out.println("CacheSize=" + size + " activeSize=" + activeSize);
				try {
					Thread.sleep(3000);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
			}
		}
	}
}

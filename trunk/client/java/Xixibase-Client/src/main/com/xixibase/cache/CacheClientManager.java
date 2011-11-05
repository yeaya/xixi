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
import java.nio.channels.Selector;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.HashMap;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;

import com.xixibase.util.Log;
import com.xixibase.util.WeightMap;


/**
 * Xixibase cache client manager.
 *
 * @author Yao Yuan
 *
 */
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
	private int maintainInterval = 1000 * 3;
	private int inactiveSocketTimeout = 1000 * 30;

	private boolean noDelay = true;

	private String[] servers;
	private WeightMap<Integer> weightMap = new XixiWeightMap<Integer>();

	private ArrayList<ConcurrentLinkedQueue<XixiSocket>> activeSocketPool = new ArrayList<ConcurrentLinkedQueue<XixiSocket>>();
	private ArrayList<LinkedList<XixiSocket>> inactiveSocketPool = new ArrayList<LinkedList<XixiSocket>>();
	private HashMap<String, Integer> hostIndexMap = new HashMap<String, Integer>();
	
	private ConcurrentLinkedQueue<Selector> selectorPool = new ConcurrentLinkedQueue<Selector>();
	private long lastSelectorOpenTime = 0;

	private int socketWriteBufferSize = 32768; // 32K, 65536; //64K
	private LocalCache localCache = null;
	private String name;
	private MaintainThread maintainThread;

	/**
	 * Creates a <tt>CacheClientManager</tt>.
	 * @param name the name of CacheClientManager.
	 */
	protected CacheClientManager(String name) {
		this.name = name;
		localCache = new LocalCache(this, 64 * 1024 * 1024);
	}
	
	/**
	 * Get the name of CacheClientManager.
	 * @return the name of CacheClientManager
	 */
	public String getName() {
		return name;
	}

	/**
	 * Get the instance of the name. If the instance does not exist, then create an instance.
	 * @param name the name of CacheClientManager instance.
	 * @return the instance of the name
	 */
	public static CacheClientManager getInstance(String name) {
		CacheClientManager manager = managers.get(name);

		if (manager == null) {
			synchronized (CacheClientManager.class) {
				manager = managers.get(name);
				if (manager == null) {
					manager = new CacheClientManager(name);
					managers.put(name, manager);
				}
			}
		}

		return manager;
	}

	/**
	 * Get the instance of the default name "default". If the instance does not exist, then create an instance.
	 * @return the instance of the name "default"
	 */
	public static CacheClientManager getInstance() {
		return getInstance("default");
	}
	
	/**
	 * Get default groupID.
	 * @return default groupID
	 */
	public int getDefaultGroupID() {
		return defaultGroupID;
	}

	/**
	 * Set default groupID.
	 * @param defaultGroupID
	 */
	public void setDefaultGroupID(int defaultGroupID) {
		this.defaultGroupID = defaultGroupID;
	}

	/**
	 * Get default port.
	 * @return default port
	 */
	public int getDefaultPort() {
		return 7788;
	}

	/**
	 * Set maintain thread check interval.
	 * @param maintainInterval
	 */
	public void setMaintainInterval(int maintainInterval) {
		this.maintainInterval = maintainInterval;
	}

	/**
	 * Get maintain thread check interval.
	 * @return maintain thread check interval
	 */
	public int getMaintainInterval() {
		return maintainInterval;
	}
	
	/**
	 * Set inactive socket timeout.
	 * @param inactiveSocketTimeout
	 */
	public void setInactiveSocketTimeout(int inactiveSocketTimeout) {
		this.inactiveSocketTimeout = inactiveSocketTimeout;
	}
	
	/**
	 * Get inactive socket timeout.
	 * @return inactive socket timeout
	 */
	public int getInactiveSocketTimeout() {
		return inactiveSocketTimeout;
	}

	/**
	 * Initialize this instance.
	 * 
     * <pre>
     *     String servers = new String[1];
     *     servers[0] = "localhost:7788";
     *     mgr.initialize(servers);</pre>
     *     
	 * @param servers
     * @return <tt>true</tt> if initialize success
	 */
	public boolean initialize(String[] servers) {
		return initialize(servers, null, null);
	}
	
	/**
	 * Initialize this instance.
	 * 
     * <pre>
     *     String[] servers = new String[2];
     *     Integer[] weights = new Integer[2];
     *     servers[0] = "localhost:7788";
     *     servers[1] = "localhost:8877";
     *     weights[0] = new Integer(70);
     *     weights[1] = new Integer(50);
     *     mgr.initialize(servers, weights);</pre>
     *     
	 * @param servers server list
	 * @param weights server weight list
     * @return <tt>true</tt> if initialize success
	 */
	public boolean initialize(String[] servers, Integer[] weights) {
		return initialize(servers, weights, null);
	}

	/**
	 * Initialize this instance.
	 * 
     * <pre>
     *     String[] servers = new String[2];
     *     Integer[] weights = new Integer[2];
     *     servers[0] = "localhost:7788";
     *     servers[1] = "localhost:8877";
     *     weights[0] = new Integer(70);
     *     weights[1] = new Integer(50);
     *     WeightMap<Integer> weightMap = XixiWeightMap<Integer>();
     *     mgr.initialize(servers, weights, weightMap);</pre>
     *     
	 * @param servers server list
	 * @param weights server weight list
	 * @param weightMap customizable weightMap
     * @return <tt>true</tt> if initialize success
	 */
	public synchronized boolean initialize(String[] servers, Integer[] weights,
			WeightMap<Integer> weightMap) {
		if (servers == null) {
			log.error("initialize, servers == null");
			return false;
		} else if (servers.length == 0) {
			log.error("initialize, servers.length == 0");
			return false;
		}
		if (initialized) {
			log.error("initialize, the manager: " + name + " was already initialized");
			return false;
		}

		this.servers = new String[servers.length];
		System.arraycopy(servers, 0, this.servers, 0, servers.length);
		
		if (weightMap != null) {
			this.weightMap = weightMap;
		} else {
			this.weightMap.clear();// = new XixiWeightMap<Integer>();
		}
		hostIndexMap.clear();// = new HashMap<String, Integer>();
		Integer[] values = new Integer[servers.length];
		for (int i = 0; i < servers.length; i++) {
			values[i] = Integer.valueOf(i);
		}
		this.weightMap.set(values, weights);

		this.initialized = true;
		for (int i = 0; i < servers.length; i++) {
			activeSocketPool.add(new ConcurrentLinkedQueue<XixiSocket>());
			inactiveSocketPool.add(new LinkedList<XixiSocket>());
			hostIndexMap.put(servers[i], Integer.valueOf(i));
			for (int j = 0; j < initConn; j++) {
				XixiSocket socket = createSocket(servers[i]);
				if (socket == null) {
					break;
				}
				addSocket(socket);
			}
		}
		maintainThread = new MaintainThread();
		maintainThread.start();
		return true;
	}
	
	/**
	 * Shutdown this instance.
	 */
	public void shutdown() {
		initialized = false;
		managers.remove(name);

		disableLocalCache();
		closeSocketPool();
		closeSelectorPool();

	//	activeSocketPool.clear();
	//	inactiveSocketPool.clear();
	//	hostIndexMap.clear();
	//	hostIndexMap = null;
	//	weightMap.clear();
		maintainThread = null;
	}
	
	/**
	 * Get active socket count.
	 * 
     * @return active socket count
	 */
	public int getActiveSocketCount() {
		int count = 0;
		for (int i = 0; i < activeSocketPool.size(); i++) {
			ConcurrentLinkedQueue<XixiSocket> queue = activeSocketPool.get(i);
			count += queue.size();
		}
			
		return count;
	}
	
	/**
	 * Get inactive socket count.
	 * 
     * @return inactive socket count
	 */
	public int getInactiveSocketCount() {
		int count = 0;
		synchronized (inactiveSocketPool) {
			for (int i = 0; i < inactiveSocketPool.size(); i++) {
				LinkedList<XixiSocket> list = inactiveSocketPool.get(i);
				count += list.size();
			}
		}
			
		return count;
	}
	
	/**
	 * Enable local cache feature.
	 */
	public void enableLocalCache() {
		localCache.start();
	}

	/**
	 * Disable local cache feature.
	 */
	public void disableLocalCache() {
		localCache.stop();
	}
	
	/**
	 * Get local cache object.
	 * 
     * @return local cache object
	 */
	public LocalCache getLocalCache() {
		return localCache;
	}

	/**
	 * Is this instance initialized?
	 * 
	 * @return <tt>true</tt> if this instance is initialized
	 */
	public final boolean isInitialized() {
		return initialized;
	}

	/**
	 * Get server list.
	 * 
	 * @return server list
	 */
	public final String[] getServers() {
		return this.servers;
	}

	/**
	 * Set the initial number of connections.
	 * 
	 * @param initConn the initial number of connections
	 */
	public final void setInitConn(int initConn) {
		this.initConn = initConn;
	}

	/**
	 * Get the initial number of connections.
	 * 
     * @return the initial number of connections
	 */
	public final int getInitConn() {
		return this.initConn;
	}

	/**
	 * Set max busy time(millisecond).
	 * 
	 * @param maxBusyTime max busy time(millisecond)
	 */
	public final void setMaxBusyTime(long maxBusyTime) {
		this.maxBusyTime = maxBusyTime;
	}

	/**
	 * Get max busy time(millisecond).
	 * 
     * @return max busy time(millisecond)
	 */
	public final long getMaxBusyTime() {
		return this.maxBusyTime;
	}

	/**
	 * Set socket timeout(millisecond).
	 * 
	 * @param socketTimeout socket timeout(millisecond)
	 */
	public final void setSocketTimeout(int socketTimeout) {
		this.socketTimeout = socketTimeout;
	}

	/**
	 * Get socket timeout(millisecond).
	 * 
     * @return socket timeout(millisecond).
	 */
	public final int getSocketTimeout() {
		return this.socketTimeout;
	}

	/**
	 * Set socket connect timeout(millisecond).
	 * 
	 * @param socketConnectTimeout socket connect timeout(millisecond)
	 */
	public final void setSocketConnectTimeout(int socketConnectTimeout) {
		this.socketConnectTimeout = socketConnectTimeout;
	}

	/**
	 * Get socket connect timeout(millisecond).
	 * 
     * @return socket connect timeout(millisecond).
	 */
	public final int getSocketConnectTimeout() {
		return this.socketConnectTimeout;
	}

	/**
	 * Set Enable/Disable socket NoDelay
	 * 
	 * @param noDelay <tt>true</tt> Enable socket NoDelay
	 */
	public final void setNoDelay(boolean noDelay) {
		this.noDelay = noDelay;
	}

	/**
	 * Is enable socket NoDelay?
	 * 
     * @return <tt>true</tt> If enable socket NoDelay.
	 */
	public final boolean isNoDelay() {
		return this.noDelay;
	}

	/**
	 * Get weight mapper.
	 * 
     * @return weight mapper
	 */
	public final WeightMap<Integer> getWeightMapper() {
		return weightMap;
	}

	/**
	 * Set the number of max active connections.
	 * 
	 * @param maxActiveConn the number of max active connections
	 */
	public void setMaxActiveConn(int maxActiveConn) {
		this.maxActiveConn = maxActiveConn;
	}

	/**
	 * Get the number of max active connections.
	 * 
     * @return the number of max active connections
	 */
	public int getMaxActiveConn() {
		return maxActiveConn;
	}

	/**
	 * Set socket write buffer size.
	 * 
	 * @param bufferSize socket write buffer size
	 */
	public void setSocketWriteBufferSize(int bufferSize) {
		this.socketWriteBufferSize = bufferSize;
	}

	/**
	 * Get socket write buffer size.
	 * 
     * @return socket write buffer size
	 */
	public int getSocketWriteBufferSize() {
		return socketWriteBufferSize;
	}

	/**
	 * Create one client with default groupID.
	 * 
     * @return created client
	 */
	public CacheClient createClient() {
		return new CacheClient(this, defaultGroupID);
	}
	
	/**
	 * Create one client with specified groupID.
	 * 
	 * @param groupID specified groupID
     * @return created client
	 */
	public CacheClient createClient(int groupID) {
		return new CacheClient(this, groupID);
	}

	/**
	 * Create one socket with specified host.
	 * 
	 * @param host specified host
     * @return created socket
	 */
	protected final XixiSocket createSocket(String host) {
		if (initialized) {
			try {
				return new TCPSocket(this, host, socketWriteBufferSize,
						socketTimeout, socketConnectTimeout, noDelay);
			} catch (Exception e) {
				log.error("manager.createSocket, failed to create Socket for host: " + host
						+ " e=" + e.toString());
			}
		}
		return null;
	}

	/**
	 * Get host with specified key.
	 * 
	 * @param key specified key
     * @return host
	 */
	public final String getHost(String key) {
//		if (!this.initialized) {
//			log.error("getHost, manager is not initialized.");
//			return null;
//		}
		
		Integer hostIndex = weightMap.get(key);
// hostIndex must not be null
//		if (hostIndex != null) {
			return servers[hostIndex.intValue()];
//		}
//		return null;
	}

	/**
	 * Get socket with specified key.
	 * 
	 * @param key specified key
     * @return socket
	 */
	public final XixiSocket getSocket(String key) {
//		if (!this.initialized) {
//			log.error("getSocket, manager is not initialized");
//			return null;
//		}

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

	/**
	 * Get socket with specified host.
	 * 
	 * @param host specified host
     * @return socket
	 */
	public final XixiSocket getSocketByHost(String host) {
//		if (!this.initialized) {
//			log.error("getSocketByHost, manager is not initialized");
//			return null;
//		}
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

	/**
	 * Add one socket into socket pool.
	 *
	 * @param socket
     * @return <tt>true</tt> if the socket added
	 */
	protected final boolean addSocket(XixiSocket socket) {
		if (initialized) {
			ArrayList<ConcurrentLinkedQueue<XixiSocket>> activePool = activeSocketPool;
			if (activePool != null) {
				Integer index = hostIndexMap.get(socket.getHost());
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

	/**
	 * Close socket pool.
	 */
	protected final void closeSocketPool() {
		for (int i = 0; i < activeSocketPool.size(); i++) {
			ConcurrentLinkedQueue<XixiSocket> sockets = activeSocketPool.get(i);
			XixiSocket socket = sockets.poll();
			while (socket != null) {
				socket.trueClose();
				socket = sockets.poll();
			}
		}
		
		synchronized (inactiveSocketPool) {
			for (int i = 0; i < inactiveSocketPool.size(); i++) {
				LinkedList<XixiSocket> list = inactiveSocketPool.get(i);
				XixiSocket socket = list.pollFirst();
				while (socket != null) {
					socket.trueClose();
					socket = list.pollFirst();
				}
			}
		}
	}
	
	/**
	 * Open one selector, ...
	 *
     * @return Selector
     * @throws IOException if Selector.open failed
	 */
	public Selector selectorOpen() throws IOException {
		Selector selector = selectorPool.poll();
		if (selector == null) {
			lastSelectorOpenTime = System.currentTimeMillis();
			return Selector.open();
		}
		return selector;
	}
	
	/**
	 * Close one selector, ...
	 *
     * @param selector
     * @throws IOException if Selector.open failed
	 */
	public void selectorClose(Selector selector) throws IOException {
		int size = selector.keys().size();
		if (size > 0) {
			selector.selectNow();
			size = selector.keys().size();
			if (size == 0 && selectorPool.size() < 16) {
				selectorPool.add(selector);
				return;
			}
		}
		selector.close();
	}

	/**
	 * Close selector pool
	 */
	protected void closeSelectorPool() {
		Selector selector = selectorPool.poll();
		while (selector != null) {
			try {
				selector.close();
			} catch (IOException e) {
				e.printStackTrace();
			}
			selector = selectorPool.poll();
		}
	}

	/**
	 * Maintain inactive socket
	 */
	protected final void maintainInactiveSocket(long currTime) {
		synchronized (inactiveSocketPool) {
			for (int i = 0; i < inactiveSocketPool.size(); i++) {
				LinkedList<XixiSocket> list = inactiveSocketPool.get(i);

				int count = 0;
				while (!list.isEmpty()) {
					XixiSocket socket = list.getLast();
					if (socket.getLastActiveTime() + inactiveSocketTimeout < currTime) {
				//		System.out.println("maintainInactiveSocket, last=" + socket.getLastActiveTime()
				//				+ " curr=" + currTime);
						list.pollLast();
						socket.trueClose();
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

	/**
	 * Maintain selector pool
	 */
	protected final void maintainSelectorPool(long currTime) {
		if (currTime > lastSelectorOpenTime + 5000) {
			lastSelectorOpenTime = currTime;
			Selector selector = selectorPool.poll();
			if (selector != null) {
				try {
					selector.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
		}
	}
	
	/**
	 * Maintain thread
	 */
	class MaintainThread extends Thread {
		public void run() {
			while (initialized) {
				long currTime = System.currentTimeMillis();
				maintainInactiveSocket(currTime);
				maintainSelectorPool(currTime);
		//		long activeSize = localCache.getActiveCacheSize();
		//		long size = localCache.getCacheSize();
		//		System.out.println("CacheSize=" + size + " activeSize=" + activeSize);
				try {
					Thread.sleep(maintainInterval);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
			}
		}
	}
}

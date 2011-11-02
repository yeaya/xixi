package com.xixibase.benchmark;

import java.util.Properties;
import java.util.concurrent.CyclicBarrier;
import java.util.concurrent.atomic.AtomicLong;

import net.rubyeye.memcached.BaseTest;

import com.google.code.yanf4j.util.ResourcesUtils;
import com.xixibase.cache.CacheClient;
import com.xixibase.cache.CacheClientManager;

public class XixibaseMulti extends BaseTest {
	public static void main(String[] args) throws Exception {
		
		Properties properties = ResourcesUtils
		.getResourceAsProperties("xixibase.properties");
		String servers = (String) properties.get("servers");
		String[] serverlist = servers.split(",");
		String poolName = "Xixibase";
		CacheClientManager manager = CacheClientManager.getInstance(poolName);
		manager.initialize(serverlist);
		
		CacheClient cc = manager.createClient();
		
		System.out.println("XixibaseMulti Client startup");
		warmUp(cc);

		for (int i = 0; i < THREADS.length; i++) {
			for (int j = 0; j < BYTES.length; j++) {
				int repeats = getReapts(i);
				test(cc, BYTES[j], THREADS[i], repeats, true);
			}
		}
		manager.shutdown();
	}

	private static void warmUp(CacheClient cc)
			throws Exception {
		test(cc, 100, 100, 10000, false);
		System.out.println("warm up");
	}

	public static void test(CacheClient cc, int length,
			int threads, int repeats, boolean print) throws Exception {
		cc.flush();
//		System.out.println("flush " + count);
		AtomicLong miss = new AtomicLong(0);
		AtomicLong fail = new AtomicLong(0);
		AtomicLong hit = new AtomicLong(0);
		CyclicBarrier barrier = new CyclicBarrier(threads + 1);

		for (int i = 0; i < threads; i++) {
			new ReadWriteThreadMulti(cc, repeats, barrier, i * repeats,
					length, miss, fail, hit).start();
		}
		barrier.await();
		long start = System.nanoTime();
		barrier.await();
		if (print) {
			long duration = System.nanoTime() - start;
			long total = repeats * threads;
			printResult(length, threads, repeats, miss, fail, hit, duration,
					total);
		}
	}
}

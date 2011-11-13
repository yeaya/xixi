package com.xixibase.benchmark;

import java.util.concurrent.CyclicBarrier;
import java.util.concurrent.atomic.AtomicLong;

import com.xixibase.cache.CacheClient;

import net.rubyeye.memcached.BaseReadWriteThread;

public class ReadWriteThread extends BaseReadWriteThread {
	CacheClient cc;

	public ReadWriteThread(CacheClient cc, int repeats,
			CyclicBarrier barrier, int offset, int length, AtomicLong miss,
			AtomicLong fail, AtomicLong hit) {
		super(repeats, barrier, offset, length, miss, fail, hit);
		this.cc = cc;
	}

	public boolean set(int i, String s) throws Exception {
		return cc.set(String.valueOf(i), s) != 0;
	}

	public String get(int n) throws Exception {
		String result = (String)this.cc.get(String.valueOf(n));
		return result;
	}
}

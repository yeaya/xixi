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

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.List;
import java.util.SortedMap;
import java.util.TreeMap;
import java.util.zip.CRC32;

import com.xixibase.util.WeightMap;

public class XixiWeightMap<V> implements WeightMap<V> {
	public static final int NATIVE_HASH = 0; // native String.hashCode();
	public static final int CRC32_HASH = 1;  // CRC32
	public static final int MD5_HASH = 2;    // MD5
	
	MessageDigest md5 = null;
	boolean consistentFlag;
	private int hashingAlg;

	private List<V> values = new ArrayList<V>();
	private List<Integer> weights = new ArrayList<Integer>();

	private List<V> buckets;
	private TreeMap<Integer, V> consistentBuckets;

	public XixiWeightMap() {
		this(false, NATIVE_HASH); // default to using the native hash
	}
	
	public XixiWeightMap(boolean consistentFlag, int hashingAlg) {
		this.consistentFlag = consistentFlag;
		this.hashingAlg = hashingAlg;
		if (hashingAlg == MD5_HASH) {
			try {
				md5 = MessageDigest.getInstance("MD5");
			} catch (NoSuchAlgorithmException e) {
				e.printStackTrace();
				throw new IllegalStateException("no md5 algorythm found");
			}
		}
	}

	public final int getHashingAlg() {
		return hashingAlg;
	}
	
	public final boolean isConsistent() {
		return consistentFlag;
	}

	public void clear() {
		values.clear();
		weights.clear();
		buckets = null;
		consistentBuckets = null;
	}

	public void set(V[] values, Integer[] weights) {
		if (values != null && values.length > 0) {
			for (int i = 0; i < values.length; i++) {
				this.values.add(values[i]);
				if (weights != null && i < weights.length && weights[i] != null) {
					if (weights[i].intValue() <= 100) {
						this.weights.add(weights[i]);
					} else {
						this.weights.add(100);
					}
				} else {
					this.weights.add(1);
				}
			}
			
			if (consistentFlag) {
				initConsistentBuckets();
			} else {
				initBuckets();
			}
		}
	}

	public V get(String k) {
		int bucket = this.getBucket(k);
		if (consistentFlag) {
			return consistentBuckets.get(bucket);
		} else {
			return buckets.get(bucket);
		}
	}

	private final int getHash(String key) {
		switch (hashingAlg) {
		case NATIVE_HASH:
			return key.hashCode();
		case CRC32_HASH:
			return crc32Hash(key.getBytes());
		case MD5_HASH:
			return md5HashingAlg((key).getBytes());
		default:
			return key.hashCode();
		}
	}

	private int crc32Hash(byte[] key) {
		CRC32 checksum = new CRC32();
		checksum.update(key);
		long crc = checksum.getValue();
		return (int)crc;
	}

	private int md5HashingAlg(byte[] key) {
		md5.reset();
		md5.update(key);
		byte[] bKey = md5.digest();
		int res = ((int) (bKey[3] & 0xFF) << 24) | ((int) (bKey[2] & 0xFF) << 16)
				| ((int) (bKey[1] & 0xFF) << 8) | (int) (bKey[0] & 0xFF);
		return res;
	}
	
	private final int getBucket(String key) {
		int hashCode = getHash(key);

		if (consistentFlag) {
			SortedMap<Integer, V> tmap = this.consistentBuckets.tailMap(hashCode);
			return (tmap.isEmpty()) ? this.consistentBuckets.firstKey() : tmap.firstKey();
		} else {
			int bucket = hashCode % buckets.size();
			if (bucket < 0) {
				return -bucket;
			}
			return bucket;
		}
	}

	private void initBuckets() {
		buckets = new ArrayList<V>();
		for (int i = 0; i < values.size(); i++) {
			for (int j = 0; j < weights.get(i).intValue(); j++) {
				buckets.add(values.get(i));
			}
		}
	}

	private void initConsistentBuckets() {
		consistentBuckets = new TreeMap<Integer, V>();
		int totalWeight = 0;
		for (int i = 0; i < this.weights.size(); i++) {
			totalWeight += this.weights.get(i);
		}

		int multi = 1;
		while (totalWeight * multi < 100) {
			multi++;
		}

		long interval = 0xFFFFFFFFL / totalWeight / multi;
		long offset = 0;
		for (int m = 0; m < multi; m++) {
			for (int i = 0; i < values.size(); i++) {
				for (int j = 0; j < weights.get(i); j++) {
					Integer node = (int)offset;
					consistentBuckets.put(node, values.get(i));
					offset += interval;
				}
			}
		}
	}
}

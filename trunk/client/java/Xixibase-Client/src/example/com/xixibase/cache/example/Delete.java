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

package com.xixibase.cache.example;

import com.xixibase.cache.CacheClient;
import com.xixibase.cache.CacheClientManager;

public class Delete  {

	public static void main(String[] args) {
		String servers = "localhost:8443";
		if (args.length >= 1) {
			servers = args[0];
		}

		String[] serverlist = servers.split(",");

		CacheClientManager manager = CacheClientManager.getInstance("example");
		manager.initialize(serverlist, true);
		
		CacheClient cc = manager.createClient();
		
		cc.set("key", "value");
		System.out.println(cc.get("key"));
		
		cc.delete("key");
		System.out.println(cc.get("key"));
		
		long cacheID1 = cc.set("key", "value1");
		System.out.println(cc.get("key"));
		long cacheID2 = cc.set("key", "value2");
		System.out.println(cc.get("key"));

		cc.delete("key", cacheID1);
		System.out.println(cc.get("key"));

		cc.delete("key", cacheID2);
		System.out.println(cc.get("key"));

		cc.flush();
		
		manager.shutdown();
	}
}

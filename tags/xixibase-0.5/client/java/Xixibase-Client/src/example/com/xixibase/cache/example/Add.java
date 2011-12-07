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

public class Add  {

	public static void main(String[] args) {
		String servers = "localhost:7788";
		if (args.length >= 1) {
			servers = args[0];
		}

		String[] serverlist = servers.split(",");

		CacheClientManager manager = CacheClientManager.getInstance("example");
		manager.initialize(serverlist);
		
		CacheClient cc = manager.createClient();
		
		cc.add("key", "value1");
		System.out.println(cc.get("key"));
		
		cc.add("key", "value2");
		System.out.println(cc.get("key"));

		cc.flush();
		
		manager.shutdown();
	}
}
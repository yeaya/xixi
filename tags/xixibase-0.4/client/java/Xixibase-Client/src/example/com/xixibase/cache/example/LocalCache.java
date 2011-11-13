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

public class LocalCache  {

	public static void main(String[] args) {
		String servers = "localhost:7788";
		if (args.length >= 1) {
			servers = args[0];
		}

		String[] serverlist = servers.split(",");

		CacheClientManager manager = CacheClientManager.getInstance("example");
		manager.initialize(serverlist);
		
		// enable local cache
		manager.enableLocalCache();
		
		CacheClient cc = manager.createClient();

		cc.set("key", "value1");
		
		// get the object from remote Xixibase server
		System.out.println(cc.get("key"));
		
		// get the object from remote Xixibase server
		// and watch the change of the object
		// and save this object into local cache 
		System.out.println(cc.getW("key"));
		
		// try to get the object from local cache
		// if the object is not exist in local cache
		//     get the object from remote Xixibase server
		System.out.println(cc.getL("key"));
		
		// try to get the object from local cache
		// if the object is not exist in local cache
		//     get the object from remote Xixibase server
		//     and watch the change of the object
		//     and save this object into local cache 
		System.out.println(cc.getLW("key")); 

		cc.flush();
		
		// set object to remote Xixibase server
		// and watch the change of the object
		// and save this object into local cache
		cc.setW("key", "value2");
		
		System.out.println(cc.getL("key"));
		
		cc.flush();

		manager.shutdown();
	}
}

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

package com.xixibase.util;

import java.util.concurrent.ConcurrentHashMap;
import org.apache.log4j.Logger;

public class Log {
	private static ConcurrentHashMap<String, Log> logs = new ConcurrentHashMap<String, Log>();
	private static boolean enableLog4j = false;
	private static int level = 1; // 0: debug, 1: info, 2: warn, 3: error, 4: no log
	
	private String name = null;
	private Logger logger = null;

	public static Log getLog(String name) {
		Log log = logs.get(name);
		if (log == null) {
			log = new Log(name);
			logs.put(name, log);
		}
		return log;
	}
	
	public Log(String name) {
		this.name = name;
	}
	
	public static void enableLog4j() {
		enableLog4j = true;
	}
	
	public static void disableLog4j() {
		enableLog4j = false;
	}
	
	public static void setLevel(int logLevel) {
		level = logLevel;
	}
	
	public void debug(String msg) {
		if (level == 0){
			if (enableLog4j) {
				if (logger == null) {
					logger = Logger.getLogger(name);
				}
				logger.debug(msg);
			} else {
				System.out.println(msg);
			}
		}
	}

	public void info(String msg) {
		if (level <= 1){
			if (enableLog4j) {
				if (logger == null) {
					logger = Logger.getLogger(name);
				}
				logger.info(msg);
			} else {
				System.out.println(msg);
			}
		}
	}
	
	public void warn(String msg) {
		if (level <= 2){
			if (enableLog4j) {
				if (logger == null) {
					logger = Logger.getLogger(name);
				}
				logger.warn(msg);
			} else {
				System.out.println(msg);
			}
		}
	}
	
	public void error(String msg) {
		if (level <= 3){
			if (enableLog4j) {
				if (logger == null) {
					logger = Logger.getLogger(name);
				}
				logger.error(msg);
			} else {
				System.out.println(msg);
			}
		}
	}
}

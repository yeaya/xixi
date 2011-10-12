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

public class Defines {
	// Get flag
	public static final int LOCAL_CACHE = 1;
	public static final int WATCH_CACHE = 2;
	public static final int TOUCH_CACHE = 4;
	
	public static final int NO_EXPIRATION = 0;
	public static final long NO_CAS = 0;
	public static final int NO_WATCH = 0;

	public static final byte XIXI_CATEGORY_COMMON = 0x00;
	public static final byte XIXI_CATEGORY_CACHE = 0x02;

	public static final byte XIXI_CMD_ERROR = 0;

	public static final byte XIXI_TYPE_ERROR = XIXI_CMD_ERROR;
	public static final byte XIXI_TYPE_HELLO_REQ = 1;
	public static final byte XIXI_TYPE_HELLO_RES = 2;

	public static final byte XIXI_TYPE_GET_REQ = 1;
	public static final byte XIXI_TYPE_GET_TOUCH_REQ = 2;
	public static final byte XIXI_TYPE_GET_RES = 3;

	public static final byte XIXI_TYPE_GET_BASE_REQ = 4;
	public static final byte XIXI_TYPE_GET_BASE_RES = 5;

	public static final byte XIXI_TYPE_UPDATE_REQ = 6;
	public static final byte XIXI_TYPE_UPDATE_RES = 7;
	public static final byte XIXI_UPDATE_SUB_OP_SET = 0;
	public static final byte XIXI_UPDATE_SUB_OP_ADD = 1;
	public static final byte XIXI_UPDATE_SUB_OP_REPLACE = 2;
	public static final byte XIXI_UPDATE_SUB_OP_APPEND = 3;
	public static final byte XIXI_UPDATE_SUB_OP_PREPEND = 4;
	public static final byte XIXI_UPDATE_REPLY = (byte)128;
	
	public static final byte XIXI_TYPE_UPDATE_BASE_REQ = 8;
	public static final byte XIXI_TYPE_UPDATE_BASE_RES = 9;
	public static final byte XIXI_UPDATE_BASE_SUB_OP_FLAGS = 1;
	public static final byte XIXI_UPDATE_BASE_SUB_OP_EXPIRATION = 2;
	public static final byte XIXI_UPDATE_BASE_REPLY = (byte)128;
	
	public static final byte XIXI_TYPE_DELETE_REQ = 10;
	public static final byte XIXI_TYPE_DELETE_RES = 11;
	public static final byte XIXI_DELETE_SUB_OP = 0;
	public static final byte XIXI_DELETE_REPLY = (byte)128;

	public static final byte XIXI_TYPE_DETLA_REQ = 12;
	public static final byte XIXI_TYPE_DETLA_RES = 13;
	public static final byte XIXI_DELTA_SUB_OP_INCR = 0;
	public static final byte XIXI_DELTA_SUB_OP_DECR = 1;
	public static final byte XIXI_DELTA_REPLY = (byte)128;

	public static final byte XIXI_TYPE_FLUSH_REQ = 14;
	public static final byte XIXI_TYPE_FLUSH_RES = 15;

	public static final byte XIXI_TYPE_STATS_REQ = 18;
	public static final byte XIXI_TYPE_STATS_RES = 19;
	public static final byte XIXI_STATS_SUB_OP_ADD_GROUP = 0;
	public static final byte XIXI_STATS_SUB_OP_REMOVE_GROUP = 1;
	public static final byte XIXI_STATS_SUB_OP_GET_STATS_GROUP_ONLY = 2;
//	public static final byte XIXI_STATS_SUB_OP_GET_AND_CLEAR_STATS_GROUP_ONLY = 3;
	public static final byte XIXI_STATS_SUB_OP_GET_STATS_SUM_ONLY = 4;
//	public static final byte XIXI_STATS_SUB_OP_GET_AND_CLEAR_STATS_SUM_ONLY = 5;
	
	public static final byte XIXI_CREATE_WATCH_REQ = 20;
	public static final byte XIXI_CREATE_WATCH_RES = 21;
	public static final byte XIXI_CHECK_WATCH_REQ = 22;
	public static final byte XIXI_CHECK_WATCH_RES = 23;
	
	public static final short XIXI_REASON_UNKNOWN_COMMAND = 10;

	public static final short XIXI_REASON_SUCCESS = 0;
}

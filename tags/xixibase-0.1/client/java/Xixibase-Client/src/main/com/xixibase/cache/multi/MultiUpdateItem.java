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

package com.xixibase.cache.multi;

public class MultiUpdateItem {
	public String key = null;
	public Object value = null;
	public long cacheID = 0;
	public int expiration = -1;

	protected short reason = 0;
	protected long newCacheID = 0;
	
	public short getReason() {
		return reason;
	}
	
	public long getNewCacheID() {
		return newCacheID;
	}
}

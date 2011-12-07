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

public final class CacheItem extends CacheBaseItem {
	protected Object value;
	protected int itemSize;

	public CacheItem(String key, long cacheID, int expiration, int groupID, int flags,
			Object value, int objectSize, int valueSize) {
		super(key, cacheID, expiration, groupID, flags, valueSize);
		this.value = value;
		this.itemSize = key.length() * 2 + objectSize + 512;
	}
	
	public Object getValue() {
		return value;
	}
}
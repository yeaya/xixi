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

import java.io.IOException;

public interface TransCoder {
	byte[] encodeKey(final String key);
	byte[] encode(final Object object, int[]/*out*/ flags) throws IOException;
	Object decode(final byte[] in, int flags, int[]/*out*/ objectSize) throws IOException;
	
	void setEncodingCharsetName(String encodingCharsetName);
	String getEncodingCharsetName();
	
	void setCompressionThreshold(int compressionThreshold);
	int getCompressionThreshold();

	void setSanitizeKeys(boolean sanitizeKeys);
	boolean getSanitizeKeys();

	short getOption1();
	void setOption1(short option1);
	byte getOption2();
	void setOption2(byte option2);
}
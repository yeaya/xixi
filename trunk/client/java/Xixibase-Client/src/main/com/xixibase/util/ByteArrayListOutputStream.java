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

import java.io.IOException;
import java.io.OutputStream;
import java.util.List;

public final class ByteArrayListOutputStream extends OutputStream {
	private int size = 0;
	private byte[] buffer;
	private int bufferSize;
	private int offset = 0;
	private List<byte[]> byteArrayList;

	public ByteArrayListOutputStream(List<byte[]> byteArrayList, final int bufferSize) {
		this.bufferSize = bufferSize;
		buffer = new byte[bufferSize];
		byteArrayList.add(buffer);
		this.byteArrayList = byteArrayList;
	}
	
	public final int getSize() {
		return size;
	}

	@Override
	public final void write(int b) throws IOException {
		if (offset < buffer.length) {
			buffer[offset] = (byte)b;
			offset++;
		} else {
			buffer = new byte[bufferSize];
			byteArrayList.add(buffer);
			buffer[0] = (byte)b;
			offset = 1;
		}
		size++;
	}

	@Override
	public final void write(byte[] b) throws IOException {
		write(b, 0, b.length);
	}

	@Override
	public final void write(byte[] b, int off, int len) throws IOException {
		if (offset < buffer.length) {
			int remain = buffer.length - offset;
			if (remain >= len) {
				System.arraycopy(b, off, buffer, offset, len);
				offset += len;
			} else {
				System.arraycopy(b, off, buffer, offset, remain);
				int t = len - remain;
				if (t <= bufferSize) {
					buffer = new byte[bufferSize];
					byteArrayList.add(buffer);
					System.arraycopy(b, off + remain, buffer, 0, t);				
				} else {
					buffer = new byte[t];
					byteArrayList.add(buffer);
					System.arraycopy(b, off + remain, buffer, 0, t);
				}
				offset = t;
			}
		} else {
			if (len <= bufferSize) {
				buffer = new byte[bufferSize];
				byteArrayList.add(buffer);
				System.arraycopy(b, off, buffer, 0, len);				
			} else {
				buffer = new byte[len];
				byteArrayList.add(buffer);
				System.arraycopy(b, off, buffer, 0, len);
			}
			offset = len;
		}
		size += len;
	}
}

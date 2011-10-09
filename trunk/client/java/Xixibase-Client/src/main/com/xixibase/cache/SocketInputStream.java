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
import java.io.InputStream;
import java.nio.ByteBuffer;

public final class SocketInputStream extends InputStream {
	private XixiSocket socket;

	public SocketInputStream(final XixiSocket socket) throws IOException {
		this.socket = socket;
		ByteBuffer readBuffer = socket.getReadBuffer();
		readBuffer.clear();
		socket.getChannel().read(readBuffer);
		readBuffer.flip();
	}

	@Override
	public final int read() throws IOException {
		byte[] b = new byte[1];
		socket.read(b, 0, 1);
		return b[0] & 0xff;
	}

	@Override
	public final int read(byte[] b, int off, int len) throws IOException {
		return socket.read(b, off, len);
	}

	public final byte[] read(int len) throws IOException {
		byte[] b = new byte[len];
		socket.read(b, 0, len);
		return b;
	}

	@Override
	public int available() throws IOException {
		return socket.getReadBuffer().remaining();
	}
}

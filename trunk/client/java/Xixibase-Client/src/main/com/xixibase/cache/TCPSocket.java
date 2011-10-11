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
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.nio.channels.SocketChannel;

public class TCPSocket implements XixiSocket {
	private CacheClientManager manager;
	private String host;
	private Socket socket;
	private java.nio.channels.SocketChannel socketChannel;
	private ByteBuffer readBuffer;
	private ByteBuffer writeBuffer;
	private long lastActiveTime;

	public TCPSocket(CacheClientManager manager, String host, int writeBufferSize, int timeout,
			int connectTimeout, boolean noDelay) throws IOException, UnknownHostException {

		this.manager = manager;
		this.host = host;

		String[] ip = host.split(":");

		if (ip.length >= 2) {
			socket = createSocket(ip[0].trim(), Integer.parseInt(ip[1].trim()), connectTimeout);
		} else {
			socket = createSocket(ip[0].trim(), manager.getDefaultPort(), connectTimeout);
		}

		readBuffer = ByteBuffer.allocateDirect(8 * 1024);
		writeBuffer = ByteBuffer.allocateDirect(writeBufferSize);

		if (timeout >= 0) {
			socket.setSoTimeout(timeout);
		}

		socket.setTcpNoDelay(noDelay);
		socketChannel = socket.getChannel();
		if (socketChannel == null) {
			try {
				socket.close();
			} catch (IOException e) {
			}
			socket = null;
			throw new IOException("Can not getChannel for host:" + host);
		}
	}

	public ByteBuffer getReadBuffer() {
		return readBuffer;
	}

	public ByteBuffer getWriteBuffer() {
		return writeBuffer;
	}

	public final SocketChannel getChannel() {
		return socketChannel;
	}

	public final String getHost() {
		return this.host;
	}

	public final void trueClose() throws IOException {
		readBuffer.clear();
		writeBuffer.clear();

		try {
			socketChannel.close();
		} catch (IOException e) {
			try {
				socket.close();
			} catch (IOException e2) {
			}
			throw new IOException("failed on close socketChannel, for host:" + host + ", " + e);
		}

		try {
			socket.close();
		} catch (IOException e) {
			throw new IOException("failed on close socket, for host:" + host + ", " + e);
		}
		socket = null;
	}

	public final void close() {
		readBuffer.clear();
		writeBuffer.clear();
		if (!manager.addSocket(host, this)) {
			try {
				trueClose();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}

	public void setLastActiveTime(long lastActiveTime) {
		this.lastActiveTime = lastActiveTime;
	}
	
	public long getLastActiveTime() {
		return lastActiveTime;
	}
	
	public boolean isConnected() {
		return (socket != null && socket.isConnected());
	}

	public void write(byte[] b, int off, int len) throws IOException {
		int remain = writeBuffer.remaining();
		if (len <= remain) {
			writeBuffer.put(b, off, len);
		} else {
			while (len > 0) {
				if (remain == 0) {
					writeToChannel();
					remain = writeBuffer.remaining();
				}
				int w = len < remain ? len : remain;
				writeBuffer.put(b, off, w);
				off += w;
				len -= w;
				remain = writeBuffer.remaining();
			}
		}
	}
	
	private final void readFromChannel() throws IOException {
		readBuffer.clear();
		socketChannel.read(readBuffer);
		readBuffer.flip();
	}

	private final void writeToChannel() throws IOException {
		writeBuffer.flip();
		socketChannel.write(writeBuffer);
		writeBuffer.clear();
	}
	
	public void flush() throws IOException {
		writeBuffer.flip();
		socketChannel.write(writeBuffer);
	}

	public int read(byte[] b, int off, int len) throws IOException {
		int remain = len;
		while (remain > 0) {
			int r1 = readBuffer.remaining();
			if (r1 == 0) {
				readFromChannel();
				r1 = readBuffer.remaining();
			}
			r1 = r1 < remain ? r1 : remain;
			readBuffer.get(b, off, r1);
			off += r1;
			remain -= r1;
		}
		return len;
	}
	
	protected final static Socket createSocket(String host, int port, int timeout) throws IOException {
		SocketChannel sc = SocketChannel.open();
		sc.socket().connect(new InetSocketAddress(host, port), timeout);
		return sc.socket();
	}
}
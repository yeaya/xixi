/*
   Copyright [2012] [Yao Yuan(yeaya@163.com)]

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
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.SocketChannel;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.util.Iterator;

import javax.net.ssl.SSLEngine;
import javax.net.ssl.SSLEngineResult;
import javax.net.ssl.SSLException;
import javax.net.ssl.SSLSession;

public class SSLSocket implements XixiSocket {
	private CacheClientManager manager;
	private String host;
	private Socket socket;
	private SocketChannel socketChannel;
	private ByteBuffer readBuffer;
	private ByteBuffer writeBuffer;
	private long lastActiveTime;

	// SSL
	private SSLEngine engine;
	private ByteBuffer peerAppData;
	private ByteBuffer peerNetData;
	private ByteBuffer myNetData;

	private boolean SSLHandshaking = false;

	private SSLEngineResult res;
	private SSLEngineResult.HandshakeStatus hsStatus;

	public SSLSocket(CacheClientManager manager, String host,
			int writeBufferSize, int timeout, int connectTimeout,
			boolean noDelay) throws IOException, UnknownHostException,
			KeyManagementException, NoSuchAlgorithmException {

		this.manager = manager;
		this.host = host;

		String[] ip = host.split(":");

		if (ip.length >= 2) {
			socket = createSocket(ip[0].trim(), Integer.parseInt(ip[1].trim()),
					connectTimeout);
		} else {
			socket = createSocket(ip[0].trim(), manager.getDefaultPort(),
					connectTimeout);
		}

		readBuffer = ByteBuffer.allocateDirect(8 * 1024);
		readBuffer.flip();
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
				e.printStackTrace();
			}
			socket = null;
			throw new IOException("Can not getChannel for host:" + host);
		}
		initSSL(timeout);
	}

	static class myTM implements javax.net.ssl.TrustManager,
			javax.net.ssl.X509TrustManager {
		public java.security.cert.X509Certificate[] getAcceptedIssuers() {
			return null;
		}

		public boolean isServerTrusted(
				java.security.cert.X509Certificate[] certs) {
			return true;
		}

		public boolean isClientTrusted(
				java.security.cert.X509Certificate[] certs) {
			return true;
		}

		public void checkServerTrusted(
				java.security.cert.X509Certificate[] certs, String authType)
				throws java.security.cert.CertificateException {
			return;
		}

		public void checkClientTrusted(
				java.security.cert.X509Certificate[] certs, String authType)
				throws java.security.cert.CertificateException {
			return;
		}
	}

	private void initSSL(int timeout) throws NoSuchAlgorithmException,
			KeyManagementException, IOException {
		javax.net.ssl.TrustManager[] trustManagers = new javax.net.ssl.TrustManager[1];
		javax.net.ssl.TrustManager tm = new myTM();
		trustManagers[0] = tm;
		javax.net.ssl.SSLContext sslContext = javax.net.ssl.SSLContext
				.getInstance("TLS");
		sslContext.init(null, trustManagers, null);

		engine = sslContext.createSSLEngine();
		engine.setUseClientMode(true);
		engine.setWantClientAuth(false);
		engine.setNeedClientAuth(false);

		SSLSession session = engine.getSession();
		peerNetData = ByteBuffer.allocate(session.getPacketBufferSize());
		peerAppData = ByteBuffer.allocate(session.getApplicationBufferSize());
		myNetData = ByteBuffer.allocate(session.getPacketBufferSize());

		peerAppData.position(peerAppData.limit());
		myNetData.position(myNetData.limit());

		engine.beginHandshake();
		hsStatus = engine.getHandshakeStatus();
		SSLHandshaking = true;
		socketChannel.configureBlocking(false);
		startSSLHandshake(timeout);
		socketChannel.configureBlocking(true);
	}

	private void startSSLHandshake(int timeout) throws IOException {
		Selector selector = Selector.open();
		socketChannel.register(selector, SelectionKey.OP_READ
				| SelectionKey.OP_WRITE, this);
		long startTime = System.currentTimeMillis();
		long timeRemaining = timeout;

		engine.beginHandshake();
		hsStatus = engine.getHandshakeStatus();

		while (timeRemaining > 0 && SSLHandshaking) {
			int n;
			n = selector.select(timeRemaining);
			if (n > 0) {
				Iterator<SelectionKey> its = selector.selectedKeys().iterator();
				while (its.hasNext()) {
					SelectionKey key = its.next();
					its.remove();
					if (key.isValid()) {
						if (key.isReadable()) {
							this.handleReadSSL();
						} else if (key.isWritable()) {
							this.handleWriteSSL();
						}

						if (myNetData.hasRemaining()) {
							socketChannel.register(selector,
									SelectionKey.OP_READ
											| SelectionKey.OP_WRITE, this);
						} else {
							socketChannel.register(selector,
									SelectionKey.OP_READ, this);
						}
					}
					if (!SSLHandshaking) {
						key.cancel();
						break;
					}
				}
			} else {
				throw new IOException("SSLSocketChannel timeout");
			}
			timeRemaining = timeout - (System.currentTimeMillis() - startTime);
		}
		selector.close();
		
		if (SSLHandshaking) {
			throw new IOException(
					"SSLSocket.startSSLHandshake failed on SSL handshake");	
		}
	}

	public int readSSL(ByteBuffer dst) throws IOException {
		if (!peerAppData.hasRemaining()) {
			int ret = readAndUnwrapSSL();
			if (ret == -1 || ret == 0) {
				return ret;
			}
		}

		int count = Math.min(peerAppData.remaining(), dst.remaining());
		for (int i = 0; i < count; i++) {
			dst.put(peerAppData.get());
		}
		return count;
	}

	private int readAndUnwrapSSL() throws IOException {
		peerAppData.clear();
		peerNetData.flip();

		do {
			res = engine.unwrap(peerNetData, peerAppData);
		} while (res.getStatus() == SSLEngineResult.Status.OK
				&& res.getHandshakeStatus() == SSLEngineResult.HandshakeStatus.NEED_UNWRAP
				&& res.bytesProduced() == 0);

		if (res.getHandshakeStatus() == SSLEngineResult.HandshakeStatus.FINISHED) {
			SSLHandshaking = false;
			return 0;
		}

		if (peerAppData.position() == 0
				&& res.getStatus() == SSLEngineResult.Status.OK
				&& peerNetData.hasRemaining()) {
			res = engine.unwrap(peerNetData, peerAppData);
		}

		hsStatus = res.getHandshakeStatus();

		if (res.getStatus() == SSLEngineResult.Status.CLOSED) {
			return -1;
		}

		peerNetData.compact();
		peerAppData.flip();

		if (hsStatus == SSLEngineResult.HandshakeStatus.NEED_TASK
				|| hsStatus == SSLEngineResult.HandshakeStatus.NEED_WRAP
				|| hsStatus == SSLEngineResult.HandshakeStatus.FINISHED) {
			doHandshake();
		}

		int remain = peerAppData.remaining();
		if (remain > 0) {
			return remain;
		}

		int bytesRead = socketChannel.read(peerNetData);
		if (bytesRead == -1) {
			engine.closeInbound();
			if (peerNetData.position() == 0
					|| res.getStatus() == SSLEngineResult.Status.BUFFER_UNDERFLOW) {
				return -1;
			}
		}

		peerAppData.clear();
		peerNetData.flip();
		do {
			res = engine.unwrap(peerNetData, peerAppData);
		} while (res.getStatus() == SSLEngineResult.Status.OK
				&& res.getHandshakeStatus() == SSLEngineResult.HandshakeStatus.NEED_UNWRAP
				&& res.bytesProduced() == 0);

		if (res.getHandshakeStatus() == SSLEngineResult.HandshakeStatus.FINISHED) {
			SSLHandshaking = false;
		}

		if (peerAppData.position() == 0
				&& res.getStatus() == SSLEngineResult.Status.OK
				&& peerNetData.hasRemaining()) {
			res = engine.unwrap(peerNetData, peerAppData);
		}

		hsStatus = res.getHandshakeStatus();

		if (res.getStatus() == SSLEngineResult.Status.CLOSED) {
			return -1;
		}

		peerNetData.compact();
		peerAppData.flip();

		if (hsStatus == SSLEngineResult.HandshakeStatus.NEED_TASK
				|| hsStatus == SSLEngineResult.HandshakeStatus.NEED_WRAP
				|| hsStatus == SSLEngineResult.HandshakeStatus.FINISHED) {
			doHandshake();
		}

		return peerAppData.remaining();
	}

	public int writeSSL(ByteBuffer src) throws IOException {
		if (myNetData.hasRemaining()) {
			socketChannel.write(myNetData);

			if (myNetData.hasRemaining()) {
				return 0;
			}
		}

		myNetData.clear();
		res = engine.wrap(src, myNetData);
		myNetData.flip();
		socketChannel.write(myNetData);

		return res.bytesConsumed();
	}

	private void closeSSL() throws IOException {
		if (myNetData.hasRemaining()) {
			engine.closeOutbound();
		} else {
			myNetData.clear();
			try {
				ByteBuffer dummy = ByteBuffer.allocate(0);
				res = engine.wrap(dummy, myNetData);
			} catch (SSLException e1) {
				e1.printStackTrace();
				engine.closeOutbound();
				return;
			}
			myNetData.flip();
			socketChannel.write(myNetData);
			engine.closeOutbound();
		}
	}

	private void doHandshake() throws IOException {
		while (true) {
			switch (hsStatus) {
			case NEED_UNWRAP:
				readAndUnwrapSSL();
				return;

			case NEED_WRAP:
				if (myNetData.hasRemaining()) {
					return;
				}
				myNetData.clear();
				ByteBuffer dummy = ByteBuffer.allocate(0);
				res = engine.wrap(dummy, myNetData);
				hsStatus = res.getHandshakeStatus();
				myNetData.flip();
				
				socketChannel.write(myNetData);

				if (myNetData.hasRemaining()) {
					return;
				}
				break;

			case FINISHED:
				SSLHandshaking = false;
				return;

			case NEED_TASK:
				Runnable task;
				while ((task = engine.getDelegatedTask()) != null) {
					task.run();
				}
				hsStatus = engine.getHandshakeStatus();
				break;
				
			case NOT_HANDSHAKING:
				return;
			}
		}
	}

	private void handleReadSSL() throws IOException {
		if (SSLHandshaking) {
			doHandshake();
		}
	}

	private void handleWriteSSL() throws IOException {
		socketChannel.write(myNetData);

		if (!myNetData.hasRemaining()) {
			if (SSLHandshaking) {
				doHandshake();
			}
		}
	}

	public ByteBuffer getWriteBuffer() {
		return writeBuffer;
	}

	public final String getHost() {
		return this.host;
	}

	public final boolean trueClose() {
		readBuffer.clear();
		writeBuffer.clear();

		boolean ret = true;
		try {
			closeSSL();
			socketChannel.close();
		} catch (IOException e) {
			try {
				socket.close();
			} catch (IOException e2) {
			}
			ret = false;
		}
		socketChannel = null;

		try {
			socket.close();
		} catch (IOException e) {
			ret = false;
		}
		socket = null;
		return ret;
	}

	public final void close() {
		readBuffer.clear();
		readBuffer.flip();
		writeBuffer.clear();
		if (!manager.addSocket(this)) {
			trueClose();
		}
	}

	public void setLastActiveTime(long lastActiveTime) {
		this.lastActiveTime = lastActiveTime;
	}

	public long getLastActiveTime() {
		return lastActiveTime;
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
					if (remain == 0) {
						throw new IOException(
								"SSLSocket.write failed on write, len=" + len);
					}
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
		for (int i = 0; i < 100; i++) {
			if (readSSL(readBuffer) != 0) {
				break;
			}
		}
		readBuffer.flip();
	}

	private final void writeToChannel() throws IOException {
		writeBuffer.flip();
		while (writeBuffer.hasRemaining()) {
			writeSSL(writeBuffer);
		}
		writeBuffer.clear();
	}

	public void flush() throws IOException {
		writeToChannel();
	}

	public byte readByte() throws IOException {
		byte[] b = new byte[1];
		read(b, 0, 1);
		return b[0];
	}

	public short readShort() throws IOException {
		byte[] b = new byte[2];
		read(b, 0, 2);
		return ObjectTransCoder.decodeShort(b);
	}

	public int readInt() throws IOException {
		byte[] b = new byte[4];
		read(b, 0, 4);
		return ObjectTransCoder.decodeInt(b);
	}

	public long readLong() throws IOException {
		byte[] b = new byte[8];
		read(b, 0, 8);
		return ObjectTransCoder.decodeLong(b);
	}

	public byte[] read(int len) throws IOException {
		byte[] b = new byte[len];
		read(b, 0, len);
		return b;
	}

	public int read(byte[] b, int off, int len) throws IOException {
		int remain = len;
		while (remain > 0) {
			int r1 = readBuffer.remaining();
			if (r1 == 0) {
				readFromChannel();
				r1 = readBuffer.remaining();
				if (r1 == 0) {
					throw new IOException("SSLSocket.read failed on read, len="
							+ len);
				}
			}
			r1 = r1 < remain ? r1 : remain;
			readBuffer.get(b, off, r1);
			off += r1;
			remain -= r1;
		}
		return len;
	}

	protected final static Socket createSocket(String host, int port,
			int timeout) throws IOException {
		SocketChannel sc = SocketChannel.open();
		sc.socket().connect(new InetSocketAddress(host, port), timeout);
		return sc.socket();
	}

	// async op
	private AsyncHandle handle;

	public boolean isBlocking() {
		return socketChannel.isBlocking();
	}

	public void configureBlocking(boolean block) throws IOException {
		socketChannel.configureBlocking(block);
	}

	public int read(ByteBuffer dst) throws IOException {
		return readSSL(dst);
	}

	public int write(ByteBuffer src) throws IOException {
		return writeSSL(src);
	}

	public boolean handleRead() throws IOException {
		return handle.onRead();
	}

	public boolean handleWrite() throws IOException {
		return handle.onWrite();
	}

	public void register(Selector sel, int ops, AsyncHandle handle)
			throws IOException {
		this.handle = handle;
		socketChannel.register(sel, ops, this);
	}
}
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
import java.nio.ByteBuffer;
import java.nio.channels.Selector;

public interface XixiSocket {
	public ByteBuffer getWriteBuffer();

	public String getHost();

	public boolean trueClose();

	public void close();

	public byte readByte() throws IOException ;
	public short readShort() throws IOException ;
	public int readInt() throws IOException ;
	public long readLong() throws IOException ;

	public byte[] read(int len) throws IOException ;

	public int read(byte[] b, int off, int len) throws IOException ;

	public void write(byte[] b, int off, int len) throws IOException ;
	
	public void flush() throws IOException ;
	
	public void setLastActiveTime(long lastActiveTime);
	public long getLastActiveTime();
	
	// async op
	public boolean isBlocking();
	public void configureBlocking(boolean block) throws IOException ;
	public int read(ByteBuffer dst) throws IOException ;
	public int write(ByteBuffer src) throws IOException ;
	public boolean handleRead() throws IOException ;
	public boolean handleWrite() throws IOException ;
	public void register(Selector sel, int ops, AsyncHandle handle) throws IOException ;
}

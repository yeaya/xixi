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

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.Date;
import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;

public class ObjectTransCoder implements TransCoder {
	public static final int FLAGS_COMPRESSED = 0x80;
	public static final int FLAGS_TYPE_MASK = 0xF;
	public static final int FLAGS_TYPE_BOOLEAN = 1;
	public static final int FLAGS_TYPE_BYTE = 2;
	public static final int FLAGS_TYPE_SHORT = 3;
	public static final int FLAGS_TYPE_CHARACTER = 4;
	public static final int FLAGS_TYPE_INTEGER = 5;
	public static final int FLAGS_TYPE_FLOAT = 6;
	public static final int FLAGS_TYPE_LONG = 7;
	public static final int FLAGS_TYPE_DOUBLE = 8;
	public static final int FLAGS_TYPE_DATE = 9;
	public static final int FLAGS_TYPE_STRING = 10;
	public static final int FLAGS_TYPE_STRINGBUFFER = 11;
	public static final int FLAGS_TYPE_STRINGBUILDER = 12;
	public static final int FLAGS_TYPE_BYTEARR = 13;
	public static final int FLAGS_TYPE_SERIALIZED = 14;

	protected String encodingCharsetName = "UTF-8";
	protected int compressionThreshold = 32 * 1024;
	protected boolean sanitizeKeys = false;
	protected int option1 = 0;
	protected int option2 = 0;

	public ObjectTransCoder() {
//		setOptions1((short)3);
//		setOptions2((byte)15);
//		short s = this.getOptions1();
//		byte b = this.getOptions2();
	}
	
	public void setEncodingCharsetName(String encodingCharsetName) {
		this.encodingCharsetName = encodingCharsetName;
	}
	
	public String getEncodingCharsetName() {
		return encodingCharsetName;
	}
	
	public void setCompressionThreshold(int compressionThreshold) {
		this.compressionThreshold = compressionThreshold;
	}
	
	public int getCompressionThreshold() {
		return compressionThreshold;
	}

	public void setSanitizeKeys(boolean sanitizeKeys) {
		this.sanitizeKeys = sanitizeKeys;
	}
	
	public boolean isSanitizeKeys() {
		return sanitizeKeys;
	}

	public short getOption1() {
		int t = option1 >> 16;
		return (short)(t & 0xFFFF);
	}
	
	public void setOption1(short option1) {
		this.option1 = option1;
		this.option1 = this.option1 << 16;
	}
	
	public byte getOption2() {
		int t = option2 >> 8;
		return (byte)(t & 0xFF);
	}
	
	public void setOption2(byte option2) {
		this.option2 = option2;
		this.option2 = (this.option2 << 8) & 0xFF00;
	}

	public byte[] encode(final Object obj, int[]/*out*/ outflags) throws IOException {
		byte[] b = null;
		int flags = 0;
		if (obj instanceof String) {
			flags = FLAGS_TYPE_STRING;
			b = ((String)obj).getBytes(encodingCharsetName);
		} else if (obj instanceof Boolean) {
			flags = FLAGS_TYPE_BOOLEAN;
			b = new byte[1];
			if (((Boolean) obj).booleanValue()) {
				b[0] = 1;
			} else {
				b[0] = 0;
			}
		} else if (obj instanceof Byte) {
			flags = FLAGS_TYPE_BYTE;
			b = new byte[1];
			b[0] = ((Byte) obj).byteValue();
		} else if (obj instanceof Short) {
			flags = FLAGS_TYPE_SHORT;
			b = encodeInt((int) ((Short) obj).shortValue());
		} else if (obj instanceof Character) {
			flags = FLAGS_TYPE_CHARACTER;
			b = encodeInt(((Character) obj).charValue());
		} else if (obj instanceof Integer) {
			flags = FLAGS_TYPE_INTEGER;
			b = encodeInt(((Integer) obj).intValue());
		} else if (obj instanceof Float) {
			flags = FLAGS_TYPE_FLOAT;
			b = encodeInt((int) Float.floatToIntBits((((Float) obj).floatValue())));
		} else if (obj instanceof Long) {
			flags = FLAGS_TYPE_LONG;
			b = encodeLong(((Long) obj).longValue());
		} else if (obj instanceof Double) {
			flags = FLAGS_TYPE_DOUBLE;
			b = encodeLong((long) Double.doubleToLongBits((((Double) obj).doubleValue())));
		} else if (obj instanceof Date) {
			flags = FLAGS_TYPE_DATE;
			b = encodeLong(((Date) obj).getTime());
		} else if (obj instanceof StringBuffer) {
			flags = FLAGS_TYPE_STRINGBUFFER;
			b = obj.toString().getBytes(encodingCharsetName);
		} else if (obj instanceof StringBuilder) {
			flags = FLAGS_TYPE_STRINGBUILDER;
			b = obj.toString().getBytes(encodingCharsetName);
		} else if (obj instanceof byte[]) {
			flags = FLAGS_TYPE_BYTEARR;
			b = (byte[]) obj;
		} else {
			flags = FLAGS_TYPE_SERIALIZED;
			ByteArrayOutputStream bos = new ByteArrayOutputStream();
			ObjectOutputStream oos = new ObjectOutputStream(bos);
			oos.writeObject(obj);
			oos.close();
			b = bos.toByteArray();
		}

		if (b != null) {
			if (b.length >= compressionThreshold && compressionThreshold > 0) {
				ByteArrayOutputStream bos = new ByteArrayOutputStream();
				GZIPOutputStream gos = new GZIPOutputStream(bos);
				gos.write(b);
				gos.close();
				bos.close();
				byte[] c = bos.toByteArray();
				if (c.length < b.length) {
					b = c;
					flags |= FLAGS_COMPRESSED;
				}
			}
		}
		outflags[0] = flags + option1 + option2;
		return b;
	}

	public Object decode(final byte[] in, int flags, int[]/*out*/ objectSize) throws IOException {
		byte[] b = in;
		if ((flags & FLAGS_COMPRESSED) == FLAGS_COMPRESSED) {
			GZIPInputStream gis = new GZIPInputStream(new ByteArrayInputStream(b));
			ByteArrayOutputStream bos = new ByteArrayOutputStream();
			int count;
			byte[] t = new byte[4096];
			while ((count = gis.read(t)) > 0) {
				bos.write(t, 0, count);
			}
			gis.close();
			bos.close();
			b = bos.toByteArray();
		}
		if (objectSize != null) {
			objectSize[0] = (b.length);
		}

		int type = flags & FLAGS_TYPE_MASK;
		switch (type) {
			case FLAGS_TYPE_BOOLEAN:
				if (b.length >= 1) {
					return (b[0] == 1) ? Boolean.TRUE : Boolean.FALSE;	
				}
				throw new IOException("failed on Boolean decode");
			case FLAGS_TYPE_BYTE:
				if (b.length >= 1) {
					return new Byte(b[0]);
				}
				throw new IOException("failed on Byte decode");
			case FLAGS_TYPE_SHORT:
				if (b.length >= 2) {
					return new Short((short) (new Integer(decodeInt(b))).intValue());
				}
				throw new IOException("failed on Short decode");
			case FLAGS_TYPE_CHARACTER:
				if (b.length >= 2) {
					return new Character((char) (new Integer(decodeInt(b))).intValue());
				}
				throw new IOException("failed on Character decode");
			case FLAGS_TYPE_INTEGER:
				if (b.length >= 4) {
					return new Integer(decodeInt(b));
				}
				throw new IOException("failed on Integer decode");
			case FLAGS_TYPE_FLOAT:
				if (b.length >= 4) {
					return new Float(Float.intBitsToFloat((new Integer(decodeInt(b))).intValue()));
				}
				throw new IOException("failed on Float decode");
			case FLAGS_TYPE_LONG:
				if (b.length >= 8) {
					return new Long(decodeLong(b));
				}
				throw new IOException("failed on Long decode");
			case FLAGS_TYPE_DOUBLE:
				if (b.length >= 8) {
					return new Double(Double.longBitsToDouble((new Long(decodeLong(b))).longValue()));
				}
				throw new IOException("failed on Double decode");
			case FLAGS_TYPE_DATE:
				if (b.length >= 8) {
					return new Date(decodeLong(b));
				}
				throw new IOException("failed on Date decode");
			case FLAGS_TYPE_STRING:
				return new String(b, encodingCharsetName);
			case FLAGS_TYPE_STRINGBUFFER:
				return new StringBuffer(new String(b, encodingCharsetName));
			case FLAGS_TYPE_STRINGBUILDER:
				return new StringBuilder(new String(b, encodingCharsetName));
			case FLAGS_TYPE_BYTEARR:
				return b;
			case FLAGS_TYPE_SERIALIZED: {
				Object obj = null;
				ObjectInputStream ois = new ObjectInputStream(new ByteArrayInputStream(b));
				try {
					obj = ois.readObject();
				} catch (ClassNotFoundException e) {
					throw new IOException(e);
				}
				ois.close();
				return obj;
			}
			default:
				throw new IOException("unknown type:" + type);
		}
	}

	public byte[] encodeKey(final String key) {
		String t = null;
		try {
			t = (sanitizeKeys) ? URLEncoder.encode((String)key, encodingCharsetName) : (String)key;
		} catch (UnsupportedEncodingException e) {
			return null;
		}
		return t.getBytes();
	}

	public static byte[] encodeInt(int value) {
		byte[] b = new byte[4];
		b[0] = (byte) ((value >> 24) & 0xFF);
		b[1] = (byte) ((value >> 16) & 0xFF);
		b[2] = (byte) ((value >> 8) & 0xFF);
		b[3] = (byte) ((value >> 0) & 0xFF);
		return b;
	}

	public static int decodeInt(byte[] b) {
		return (((((int) b[3]) & 0xFF)) + ((((int) b[2]) & 0xFF) << 8)
				+ ((((int) b[1]) & 0xFF) << 16) + ((((int) b[0]) & 0xFF) << 24));
	}

	public static byte[] encodeLong(long value) {
		byte[] b = new byte[8];
		b[0] = (byte) ((value >> 56) & 0xFF);
		b[1] = (byte) ((value >> 48) & 0xFF);
		b[2] = (byte) ((value >> 40) & 0xFF);
		b[3] = (byte) ((value >> 32) & 0xFF);
		b[4] = (byte) ((value >> 24) & 0xFF);
		b[5] = (byte) ((value >> 16) & 0xFF);
		b[6] = (byte) ((value >> 8) & 0xFF);
		b[7] = (byte) ((value >> 0) & 0xFF);
		return b;
	}

	public static long decodeLong(byte[] b) {
		return ((((long) b[7]) & 0xFF) + ((((long) b[6]) & 0xFF) << 8)
				+ ((((long) b[5]) & 0xFF) << 16) + ((((long) b[4]) & 0xFF) << 24)
				+ ((((long) b[3]) & 0xFF) << 32) + ((((long) b[2]) & 0xFF) << 40)
				+ ((((long) b[1]) & 0xFF) << 48) + ((((long) b[0]) & 0xFF) << 56));
	}
}

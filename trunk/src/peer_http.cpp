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

#include <string.h>
#include "peer_http.h"
#include "settings.h"
#include "currtime.h"
#include "stats.h"
#include "log.h"
#include "auth.h"
#include "server.h"

#define READ_BUFFER_HIGHWAT 8192
#define DATA_BUFFER_SIZE 2048

#define LOG_TRACE2(x)  LOG_TRACE("Peer_Http id=" << get_peer_id() << " " << x)
#define LOG_DEBUG2(x)  LOG_DEBUG("Peer_Http id=" << get_peer_id() << " " << x)
#define LOG_INFO2(x)  LOG_INFO("Peer_Http id=" << get_peer_id() << " " << x)
#define LOG_WARNING2(x)  LOG_WARNING("Peer_Http id=" << get_peer_id() << " " << x)
#define LOG_ERROR2(x)  LOG_ERROR("Peer_Http id=" << get_peer_id() << " " << x)

// g: group_id
// w: watch_id
// k: key
// v: value
// e: expiration(second)
// c: cache_id
// f: flags
// d: delta

// XIXI_Get_Req_Pdu
// uint32_t group_id;
// uint32_t watch_id;
// uint16_t key_length;
// uint8_t* key;
// GET /xixibase/get?g=xxx&w=xxx&k=xxx

// XIXI_Get_Touch_Req_Pdu
// uint32_t group_id;
// uint32_t watch_id;
// uint32_t expiration;
// uint16_t key_length;
// uint8_t* key;
// GET /xixibase/get?g=xxx&w=xxx&k=xxx&e=xxx

// XIXI_Get_Base_Req_Pdu
// uint32_t group_id;
// uint16_t key_length;
// uint8_t* key;
// GET /xixibase/getbase?g=xxx&k=xxx

//onst uint8_t XIXI_UPDATE_SUB_OP_MASK = 7;
//const uint8_t XIXI_UPDATE_SUB_OP_SET = 0;
//const uint8_t XIXI_UPDATE_SUB_OP_ADD = 1;
//const uint8_t XIXI_UPDATE_SUB_OP_REPLACE = 2;
//const uint8_t XIXI_UPDATE_SUB_OP_APPEND = 3;
//const uint8_t XIXI_UPDATE_SUB_OP_PREPEND = 4;
//const uint8_t XIXI_UPDATE_REPLY = 128;
// XIXI_Update_Req_Pdu
// uint64_t cache_id;
// uint32_t group_id;
// uint32_t flags;
// uint32_t expiration;
// uint32_t watch_id;
// uint16_t key_length;
// uint32_t data_length;
// POST /xixibase/set?c=xxx&g=xxx&f=xxx&e=xxx&w=xxx&k=xxx
// Content_Length: data_length
// data


// data: must not be NULL
// length: must > 0
// sub: must not be NULL
// sub_len: must > 0
char* memfind(char* data, uint32_t length, char* sub, uint32_t sub_len) {
	char* p = data;
	if (length <= sub_len) {
		if (length == sub_len && memcmp(data, sub, length) == 0) {
			return data;
		}
		return NULL;
	}
	for (uint32_t i = 0; i <= length - sub_len; i++) {
		uint32_t j = 0;
		for (; j < sub_len; j++) {
			if (data[i + j] != sub[j]) {
				break;
			}
		}
		if (j == sub_len) {
			return data + i;
		}
	}
	return NULL;
}


void tokenize_command(char* command, vector<token_t>& tokens)
{
	assert(command != NULL);

	char* start = command;
	char* end = command;
	token_t t;

	while (true) {
		if (*end == '&') {
			if (start != end) {
				t.value = start;
				t.length = end - start;
				*end = '\0';
				tokens.push_back(t);
			}
			start = end + 1;
		} else if (/**end == ' ' || */*end == '\0') {
			if (start != end) {
				t.value = start;
				t.length = end - start;
				*end = '\0';
				tokens.push_back(t);
			}
			break;
		}
		++end;
	}
}

char* Peer_Http::decode_uri(char* uri, uint32_t length, uint32_t& out) {
	char* buf = (char*)request_buf_.prepare(length + 1);
	if (buf == NULL) {
		return NULL;
	}
	char* p = buf;
	out = 0;
	for (uint32_t i = 0; i < length; i++) {
		char c = *uri;
		uri++;
		if (c == ' ' || c == '\r' || c == '\n' || c == '\0') {
			break;
		}
		if (c == '%' && i + 2 < length) {
			char c1 = *uri;
			if (c1 >= '0' && c1 <= '9') {
				c1 = c1 - '0';
			} else {
				if (c1 >= 'a' && c1 <= 'f') {
					c1 = c1 - 'a' + 10;
				} else {
					c1 = c1 - 'A' + 10;
				}
			}
			uri++;
			char c2 = *uri;
			if (c2 >= '0' && c2 <= '9') {
				c2 = c2 - '0';
			} else {
				if (c2 >= 'a' && c2 <= 'f') {
					c2 = c2 - 'a' + 10;
				} else {
					c2 = c2 - 'A' + 10;
				}
			}
			uri++;
			c = (c1 << 4) + (c2& 0xF);
		}
		if (c == '+') {
			c = ' ';
		}
		*p = c;
		p++;
		out++;
	}
	*p = '\0';
	return buf;
}

Peer_Http::Peer_Http(boost::asio::ip::tcp::socket* socket) : self_(this) {
	LOG_DEBUG2("Peer_Http::Peer_Http()");
	op_count_ = 0;
	socket_ = socket;
	state_ = PEER_STATE_NEW_CMD;
	next_state_ = PEER_STATE_NEW_CMD;
	cache_item_ = NULL;
	write_buf_total_ = 0;
	read_item_buf_ = NULL;
	swallow_size_ = 0;
	next_data_len_ = XIXI_PDU_HEAD_LENGTH;
	timer_ = NULL;

	group_id_ = 0;
	watch_id_ = 0;
	cache_id_ = 0;
	key_ = NULL;
	key_length_ = 0;
	value_ = NULL;
	value_length_ = 0;
	post_data_ = NULL;
	content_length_ = 0;
	flags_ = 0;
	expiration_ = 0;
	touch_flag_ = false;
	delta_ = 1;

	stats_.new_conn();
}

Peer_Http::~Peer_Http() {
	LOG_DEBUG2("~Peer_Http::Peer_Http()");
	cleanup();
	stats_.close_conn();
}

void Peer_Http::cleanup() {
	cache_item_ = NULL;

	for (uint32_t i = 0; i < cache_items_.size(); i++) {
		cache_mgr_.release_reference(cache_items_[i]);
	}
	cache_items_.clear();

	if (socket_ != NULL) {
		delete socket_;
		socket_ = NULL;
	}
	if (timer_ != NULL) {
		delete timer_;
		timer_ = NULL;
	}
}

void Peer_Http::write_simple_res(xixi_choice choice) {
	uint8_t* cb = request_buf_.prepare(XIXI_PDU_SIMPLE_RES_LENGTH);
	XIXI_Simple_Res_Pdu::encode(cb, choice);

	add_write_buf(cb, XIXI_PDU_SIMPLE_RES_LENGTH);

	set_state(PEER_STATUS_WRITE);
	next_state_ = PEER_STATE_NEW_CMD;
}

void Peer_Http::write_simple_res(xixi_choice choice, uint32_t request_id) {
	uint8_t* cb = request_buf_.prepare(XIXI_PDU_SIMPLE_RES_WITH_REQID_LENGTH);
	XIXI_Simple_Res_With_ReqID_Pdu::encode(cb, request_id, choice);

	add_write_buf(cb, XIXI_PDU_SIMPLE_RES_WITH_REQID_LENGTH);

	set_state(PEER_STATUS_WRITE);
	next_state_ = PEER_STATE_NEW_CMD;
}

void Peer_Http::write_error(xixi_reason error_code) {
	char* res = NULL;
	switch (error_code) {
		case XIXI_REASON_NOT_FOUND:
			res = "HTTP/1.1 404 Not Found\r\n"
				"Server: "VERSION"\r\n"
				"Content-Type: text/html\r\n"
				"Content-Length: 21\r\n\r\n"
				"XIXI_REASON_NOT_FOUND";
			break;
		case XIXI_REASON_EXISTS:
			res = "HTTP/1.1 405 Method Not Allowed\r\n"
				"Server: "VERSION"\r\n"
				"Content-Type: text/html\r\n"
				"Content-Length: 21\r\n\r\n"
				"XIXI_REASON_NOT_FOUND";
			break;
		case XIXI_REASON_TOO_LARGE:
			res = "HTTP/1.1 413 Request Entity Too Large\r\n"
				"Server: "VERSION"\r\n"
				"Content-Type: text/html\r\n"
				"Content-Length: 21\r\n\r\n"
				"XIXI_REASON_NOT_FOUND";
			break;
		case XIXI_REASON_INVALID_PARAMETER:
			res = "HTTP/1.1 400 Bad Request\r\n"
				"Server: "VERSION"\r\n"
				"Content-Type: text/html\r\n"
				"Content-Length: 21\r\n\r\n"
				"XIXI_REASON_NOT_FOUND";
			break;
		case XIXI_REASON_INVALID_OPERATION:
			res = "HTTP/1.1 400 Bad Request\r\n"
				"Server: "VERSION"\r\n"
				"Content-Type: text/html\r\n"
				"Content-Length: 21\r\n\r\n"
				"XIXI_REASON_NOT_FOUND";
			break;
		case XIXI_REASON_MISMATCH:
			res = "HTTP/1.1 400 Bad Request\r\n"
				"Server: "VERSION"\r\n"
				"Content-Type: text/html\r\n"
				"Content-Length: 21\r\n\r\n"
				"XIXI_REASON_NOT_FOUND";
			break;
		case XIXI_RES_AUTH_ERROR:
			res = "HTTP/1.1 401 Unauthorized\r\n"
				"Server: "VERSION"\r\n"
				"Content-Type: text/html\r\n"
				"Content-Length: 21\r\n\r\n"
				"XIXI_REASON_NOT_FOUND";
			break;
		case XIXI_REASON_UNKNOWN_COMMAND:
			res = "HTTP/1.1 400 Bad Request\r\n"
				"Server: "VERSION"\r\n"
				"Content-Type: text/html\r\n"
				"Content-Length: 21\r\n\r\n"
				"XIXI_REASON_NOT_FOUND";
			break;
		case XIXI_REASON_OUT_OF_MEMORY:
			res = "HTTP/1.1 500 Internal Server Error\r\n"
				"Server: "VERSION"\r\n"
				"Content-Type: text/html\r\n"
				"Content-Length: 21\r\n\r\n"
				"XIXI_REASON_NOT_FOUND";
			break;
/*		case XIXI_REASON_WAIT_FOR_ME:
			res = "HTTP/1.1 200 OK\r\n"
				"Server: "VERSION"\r\n"
				"Content-Type: text/html\r\n"
				"Content-Length: 21\r\n\r\n"
				"XIXI_REASON_NOT_FOUND";
			break;
		case XIXI_REASON_PLEASE_TRY_AGAIN:
			res = "HTTP/1.1 200 OK\r\n"
				"Server: "VERSION"\r\n"
				"Content-Type: text/html\r\n"
				"Content-Length: 21\r\n\r\n"
				"XIXI_REASON_NOT_FOUND";
			break;*/
		case XIXI_REASON_WATCH_NOT_FOUND:
			res = "HTTP/1.1 400 Bad Request\r\n"
				"Server: "VERSION"\r\n"
				"Content-Type: text/html\r\n"
				"Content-Length: 21\r\n\r\n"
				"XIXI_REASON_NOT_FOUND";
			break;
	}

	add_write_buf((uint8_t*)res, strlen(res));
	set_state(PEER_STATUS_WRITE);
	next_state_ = PEER_STATE_CLOSING;
}

uint32_t Peer_Http::process() {
	LOG_TRACE2("process length=" << read_buffer_.read_data_size_);
	uint32_t offset = 0;

	uint32_t tmp = 0;
	uint32_t process_reqest_count = 0;
	bool run = true;

	while (run) {

		LOG_TRACE2("process while state_" << (uint32_t)state_ << " offset=" << offset);

		switch (state_) {

	case PEER_STATE_NEW_CMD:
		reset_for_new_cmd();
		break;

	case PEER_STATE_READ_HEADER:
		tmp = try_read_command((char*)read_buffer_.read_curr_, read_buffer_.read_data_size_);
		if (tmp > 0) {
			process_reqest_count++;
			offset += tmp;
			read_buffer_.read_curr_ += tmp;
			read_buffer_.read_data_size_ -= tmp;
		} else {
			read_buffer_.handle_processed();
			uint32_t size = read_some(read_buffer_.get_read_buf(), read_buffer_.get_read_buf_size());
			if (size == 0) {
				run = false;
			} else {
				read_buffer_.read_data_size_ += size;
				//  LOG_INFO2("read_some PEER_STATE_READ_HEADER, size=" << size);
			}
		}
		break;

	case PEER_STATE_READ_BODY_EXTRAS:
		if (next_data_len_ == 0) {
			process_post();
		} else if (read_buffer_.read_data_size_ > 0) {
			tmp = read_buffer_.read_data_size_ > next_data_len_ ? next_data_len_ : read_buffer_.read_data_size_;
			memcpy(read_item_buf_, read_buffer_.read_curr_, tmp);
			offset += tmp;
			read_item_buf_ += tmp;
			next_data_len_ -= tmp;
			read_buffer_.read_curr_ += tmp;
			read_buffer_.read_data_size_ -= tmp;
		} else {
			uint32_t size = read_some(read_item_buf_, next_data_len_);
			if (size == 0) {
				run = false;
			} else {
				offset += size;
				read_item_buf_ += size;
				next_data_len_ -= size;
			}
		}
		break;
/*
	case PEER_STATE_READ_BODY_EXTRAS2:
//		tmp = process_pdu_extras2(read_pdu_, read_buffer_.read_curr_, read_buffer_.read_data_size_);
		if (tmp > 0) {
			offset += tmp;
			read_buffer_.read_curr_ += tmp;
			read_buffer_.read_data_size_ -= tmp;
		} else {
			read_buffer_.handle_processed();
			uint32_t size = read_some(read_buffer_.get_read_buf(), read_buffer_.get_read_buf_size());
			if (size == 0) {
				run = false;
			} else {
				read_buffer_.read_data_size_ += size;
				//  LOG_INFO2("read_some PEER_STATE_READ_BODY_EXTRAS2, size=" << size);
			}
		}
		break;
*/
	case PEER_STATE_SWALLOW:
		if (swallow_size_ == 0) {
			set_state(PEER_STATE_NEW_CMD);
		} else if (read_buffer_.read_data_size_ > 0) {
			tmp = read_buffer_.read_data_size_ > swallow_size_ ? swallow_size_ : read_buffer_.read_data_size_;
			swallow_size_ -= tmp;
			offset += tmp;
			read_buffer_.read_curr_ += tmp;
			read_buffer_.read_data_size_ -= tmp;
		} else {
			read_buffer_.handle_processed();
			uint32_t size = read_some(read_buffer_.get_read_buf(), read_buffer_.get_read_buf_size());
			if (size == 0) {
				run = false;
			} else {
				read_buffer_.read_data_size_ += size;
				//  LOG_INFO2("read_some PEER_STATE_SWALLOW, size=" << size);
			}
		}
		break;

	case PEER_STATUS_ASYNC_WAIT:
		run = false;
		break;

	case PEER_STATUS_WRITE:
		set_state(next_state_);
		next_state_ = PEER_STATE_NEW_CMD;
		if (state_ == PEER_STATE_NEW_CMD && process_reqest_count < 32) {
			if (read_buffer_.read_data_size_ >= 2) {
				set_state(PEER_STATE_READ_HEADER);
			} else {
				run = false;
			}
		} else {
			run = false;
		}
		break;

	case PEER_STATE_CLOSING:
		set_state(PEER_STATE_CLOSED);
		run = false;
		break;

	case PEER_STATE_CLOSED:
		break;

	default:
		assert(false);
		break;
		}
	}

	return offset;
}

void Peer_Http::set_state(peer_state state) {
	LOG_TRACE("state change, from " << (uint32_t)state_ << " to " << (uint32_t)state);
	state_ = state;
}

/*
GET / HTTP/1.1
GET /xixibase/get?g=222 HTTP/1.1

Accept-Language: zh-cn
User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; Trident/4.0; .NET CLR 2.0.50727; .NET CLR 3.0.04506.648; .NET CLR 3.5.21022; InfoPath.2; 360SE)
Accept-Encoding: gzip, deflate
Host: localhost:7788
Connection: Keep-Alive
*/

uint32_t Peer_Http::try_read_command(char* data, uint32_t data_len) {
	char* p = memfind(data, data_len, "\r\n\r\n", 4);
	if (p != NULL) {
		*p = '\0';

		assert(p < (data + data_len));

		process_request_header(data, (uint32_t)(p - data));

		return (p - data + 4);
	}

	if (data_len >= 4096) {
		set_state(PEER_STATE_CLOSING);
		return 0;
	}

	set_state(PEER_STATE_READ_HEADER);
	return 0;
}

void Peer_Http::process_request_header(char* request_header, uint32_t length) {
	char* request_line = request_header;
	uint32_t request_line_length = length;
	char* p = memfind(request_line, length, "\r\n", 2);
	char* request_header_fields = NULL;
	uint32_t request_header_fields_length = 0;
	if (p != NULL) {
		request_header_fields = p + 2;
		request_header_fields_length = length - (request_header_fields - request_header);
		*p = '\0';
		request_line_length = p - request_line;
	}
	if (request_line_length >= 6) {
		if (memcmp(request_line + request_line_length - 3, "1.1", 3) == 0) {
			http_request_.http_11 = true;
		}

		uint32_t offset = 0;
		uint32_t method = *((uint32_t*)request_header);
		bool is_get = true;
		if (method == GET_METHOD) {
			offset = 4;
		} else if (method == POST_METHOD) {
			offset = 5;
			is_get = false;
		} else {
			LOG_WARNING2("try_read_command error method=" << method);
			write_error(XIXI_REASON_INVALID_PARAMETER);
			return;
		}
		http_request_.method = method;

		http_request_.uri = request_line + offset;
		p = (char*)memchr(http_request_.uri, ' ', request_line_length - offset);
		if (p != NULL) {
			*p = '\0';
			http_request_.uri_length = p - http_request_.uri;
		} else {
			http_request_.uri_length = request_line_length - offset;
		}
		
//		http_request_.uri = decode_uri(request_line + offset, request_line_length - offset, http_request_.uri_length);
//		if (http_request_.uri == NULL) {
//			LOG_WARNING2("try_read_command failed on decode_uri " << request_line + offset);
//			write_error(XIXI_REASON_OUT_OF_MEMORY);
//			return;
//		}
		if (!process_request_header_fields(request_header_fields, request_header_fields_length)) {
			LOG_WARNING2("try_read_command failed on process request header fields " << request_header_fields);
			write_error(XIXI_REASON_INVALID_PARAMETER);
			return;
		}
		if (method == POST_METHOD && content_length_ > 0) {
			if (content_length_ > settings_.item_size_max * 3) {
				LOG_WARNING2("try_read_command the content length is too large " << content_length_);
				write_error(XIXI_REASON_TOO_LARGE);
				return;
			}
			post_data_ = request_buf_.prepare(content_length_ + 1);
			if (post_data_ == NULL) {
				LOG_WARNING2("try_read_command out of memory " << content_length_);
				write_error(XIXI_REASON_OUT_OF_MEMORY);
				return;
			}
			post_data_[content_length_] = '\0';
			read_item_buf_ = post_data_;
			next_data_len_ = content_length_;
			set_state(PEER_STATE_READ_BODY_EXTRAS);

			char* buf = (char*)request_buf_.prepare(http_request_.uri_length + 1);
			if (buf == NULL) {
				LOG_WARNING2("try_read_command uri out of memory " << (http_request_.uri_length + 1));
				write_error(XIXI_REASON_OUT_OF_MEMORY);
			}
			memcpy(buf, http_request_.uri, http_request_.uri_length);
			buf[http_request_.uri_length] = '\0';
			http_request_.uri = buf;
		} else {
			process_command();
		}
	} else {
		LOG_WARNING2("try_read_command error unkown request=" << request_header);
		write_error(XIXI_REASON_INVALID_PARAMETER);
	}
}

bool Peer_Http::process_request_header_fields(char* request_header_field, uint32_t length) {
	char* name = request_header_field;
	uint32_t name_length = 0;
	char* p = (char*)memchr(name, ':', length);
	while (p != NULL) {
		*p = '\0';
		name_length = p - name;
		p++;
		while (*p == ' ') {
			p++;
		}
		char* value = p;
		uint32_t value_length = 0;
		char* p2 = (char*)memchr(value, '\n', length - (value - request_header_field));
		if (p2 != NULL) {
			if (*(p2 - 1) == '\r') {
				*(p2 - 1) = '\0';
				value_length = (p2 - value) - 1;
			} else {
				*(p2) = '\0';
				value_length = p2 - value;
			}
	//		LOG_INFO2(name << "=" << value);
			if (!handle_request_header_field(name, name_length, value, value_length)) {
				return false;
			}
			name = p2 + 1;
			name_length = 0;
			p = (char*)memchr(name, ':', length - (name - request_header_field));
		} else {
			break;
		}
	}
	// Content-Type
	// get Content_Length
/*	char* t = strstr(request_header_field, "Content-Length");
	if (t != NULL) {
		if (!safe_toui64(t + 15, t - request_header_field - 15, content_length_)) {
			write_error(XIXI_REASON_INVALID_PARAMETER);
			return;
		}
	}*/
	return true;
}

bool Peer_Http::handle_request_header_field(char* name, uint32_t name_length, char* value, uint32_t value_length) {
	if (name_length == 12) {
		if (memcmp(name, "Content-Type", name_length) == 0) {
			char* buf = (char*)request_buf_.prepare(value_length + 1);
			if (buf == NULL) {
				return false;
			}
			memcpy(buf, value, value_length);
			buf[value_length] = '\0';
			http_request_.content_type = buf;
			http_request_.content_type_length = value_length;

			if (value_length > 30 && memcmp(buf, "multipart/form-data", 19) == 0) {
				char* p = strstr(buf + 19, "boundary=");
				if (p != NULL) {
					http_request_.boundary = p + 9;
					http_request_.boundary_length = value_length - (http_request_.boundary - buf);
				}
				buf[19] = '\0';
				http_request_.content_type_length = 19;
			}
		}
	} else if (name_length == 14) {
		if (memcmp(name, "Content-Length", name_length) == 0) {
			if (!safe_toui32(value, value_length, content_length_)) {
				return false;
			}
		}
	}
//	[2011-Oct-26 23:58:37.671875 00159E20]: Peer_Http id=1 Content-Type=multipart/form-data; boundary=---------------------------7db261252115f6
	return true;
}

void Peer_Http::process_command() {
	if (http_request_.uri_length >= 10 && memcmp(http_request_.uri, "/xixibase/", 10) == 0) {
		uint32_t cmd_length = 0;
		char* arg = strrchr(http_request_.uri, '?');
		if (arg != NULL) {
			cmd_length = arg - http_request_.uri - 10;
			if (!process_request_arg(arg + 1)) {
				LOG_WARNING2("process_cahce_arg failed arg=" << (arg + 1));
				write_error(XIXI_REASON_INVALID_PARAMETER);
				return;
			}
		} else {
			cmd_length = http_request_.uri_length - 10;
		}

		char* cmd = http_request_.uri + 10;
		if (cmd_length == 3) {
			if (memcmp(cmd, "get", cmd_length) == 0) {
				process_get();
			} else if (memcmp(cmd, "set", cmd_length) == 0) {
				process_update(XIXI_UPDATE_SUB_OP_SET);
			} else if (memcmp(cmd, "add", cmd_length) == 0) {
				process_update(XIXI_UPDATE_SUB_OP_ADD);
			} else if (memcmp(cmd, "del", cmd_length) == 0) {
				process_delete();
			} else {
				LOG_WARNING2("try_read_command error unkown request=" << http_request_.uri);
				write_error(XIXI_REASON_INVALID_PARAMETER);
				return;
			}
		} else if (cmd_length == 4) {
			if (memcmp(cmd, "incr", cmd_length) == 0) {
				process_delta(true);
			} else if (memcmp(cmd, "decr", cmd_length) == 0) {
				process_delta(false);
			} else {
				LOG_WARNING2("try_read_command error unkown request=" << http_request_.uri);
				write_error(XIXI_REASON_INVALID_PARAMETER);
				return;
			}
		} else if (cmd_length == 5) {
			if (memcmp(cmd, "flush", cmd_length) == 0) {
				process_flush();
			} else if (memcmp(cmd, "touch", cmd_length) == 0) {
				process_touch();
			} else if (memcmp(cmd, "stats", cmd_length) == 0) {
				process_stats();
			} else {
				LOG_WARNING2("try_read_command error unkown request=" << http_request_.uri);
				write_error(XIXI_REASON_INVALID_PARAMETER);
				return;
			}
		} else if (cmd_length == 6) {
			if (memcmp(cmd, "append", cmd_length) == 0) {
				process_update(XIXI_UPDATE_SUB_OP_APPEND);
			} else {
				LOG_WARNING2("try_read_command error unkown request=" << http_request_.uri);
				write_error(XIXI_REASON_INVALID_PARAMETER);
				return;
			}
		} else if (cmd_length == 7) {
			if (memcmp(cmd, "replace", cmd_length) == 0) {
				process_update(XIXI_UPDATE_SUB_OP_REPLACE);
			} else if (memcmp(cmd, "prepend", cmd_length) == 0) {
				process_update(XIXI_UPDATE_SUB_OP_PREPEND);
			} else {
				LOG_WARNING2("try_read_command error unkown request=" << http_request_.uri);
				write_error(XIXI_REASON_INVALID_PARAMETER);
				return;
			}
		} else {
			LOG_WARNING2("try_read_command error unkown cmd_length=" << cmd_length);
			write_error(XIXI_REASON_INVALID_PARAMETER);
		}
	} else {
		key_ = http_request_.uri;
		key_length_ = http_request_.uri_length;
		process_get();
	}
	if (!http_request_.http_11) {
		next_state_ = PEER_STATE_CLOSING;
	}
}

bool Peer_Http::process_request_arg(char* arg) {
	tokens_.clear();
	tokenize_command(arg, tokens_);

	for (size_t i = 0; i < tokens_.size(); i++) {
		token_t& t = tokens_[i];
		if (t.length >= 3) {
			if (memcmp(t.value, "g=", 2) == 0) {
				if (!safe_toui32(t.value + 2, t.length, group_id_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (memcmp(t.value, "w=", 2) == 0) {
				if (!safe_toui32(t.value + 2, t.length, watch_id_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (memcmp(t.value, "k=", 2) == 0) {
				key_ = decode_uri(t.value + 2, t.length - 2, key_length_);
				if (key_ == NULL) {
					write_error(XIXI_REASON_OUT_OF_MEMORY);
					return false;
				}
//				key_length_ = t.length - 2;
			} else if (memcmp(t.value, "v=", 2) == 0) {
			//	value_ = t.value + 2;
			//	value_length_ = t.length - 2;
				value_ = decode_uri(t.value + 2, t.length - 2, value_length_);
				if (value_ == NULL) {
					write_error(XIXI_REASON_OUT_OF_MEMORY);
					return false;
				}
			} else if (memcmp(t.value, "f=", 2) == 0) {
				if (!safe_toui32(t.value + 2, t.length, flags_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (memcmp(t.value, "c=", 2) == 0) {
				if (!safe_toui64(t.value + 2, t.length, cache_id_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (memcmp(t.value, "d=", 2) == 0) {
				if (!safe_toi64(t.value + 2, t.length, delta_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (memcmp(t.value, "e=", 2) == 0) {
				touch_flag_ = true;
				if (!safe_toui32(t.value + 2, t.length, expiration_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			}
		}
	}
	return true;
}

void Peer_Http::process_post() {
	if (http_request_.content_type_length == 33
			&& memcmp(http_request_.content_type, "application/x-www-form-urlencoded", http_request_.content_type_length) == 0) {

		if (!process_request_arg((char*)post_data_)) {
			LOG_WARNING2("process_cahce_arg failed");
			write_error(XIXI_REASON_INVALID_PARAMETER);
			return;
		}
		process_command();
	} if (http_request_.content_type_length == 19
			&& memcmp(http_request_.content_type, "multipart/form-data", http_request_.content_type_length) == 0) {
		char* buf = (char*)memfind((char*)post_data_, content_length_, (char*)http_request_.boundary, http_request_.boundary_length);
		while (buf != NULL) {
			buf += http_request_.boundary_length + 2;
			buf = strstr(buf, " name=\"");
			if (buf != NULL) {
				char* name = buf + 7;
				uint32_t name_length = 0;
				char* end = strstr(name, "\"");
				if (end != NULL) {
					name_length = end - name;
				}
				
				char* value = strstr(name + 2, "\r\n\r\n");
				if (value != NULL) {
					value += 4;
				}
				
				buf = (char*)memfind((char*)value, content_length_ - (value - (char*)post_data_), (char*)http_request_.boundary, http_request_.boundary_length);
				if (buf == NULL) {
					break;
				}
				if (name_length = 1) {
					if (name[0] == 'g') {
						if (!safe_toui32(value, buf - value - 4, group_id_)) {
							write_error(XIXI_REASON_INVALID_PARAMETER);
							return;
						}
					} else if (name[0] == 'w') {
						if (!safe_toui32(value, buf - value - 4, watch_id_)) {
							write_error(XIXI_REASON_INVALID_PARAMETER);
							return;
						}
					} else if (name[0] == 'k') {
						key_ = value;
						key_length_ = buf - value - 4;
					} else if (name[0] == 'v') {
						value_ = value;
						value_length_ = buf - value - 4;
					} else if (name[0] == 'f') {
						if (!safe_toui32(value, buf - value - 4, flags_)) {
							write_error(XIXI_REASON_INVALID_PARAMETER);
							return;
						}
					} else if (name[0] == 'c') {
						if (!safe_toui64(value, buf - value - 4, cache_id_)) {
							write_error(XIXI_REASON_INVALID_PARAMETER);
							return;
						}
					} else if (name[0] == 'd') {
						if (!safe_toi64(value, buf - value - 4, delta_)) {
							write_error(XIXI_REASON_INVALID_PARAMETER);
							return;
						}
					} else if (name[0] == 'e') {
						touch_flag_ = true;
						if (!safe_toui32(value, buf - value - 4, expiration_)) {
							write_error(XIXI_REASON_INVALID_PARAMETER);
							return;
						}
					}
				}
			} else {
				break;
			}
		}
		process_command();
	} else {
		LOG_WARNING2("process_cahce_arg unsupport content_type " << http_request_.content_type);
		write_error(XIXI_REASON_INVALID_PARAMETER);
	}
}

void Peer_Http::process_get() {

	Cache_Item* it;
	bool watch_error = false;
	if (touch_flag_) {
		it = cache_mgr_.get_touch(group_id_, (uint8_t*)key_, key_length_, watch_id_, expiration_, watch_error);
	} else {
		it = cache_mgr_.get(group_id_, (uint8_t*)key_, key_length_, watch_id_, expiration_, watch_error);
	}	

	if (it != NULL) {
		cache_item_ = it;
		cache_items_.push_back(it);

		uint8_t* buf = request_buf_.prepare(20);
		uint32_t data_size = _snprintf((char*)buf, 20, "%lu\r\n\r\n", it->data_size);
//			char* res = "HTTP/1.1 200 OK\r\n\r\n""Connection: keep-alive\r\n""Via: 1.1 hkidc-dmz-wsa-2.cisco.com:80 (IronPort-WSA/7.1.3-013)\r\n""Vary: Accept-Encoding\r\n""Date: Tue, 25 Oct 2011 09:24:11 GMT\r\n";
		char* res = "HTTP/1.1 200 OK\r\n"
			"Server: xixibase/0.1\r\n"
			"Content-Type: text/html\r\n"
			"Content-Length: ";//21\r\n\r\n"
		//	"<html>xixibase</html>";
		add_write_buf((uint8_t*)res, strlen(res));
		add_write_buf(buf, data_size);
		add_write_buf(it->get_data(), it->data_size);
		set_state(PEER_STATUS_WRITE);
		next_state_ = PEER_STATE_NEW_CMD;
	} else {
		if (watch_error) {
		//	write_error(XIXI_REASON_WATCH_NOT_FOUND, 0, true);
/*			char* res = "HTTP/1.1 200 OK\r\n"
				"Server: xixibase/0.1\r\n"
				"Content-Type: text/html\r\n"
				"Content-Length: 27\r\n\r\n"
				"XIXI_REASON_WATCH_NOT_FOUND";
			add_write_buf((uint8_t*)res, strlen(res));
			set_state(PEER_STATUS_WRITE);
			next_state_ = PEER_STATE_NEW_CMD;*/
			write_error(XIXI_REASON_WATCH_NOT_FOUND);
		} else {
		//	write_error(XIXI_REASON_NOT_FOUND, 0, true);
/*			char* res = "HTTP/1.1 200 OK\r\n"
				"Server: xixibase/0.1\r\n"
				"Content-Type: text/html\r\n"
				"Content-Length: 21\r\n\r\n"
				"XIXI_REASON_NOT_FOUND";
			add_write_buf((uint8_t*)res, strlen(res));
			set_state(PEER_STATUS_WRITE);
			next_state_ = PEER_STATE_NEW_CMD;*/
			write_error(XIXI_REASON_NOT_FOUND);
		}
	}
}

void Peer_Http::process_update(uint8_t sub_op) {

	uint32_t data_len = key_length_ + value_length_;

	cache_item_ = cache_mgr_.alloc_item(group_id_, key_length_, flags_,
		expiration_, value_length_);

	if (cache_item_ == NULL) {
		if (cache_mgr_.item_size_ok(key_length_, value_length_)) {
			write_error(XIXI_REASON_OUT_OF_MEMORY);
		} else {
			write_error(XIXI_REASON_TOO_LARGE);
		}
		return;
	}

	memcpy(cache_item_->get_key(), key_, key_length_);
	memcpy(cache_item_->get_data(), value_, value_length_);
//	uint8_t* key = cache_item_->get_key();
//	uint8_t* data = cache_item_->get_data();

//	uint32_t data_len = key_length_ + value_length_;

	cache_item_->calc_hash_value();

	cache_item_->cache_id = cache_id_;

	uint64_t cache_id;
	xixi_reason reason;

	switch (sub_op) {
	case XIXI_UPDATE_SUB_OP_SET:
		reason = cache_mgr_.set(cache_item_, watch_id_, cache_id);
		break;
	case XIXI_UPDATE_SUB_OP_ADD:
		reason = cache_mgr_.add(cache_item_, watch_id_, cache_id);
		break;
	case XIXI_UPDATE_SUB_OP_REPLACE:
		reason = cache_mgr_.replace(cache_item_, watch_id_, cache_id);
		break;
	case XIXI_UPDATE_SUB_OP_APPEND:
		reason = cache_mgr_.append(cache_item_, watch_id_, cache_id);
		break;
	case XIXI_UPDATE_SUB_OP_PREPEND:
		reason = cache_mgr_.prepend(cache_item_, watch_id_, cache_id);
		break;
	default:
		reason = XIXI_REASON_UNKNOWN_COMMAND;
		break;
	}

	if (reason == XIXI_REASON_SUCCESS) {
		uint8_t* buf = request_buf_.prepare(24);
		uint32_t data_size = _snprintf((char*)buf, 24, "%llu", cache_id);
		uint8_t* buf2 = request_buf_.prepare(50);
		uint32_t data_size2 = _snprintf((char*)buf2, 50, "%lu\r\n\r\n%s", data_size, buf);
		char* res = "HTTP/1.1 200 OK\r\n"
			"Server: "VERSION"\r\n"
			"Content-Type: text/html\r\n"
			"Content-Length: ";
		add_write_buf((uint8_t*)res, strlen(res));
		add_write_buf(buf2, data_size2);
		set_state(PEER_STATUS_WRITE);
		next_state_ = PEER_STATE_NEW_CMD;
	} else {
		write_error(reason);
	}
	cache_mgr_.release_reference(cache_item_);
	cache_item_ = NULL;
}

void Peer_Http::process_delete() {
	LOG_TRACE2("process_delete");

	LOG_DEBUG2("Deleteing " << string((char*)key_, key_length_));

	xixi_reason reason = cache_mgr_.remove(group_id_, (uint8_t*)key_, key_length_, cache_id_);
	if (reason == XIXI_REASON_SUCCESS) {
		write_simple_res(XIXI_CHOICE_DELETE_RES);
	} else {
		write_error(reason);
	}
}

void Peer_Http::process_delta(bool incr) {
	LOG_TRACE2("process_delta");

	LOG_DEBUG2("Delta " << string((char*)key_, key_length_));

	int64_t value;
//	uint64_t cache_id = pdu->cache_id;
	xixi_reason reason = cache_mgr_.delta(group_id_, (uint8_t*)key_, key_length_, incr, delta_, cache_id_, value);
	if (reason == XIXI_REASON_SUCCESS) {
		uint8_t* cb = request_buf_.prepare(XIXI_Delta_Res_Pdu::calc_encode_size());
		XIXI_Delta_Res_Pdu::encode(cb, value, cache_id_);

		add_write_buf(cb, XIXI_Delta_Res_Pdu::calc_encode_size());

		set_state(PEER_STATUS_WRITE);
		next_state_ = PEER_STATE_NEW_CMD;
	} else {
		write_error(reason);
	}
}

uint32_t Peer_Http::process_get_base_req_pdu_extras(XIXI_Get_Base_Req_Pdu* pdu, uint8_t* data, uint32_t data_length) {
	LOG_TRACE2("process_get_base_req_pdu_extras");
	uint8_t* key = data;
	size_t key_length = pdu->key_length;
	if (data_length < key_length) {
		return 0;
	}

	uint64_t cache_id;
	uint32_t flags;
	uint32_t expiration;
	bool ret = cache_mgr_.get_base(pdu->group_id, key, key_length, cache_id, flags, expiration);
	if (ret) {
		uint8_t* cb = request_buf_.prepare(XIXI_Get_Base_Res_Pdu::calc_encode_size());
		XIXI_Get_Base_Res_Pdu rs;
		rs.cache_id = cache_id;
		rs.flags = flags;
		rs.expiration = expiration;
		rs.encode(cb);

		add_write_buf(cb, XIXI_Get_Base_Res_Pdu::calc_encode_size());

		set_state(PEER_STATUS_WRITE);
		next_state_ = PEER_STATE_NEW_CMD;
	} else {
//		write_error(XIXI_REASON_NOT_FOUND, 0, true);
	}

	return key_length;
}

uint32_t Peer_Http::process_update_flags_req_pdu_extras(XIXI_Update_Flags_Req_Pdu* pdu, uint8_t* data, uint32_t data_length) {
	LOG_TRACE2("process_update_flags_req_pdu_extras");
	uint8_t* key = data;
	size_t key_length = pdu->key_length;
	if (data_length < key_length) {
		return 0;
	}
	uint64_t cache_id = 0;
	bool ret = cache_mgr_.update_flags(pdu->group_id, key, key_length, pdu, cache_id);
	if (pdu->reply()) {
		if (ret) {
			uint8_t* cb = request_buf_.prepare(XIXI_Update_Flags_Res_Pdu::calc_encode_size());
			XIXI_Update_Flags_Res_Pdu::encode(cb, cache_id);

			add_write_buf(cb, XIXI_Update_Flags_Res_Pdu::calc_encode_size());

			set_state(PEER_STATUS_WRITE);
			next_state_ = PEER_STATE_NEW_CMD;
		} else {
			if (cache_id != 0) {
//				write_error(XIXI_REASON_MISMATCH, 0, true);
			} else {
	//			write_error(XIXI_REASON_NOT_FOUND, 0, true);
			}
		}
	} else {
		set_state(PEER_STATE_NEW_CMD);
		next_state_ = PEER_STATE_NEW_CMD;
	}
	return key_length;
}

void Peer_Http::process_touch() {
	LOG_TRACE2("process_touch");

	XIXI_Update_Expiration_Req_Pdu pdu;
	uint64_t cache_id = 0;
	bool ret = cache_mgr_.update_expiration(group_id_, (uint8_t*)key_, key_length_, &pdu, cache_id);
	if (ret) {
		uint8_t* cb = request_buf_.prepare(XIXI_Update_Expiration_Res_Pdu::calc_encode_size());
		XIXI_Update_Expiration_Res_Pdu::encode(cb, cache_id);

		add_write_buf(cb, XIXI_Update_Expiration_Res_Pdu::calc_encode_size());

		set_state(PEER_STATUS_WRITE);
		next_state_ = PEER_STATE_NEW_CMD;
	} else {
		if (cache_id != 0) {
			write_error(XIXI_REASON_MISMATCH);
		} else {
			write_error(XIXI_REASON_NOT_FOUND);
		}
	}
}

void Peer_Http::process_auth_req_pdu_fixed(XIXI_Auth_Req_Pdu* pdu) {
	LOG_TRACE2("process_auth_req_pdu_fixed");
	next_data_len_ = pdu->base64_length;
	set_state(PEER_STATE_READ_BODY_EXTRAS2);
}

uint32_t Peer_Http::process_auth_req_pdu_extras(XIXI_Auth_Req_Pdu* pdu, uint8_t* data, uint32_t data_length) {
	LOG_TRACE2("process_auth_req_pdu_extras");
	uint8_t* base64 = data;
	uint32_t base64_length = pdu->base64_length;

	if (data_length < pdu->base64_length) {
		return 0;
	}

	std::string out;
	auth_.login(pdu->base64, pdu->base64_length, out);

	return base64_length;
}

void Peer_Http::process_hello_req_pdu_fixed() {
	LOG_TRACE2("process_hello_req_pdu_fixed");
	write_simple_res(XIXI_CHOICE_HELLO_RES);
}

void Peer_Http::process_create_watch_req_pdu_fixed(XIXI_Create_Watch_Req_Pdu* pdu) {
	LOG_INFO2("process_create_watch_req_pdu_fixed");
	uint32_t watchID = cache_mgr_.create_watch(pdu->group_id, pdu->max_next_check_interval);

	uint8_t* cb = request_buf_.prepare(XIXI_Create_Watch_Res_Pdu::get_body_size());
	XIXI_Create_Watch_Res_Pdu::encode(cb, watchID);

	add_write_buf(cb, XIXI_Create_Watch_Res_Pdu::get_body_size());

	set_state(PEER_STATUS_WRITE);
	next_state_ = PEER_STATE_NEW_CMD;
}

void Peer_Http::process_check_watch_req_pdu_fixed(XIXI_Check_Watch_Req_Pdu* pdu) {
	std::list<uint64_t> updated_list;
	uint32_t updated_count = 0;
	boost::shared_ptr<Cache_Watch_Sink> sp = self_;
	bool ret = cache_mgr_.check_watch_and_set_callback(pdu->group_id, pdu->watch_id, updated_list, updated_count, pdu->ack_cache_id, sp, pdu->max_next_check_interval);
	//  LOG_INFO2("process_check_watch_req_pdu_fixed watch_id=" << pdu->watch_id << " ack=" << pdu->ack_cache_id << " updated_count=" << updated_count);

	if (!ret) {
//		write_error(XIXI_REASON_WATCH_NOT_FOUND, 0, true);
	} else {
		if (updated_count > 0) {
			uint32_t size = XIXI_Check_Watch_Res_Pdu::calc_encode_size(updated_count);

			uint8_t* buf = request_buf_.prepare(size);
			if (buf != NULL) {
				XIXI_Check_Watch_Res_Pdu::encode(buf, updated_count, updated_list);

				add_write_buf(buf, size);

				set_state(PEER_STATUS_WRITE);
				next_state_ = PEER_STATE_NEW_CMD;
			} else {
//				write_error(XIXI_REASON_OUT_OF_MEMORY, 0, true);
			}
		} else {
			//    LOG_INFO2("process_check_watch_req_pdu_fixed wait a moment watch_id=" << pdu->watch_id << " updated_count=" << updated_count);
			timer_ = new boost::asio::deadline_timer(socket_->get_io_service());
			timer_->expires_from_now(boost::posix_time::seconds(pdu->check_timeout));
			timer_->async_wait(boost::bind(&Peer_Http::handle_timer, this,
				boost::asio::placeholders::error, pdu->watch_id));
			set_state(PEER_STATUS_ASYNC_WAIT);
		}
	}
}

void Peer_Http::process_flush() {
	uint32_t flush_count = 0;
	uint64_t flush_size = 0;
	cache_mgr_.flush(group_id_, flush_count, flush_size);

	uint8_t* cb = request_buf_.prepare(XIXI_Flush_Res_Pdu::calc_encode_size());

//	res_pdu.encode(cb);

	add_write_buf(cb, XIXI_Flush_Res_Pdu::calc_encode_size());

	set_state(PEER_STATUS_WRITE);
	next_state_ = PEER_STATE_NEW_CMD;
}

void Peer_Http::process_stats() {
	std::string result;
	XIXI_Stats_Req_Pdu pdu;
	cache_mgr_.stats(&pdu, result);
	int size = result.size();
	int buf_size = XIXI_Stats_Res_Pdu::calc_encode_size(size);
	uint8_t* buf = request_buf_.prepare(buf_size);
	if (buf != NULL) {
		XIXI_Stats_Res_Pdu::encode(buf, (uint8_t*)result.c_str(), size);

		add_write_buf(buf, buf_size);

		set_state(PEER_STATUS_WRITE);
		next_state_ = PEER_STATE_NEW_CMD;
	} else {
		write_error(XIXI_REASON_OUT_OF_MEMORY);
	}
}

void Peer_Http::reset_for_new_cmd() {
	cache_item_ = NULL;
	for (uint32_t i = 0; i < cache_items_.size(); i++) {
		cache_mgr_.release_reference(cache_items_[i]);
	}
	cache_items_.clear();

	group_id_ = 0;
	watch_id_ = 0;
	cache_id_ = 0;
	key_ = NULL;
	key_length_ = 0;
	value_ = NULL;
	value_length_ = 0;
	post_data_ = NULL;
	content_length_ = 0;
	flags_ = 0;
	expiration_ = 0;
	touch_flag_ = false;
	delta_ = 1;
	http_request_.reset();

	request_buf_.reset();
	write_buf_total_ = 0;
	read_item_buf_ = NULL;
	next_data_len_ = XIXI_PDU_HEAD_LENGTH;
	set_state(PEER_STATE_READ_HEADER);
}

void Peer_Http::start(uint8_t* data, uint32_t data_length) {
	lock_.lock();
	if (read_buffer_.get_read_buf_size() >= data_length) {
		memcpy(read_buffer_.get_read_buf(), data, data_length);
		read_buffer_.read_data_size_ += data_length;

		process();

		if (state_ != PEER_STATUS_ASYNC_WAIT) {
			if (!is_closed()) {
				if (!try_write()) {
					try_read();
				}
			}
		} else {
			try_write();
		}
	} else {
		LOG_INFO2("start invalid parameter:data_length=" << data_length);
	}
	bool closed = (op_count_ == 0 && state_ != PEER_STATUS_ASYNC_WAIT);
	lock_.unlock();
	if (closed) {
		LOG_INFO2("start closed");
		self_.reset();
	}
}

void Peer_Http::handle_read(const boost::system::error_code& err, size_t length) {
	LOG_TRACE2("handle_read length=" << length << " err=" << err.message() << " err_value=" << err.value());
	lock_.lock();
	--op_count_;
	if (!err) {
		read_buffer_.read_data_size_ += length;

		process();

		if (state_ != PEER_STATUS_ASYNC_WAIT) {
			if (!is_closed()) {
				if (!try_write()) {
					try_read();
				}
			} else {
				LOG_DEBUG2("handle_read peer is closed, op_count_=" << op_count_);
			}
		} else {
			try_write();
		}
	} else {
		LOG_DEBUG2("handle_read error op_count_=" << op_count_ << " err=" << err);
	}

	bool closed = (op_count_ == 0 && state_ != PEER_STATUS_ASYNC_WAIT);
	lock_.unlock();
	if (closed) {
		self_.reset();
	}
}

void Peer_Http::handle_write(const boost::system::error_code& err) {
	LOG_TRACE2("handle_write err=" << err.message() << " err_value=" << err.value());
	lock_.lock();
	--op_count_;
	if (!err) {
		write_buf_.clear();

		process();

		if (state_ != PEER_STATUS_ASYNC_WAIT) {
			if (!is_closed()) {
				if (!try_write()) {
					try_read();
				}
			}
		} else {
			try_write();
		}
	}

	bool closed = (op_count_ == 0 && state_ != PEER_STATUS_ASYNC_WAIT);
	lock_.unlock();
	if (closed) {
		self_.reset();
	}
}

void Peer_Http::try_read() {
//	if (op_count_ == 0) {
		++op_count_;
		read_buffer_.handle_processed();
		socket_->async_read_some(boost::asio::buffer(read_buffer_.get_read_buf(), read_buffer_.get_read_buf_size()),
			make_custom_alloc_handler(handler_allocator_,
			boost::bind(&Peer_Http::handle_read, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred)));
		LOG_TRACE2("try_read async_read_some get_read_buf_size=" << read_buffer_.get_read_buf_size());
//	}
}

bool Peer_Http::try_write() {
//	if (op_count_ == 0) {
		if (!write_buf_.empty()) {
			++op_count_;
			async_write(*socket_, write_buf_,
				make_custom_alloc_handler(handler_allocator_,
				boost::bind(&Peer_Http::handle_write, this,
				boost::asio::placeholders::error)));
			LOG_TRACE2("try_write async_write write_buf.count=" << write_buf_.size());
			return true;
		}
//	}
	return false;
}

uint32_t Peer_Http::read_some(uint8_t* buf, uint32_t length) {
	boost::system::error_code ec;
	if (socket_->available(ec) == 0) {
		return 0;
	}
	return socket_->read_some(boost::asio::buffer(buf, length), ec);
}

void Peer_Http::handle_timer(const boost::system::error_code& err, uint32_t watch_id) {
	//  LOG_INFO("Peer_Http::handle_timer err=" << err);
	std::list<uint64_t> updated_list;
	uint32_t updated_count = 0;
	lock_.lock();
	bool ret = cache_mgr_.check_watch_and_clear_callback(watch_id, updated_list, updated_count);
	//  LOG_INFO2("handle_timer watch_id=" << watch_id << " updated_count=" << updated_count);

	if (!ret) {
		write_error(XIXI_REASON_WATCH_NOT_FOUND);
	} else {
		uint32_t size = XIXI_Check_Watch_Res_Pdu::calc_encode_size(updated_count);

		uint8_t* buf = request_buf_.prepare(size);
		if (buf != NULL) {
			XIXI_Check_Watch_Res_Pdu::encode(buf, updated_count, updated_list);

			add_write_buf(buf, size);

			set_state(PEER_STATE_NEW_CMD);
			next_state_ = PEER_STATE_NEW_CMD;
			try_write();
		} else {
			write_error(XIXI_REASON_OUT_OF_MEMORY);
		}
	}

	delete timer_;
	timer_ = NULL;
	lock_.unlock();
}

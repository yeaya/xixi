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

#include "peer_http.h"
#include "settings.h"
#include "currtime.h"
#include "stats.h"
#include "log.h"
#include "auth.h"
#include "server.h"

#define READ_BUFFER_HIGHWAT 8192
#define DATA_BUFFER_SIZE 2048

#define DEFAULT_RES_200 "HTTP/1.1 200 OK\r\nServer: "HTTP_SERVER"\r\nContent-Type: text/html\r\nContent-Length: "

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
// a: ack_cache_id
// i: interval
// t: timeout
// s: sub op

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

// data: must not be NULL
// length: must > 0
// sub: must not be NULL
// sub_len: must > 0
char* memfind(char* data, uint32_t length, const char* sub, uint32_t sub_len) {
	if (length < sub_len) {
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

void tokenize_command(char* command, vector<token_t>& tokens) {
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

void to_lower(char* buf, uint32_t length) {
	for (uint32_t i = 0; i < length; i++) {
		char c = buf[i];
		if (c >= 'A' && c <= 'Z') {
			buf[i] = c + ('a' - 'A');
		}
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
		if (c == '\0') { // c == ' ' ||  // c == '\r' || c == '\n' || 
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
			i += 2;
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
	ack_cache_id_ = 0;
	interval_ = 30;
	timeout_ = 10;
	sub_op_ = 0;

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

void Peer_Http::write_error(xixi_reason error_code) {
	write_buf_.clear();
	write_buf_total_ = 0;

	const char* res = NULL;
	uint32_t res_size = 0;
	switch (error_code) {
		case XIXI_REASON_NOT_FOUND:
#define ERROR_XIXI_REASON_NOT_FOUND "HTTP/1.1 404 Not Found\r\n""Server: "HTTP_SERVER"\r\n"	"Content-Type: text/html\r\n""Content-Length: 21\r\n\r\n""XIXI_REASON_NOT_FOUND"
			res = ERROR_XIXI_REASON_NOT_FOUND;
			res_size = sizeof(ERROR_XIXI_REASON_NOT_FOUND) - 1;
			break;
		case XIXI_REASON_EXISTS:
#define ERROR_XIXI_REASON_EXISTS "HTTP/1.1 405 Method Not Allowed\r\n""Server: "HTTP_SERVER"\r\n""Content-Type: text/html\r\n""Content-Length: 18\r\n\r\n""XIXI_REASON_EXISTS"
			res = ERROR_XIXI_REASON_EXISTS;
			res_size = sizeof(ERROR_XIXI_REASON_EXISTS) - 1;
			break;
		case XIXI_REASON_TOO_LARGE:
#define ERROR_XIXI_REASON_TOO_LARGE "HTTP/1.1 413 Request Entity Too Large\r\n""Server: "HTTP_SERVER"\r\n""Content-Type: text/html\r\n""Content-Length: 21\r\n\r\n""XIXI_REASON_TOO_LARGE"
			res = ERROR_XIXI_REASON_TOO_LARGE;
			res_size = sizeof(ERROR_XIXI_REASON_TOO_LARGE) - 1;
			break;
		case XIXI_REASON_INVALID_PARAMETER:
#define ERROR_XIXI_REASON_INVALID_PARAMETER "HTTP/1.1 400 Bad Request\r\n""Server: "HTTP_SERVER"\r\n""Content-Type: text/html\r\n""Content-Length: 29\r\n\r\n""XIXI_REASON_INVALID_PARAMETER"
			res = ERROR_XIXI_REASON_INVALID_PARAMETER;
			res_size = sizeof(ERROR_XIXI_REASON_INVALID_PARAMETER) - 1;
			break;
		case XIXI_REASON_INVALID_OPERATION:
#define ERROR_XIXI_REASON_INVALID_OPERATION "HTTP/1.1 400 Bad Request\r\n""Server: "HTTP_SERVER"\r\n""Content-Type: text/html\r\n""Content-Length: 29\r\n\r\n""XIXI_REASON_INVALID_OPERATION"
			res = ERROR_XIXI_REASON_INVALID_OPERATION;
			res_size = sizeof(ERROR_XIXI_REASON_INVALID_OPERATION) - 1;
			break;
		case XIXI_REASON_MISMATCH:
#define ERROR_XIXI_REASON_MISMATCH "HTTP/1.1 400 Bad Request\r\n""Server: "HTTP_SERVER"\r\n""Content-Type: text/html\r\n""Content-Length: 20\r\n\r\n""XIXI_REASON_MISMATCH"
			res = ERROR_XIXI_REASON_MISMATCH;
			res_size = sizeof(ERROR_XIXI_REASON_MISMATCH) - 1;
			break;
		case XIXI_RES_AUTH_ERROR:
#define ERROR_XIXI_RES_AUTH_ERROR "HTTP/1.1 401 Unauthorized\r\n""Server: "HTTP_SERVER"\r\n""Content-Type: text/html\r\n""Content-Length: 19\r\n\r\n""XIXI_RES_AUTH_ERROR"
			res = ERROR_XIXI_RES_AUTH_ERROR;
			res_size = sizeof(ERROR_XIXI_RES_AUTH_ERROR) - 1;
			break;
		case XIXI_REASON_UNKNOWN_COMMAND:
#define ERROR_XIXI_REASON_UNKNOWN_COMMAND "HTTP/1.1 400 Bad Request\r\n""Server: "HTTP_SERVER"\r\n""Content-Type: text/html\r\n""Content-Length: 28\r\n\r\n""XIXI_REASON_UNKNOWN_COMMAND"
			res = ERROR_XIXI_REASON_UNKNOWN_COMMAND;
			res_size = sizeof(ERROR_XIXI_REASON_UNKNOWN_COMMAND) - 1;
			break;
		case XIXI_REASON_OUT_OF_MEMORY:
#define ERROR_XIXI_REASON_OUT_OF_MEMORY "HTTP/1.1 500 Internal Server Error\r\n""Server: "HTTP_SERVER"\r\n""Content-Type: text/html\r\n""Content-Length: 25\r\n\r\n""XIXI_REASON_OUT_OF_MEMORY"
			res = ERROR_XIXI_REASON_OUT_OF_MEMORY;
			res_size = sizeof(ERROR_XIXI_REASON_OUT_OF_MEMORY) - 1;
			break;
		case XIXI_REASON_WATCH_NOT_FOUND:
#define ERROR_XIXI_REASON_WATCH_NOT_FOUND "HTTP/1.1 400 Bad Request\r\n""Server: "HTTP_SERVER"\r\n""Content-Type: text/html\r\n""Content-Length: 27\r\n\r\n""XIXI_REASON_WATCH_NOT_FOUND"
			res = ERROR_XIXI_REASON_WATCH_NOT_FOUND;
			res_size = sizeof(ERROR_XIXI_REASON_WATCH_NOT_FOUND) - 1;
			break;
	}

	add_write_buf((uint8_t*)res, res_size);
	set_state(PEER_STATUS_WRITE);
	next_state_ = PEER_STATE_CLOSING;
}

void Peer_Http::process() {
	LOG_TRACE2("process length=" << read_buffer_.read_data_size_);

	uint32_t tmp = 0;
	uint32_t process_reqest_count = 0;
	bool run = true;

	while (run) {

		LOG_TRACE2("process while state_" << (uint32_t)state_);

		switch (state_) {

		case PEER_STATE_NEW_CMD:
			reset_for_new_cmd();
			break;

		case PEER_STATE_READ_HEADER:
			tmp = try_read_command((char*)read_buffer_.read_curr_, read_buffer_.read_data_size_);
			if (tmp > 0) {
				process_reqest_count++;
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
				read_item_buf_ += tmp;
				next_data_len_ -= tmp;
				read_buffer_.read_curr_ += tmp;
				read_buffer_.read_data_size_ -= tmp;
			} else {
				uint32_t size = read_some(read_item_buf_, next_data_len_);
				if (size == 0) {
					run = false;
				} else {
					read_item_buf_ += size;
					next_data_len_ -= size;
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
				if (read_buffer_.read_data_size_ >= 10) {
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
			run = false;
			break;

		default:
			assert(false);
			run = false;
			break;
		}
	}
}

void Peer_Http::set_state(peer_state state) {
	LOG_TRACE("state change, from " << (uint32_t)state_ << " to " << (uint32_t)state);
	state_ = state;
}

uint32_t Peer_Http::try_read_command(char* data, uint32_t data_len) {
	char* p = memfind(data, data_len, "\r\n\r\n", 4);
	if (p != NULL) {
		*p = '\0';

		assert(p < (data + data_len));

		process_request_header(data, (uint32_t)(p - data));

		return (p - data + 4);
	}

	if (data_len >= 4096) {
		LOG_WARNING2("try_read_command header too large > " << data_len);
		write_error(XIXI_REASON_TOO_LARGE);
		return data_len; // ?
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
			LOG_WARNING2("process_request_header error method=" << method);
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
		
		if (!process_request_header_fields(request_header_fields, request_header_fields_length)) {
			LOG_WARNING2("process_request_header failed on process request header fields " << request_header_fields);
			write_error(XIXI_REASON_INVALID_PARAMETER);
			return;
		}
		if (method == POST_METHOD && content_length_ > 0) {
			if (content_length_ > settings_.item_size_max * 3) {
				LOG_WARNING2("process_request_header the content length is too large " << content_length_);
				write_error(XIXI_REASON_TOO_LARGE);
				return;
			}
			post_data_ = request_buf_.prepare(content_length_ + 1);
			if (post_data_ == NULL) {
				LOG_WARNING2("process_request_header out of memory " << content_length_);
				write_error(XIXI_REASON_OUT_OF_MEMORY);
				return;
			}
			post_data_[content_length_] = '\0';
			read_item_buf_ = post_data_;
			next_data_len_ = content_length_;
			set_state(PEER_STATE_READ_BODY_EXTRAS);

			char* buf = (char*)request_buf_.prepare(http_request_.uri_length + 1);
			if (buf == NULL) {
				LOG_WARNING2("process_request_header uri out of memory " << (http_request_.uri_length + 1));
				write_error(XIXI_REASON_OUT_OF_MEMORY);
			}
			memcpy(buf, http_request_.uri, http_request_.uri_length);
			buf[http_request_.uri_length] = '\0';
			http_request_.uri = buf;
		} else {
			process_command();
		}
	} else {
		LOG_WARNING2("process_request_header error unkown request=" << request_header);
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
		if (p == NULL) {
			break;
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

	return true;
}

bool Peer_Http::handle_request_header_field(char* name, uint32_t name_length, char* value, uint32_t value_length) {
	if (name_length == 12) {
		to_lower(name, name_length);
		if (memcmp(name, "content-type", name_length) == 0) {
			char* buf = (char*)request_buf_.prepare(value_length + 1);
			if (buf == NULL) {
				return false;
			}
			memcpy(buf, value, value_length);
			buf[value_length] = '\0';
			to_lower(buf, value_length);
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
		to_lower(name, name_length);
		if (memcmp(name, "content-length", name_length) == 0) {
			if (!safe_toui32(value, value_length, content_length_)) {
				return false;
			}
		}
	}
	return true;
}

void Peer_Http::process_command() {
	if (http_request_.uri_length >= 10 && memcmp(http_request_.uri, "/xixibase/", 10) == 0) {
		uint32_t cmd_length = 0;
		char* arg = strrchr(http_request_.uri, '?');
		if (arg != NULL) {
			cmd_length = arg - http_request_.uri - 10;
			if (!process_request_arg(arg + 1)) {
				LOG_WARNING2("process_command failed arg=" << (arg + 1));
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
				LOG_WARNING2("process_command error unkown request=" << http_request_.uri);
				write_error(XIXI_REASON_INVALID_PARAMETER);
				return;
			}
		} else if (cmd_length == 4) {
			if (memcmp(cmd, "incr", cmd_length) == 0) {
				process_delta(true);
			} else if (memcmp(cmd, "decr", cmd_length) == 0) {
				process_delta(false);
			} else {
				LOG_WARNING2("process_command error unkown request=" << http_request_.uri);
				write_error(XIXI_REASON_INVALID_PARAMETER);
				return;
			}
		} else if (cmd_length == 5) {
			if (memcmp(cmd, "flush", cmd_length) == 0) {
				process_flush();
			} else if (memcmp(cmd, "touch", cmd_length) == 0) {
				process_touch();
			} else if (memcmp(cmd, "flags", cmd_length) == 0) {
				process_update_flags();
			} else if (memcmp(cmd, "stats", cmd_length) == 0) {
				process_stats();
			} else {
				LOG_WARNING2("process_command error unkown request=" << http_request_.uri);
				write_error(XIXI_REASON_INVALID_PARAMETER);
				return;
			}
		} else if (cmd_length == 6) {
			if (memcmp(cmd, "append", cmd_length) == 0) {
				process_update(XIXI_UPDATE_SUB_OP_APPEND);
			} else {
				LOG_WARNING2("process_command error unkown request=" << http_request_.uri);
				write_error(XIXI_REASON_INVALID_PARAMETER);
				return;
			}
		} else if (cmd_length == 7) {
			if (memcmp(cmd, "replace", cmd_length) == 0) {
				process_update(XIXI_UPDATE_SUB_OP_REPLACE);
			} else if (memcmp(cmd, "prepend", cmd_length) == 0) {
				process_update(XIXI_UPDATE_SUB_OP_PREPEND);
			} else if (memcmp(cmd, "getbase", cmd_length) == 0) {
				process_get_base();
			} else {
				LOG_WARNING2("process_command error unkown request=" << http_request_.uri);
				write_error(XIXI_REASON_INVALID_PARAMETER);
				return;
			}
		} else if (cmd_length == 10) {
			if (memcmp(cmd, "checkwatch", cmd_length) == 0) {
				this->process_check_watch();
			} else {
				LOG_WARNING2("process_command error unkown request=" << http_request_.uri);
				write_error(XIXI_REASON_INVALID_PARAMETER);
				return;
			}
		} else if (cmd_length == 11) {
			if (memcmp(cmd, "createwatch", cmd_length) == 0) {
				this->process_create_watch();
			} else {
				LOG_WARNING2("process_command error unkown request=" << http_request_.uri);
				write_error(XIXI_REASON_INVALID_PARAMETER);
				return;
			}
		} else {
			LOG_WARNING2("process_command error unkown cmd_length=" << cmd_length);
			write_error(XIXI_REASON_INVALID_PARAMETER);
		}
	} else {
		key_ = (uint8_t*)http_request_.uri;
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
		if (t.length >= 2 && t.value[1] == '=') {
			if (t.value[0] =='g') {
				if (!safe_toui32(t.value + 2, t.length - 2, group_id_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (t.value[0] =='w') {
				if (!safe_toui32(t.value + 2, t.length - 2, watch_id_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (t.value[0] =='k') {
				key_ = (uint8_t*)decode_uri(t.value + 2, t.length - 2, key_length_);
				if (key_ == NULL) {
					write_error(XIXI_REASON_OUT_OF_MEMORY);
					return false;
				}
			} else if (t.value[0] =='v') {
				value_ = (uint8_t*)decode_uri(t.value + 2, t.length - 2, value_length_);
				if (value_ == NULL) {
					write_error(XIXI_REASON_OUT_OF_MEMORY);
					return false;
				}
			} else if (t.value[0] =='f') {
				if (!safe_toui32(t.value + 2, t.length - 2, flags_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (t.value[0] =='c') {
				if (!safe_toui64(t.value + 2, t.length - 2, cache_id_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (t.value[0] =='d') {
				if (!safe_toi64(t.value + 2, t.length - 2, delta_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (t.value[0] =='e') {
				touch_flag_ = true;
				if (!safe_toui32(t.value + 2, t.length - 2, expiration_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (t.value[0] =='a') {
				touch_flag_ = true;
				if (!safe_toui64(t.value + 2, t.length - 2, ack_cache_id_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (t.value[0] =='i') {
				touch_flag_ = true;
				if (!safe_toui32(t.value + 2, t.length - 2, interval_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (t.value[0] =='t') {
				touch_flag_ = true;
				if (!safe_toui32(t.value + 2, t.length - 2, timeout_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (t.value[0] =='s') {
				touch_flag_ = true;
				if (!safe_toui32(t.value + 2, t.length - 2, sub_op_)) {
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
	} else if (http_request_.content_type_length == 19
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
				if (name_length == 1) {
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
						key_ = (uint8_t*)value;
						key_length_ = buf - value - 4;
					} else if (name[0] == 'v') {
						value_ = (uint8_t*)value;
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
					} else if (name[0] =='a') {
						if (!safe_toui64(value, buf - value - 4, ack_cache_id_)) {
							write_error(XIXI_REASON_INVALID_PARAMETER);
							return;
						}
					} else if (name[0] =='i') {
						if (!safe_toui32(value, buf - value - 4, interval_)) {
							write_error(XIXI_REASON_INVALID_PARAMETER);
							return;
						}
					} else if (name[0] =='t') {
						if (!safe_toui32(value, buf - value - 4, timeout_)) {
							write_error(XIXI_REASON_INVALID_PARAMETER);
							return;
						}
					} else if (name[0] =='s') {
						if (!safe_toui32(value, buf - value - 4, sub_op_)) {
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
		LOG_WARNING2("process_cahce_arg unsupport content_type " << http_request_.content_type_length);
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
		uint32_t data_size = _snprintf((char*)buf, 20, "%"PRIu32"\r\n\r\n", it->data_size);
		add_write_buf((uint8_t*)DEFAULT_RES_200, sizeof(DEFAULT_RES_200) - 1);
		add_write_buf(buf, data_size);
		add_write_buf(it->get_data(), it->data_size);
		set_state(PEER_STATUS_WRITE);
		next_state_ = PEER_STATE_NEW_CMD;
	} else {
		if (watch_error) {
			write_error(XIXI_REASON_WATCH_NOT_FOUND);
		} else {
			write_error(XIXI_REASON_NOT_FOUND);
		}
	}
}

void Peer_Http::process_update(uint8_t sub_op) {
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
		uint32_t data_size = _snprintf((char*)buf, 24, "%"PRIu64, cache_id);
		uint8_t* buf2 = request_buf_.prepare(50);
		uint32_t data_size2 = _snprintf((char*)buf2, 50, "%"PRIu32"\r\n\r\n%s", data_size, buf);

		add_write_buf((uint8_t*)DEFAULT_RES_200, sizeof(DEFAULT_RES_200) - 1);
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
		add_write_buf((uint8_t*)DEFAULT_RES_200, sizeof(DEFAULT_RES_200) - 1);

		set_state(PEER_STATUS_WRITE);
		next_state_ = PEER_STATE_NEW_CMD;
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
		uint8_t* buf = request_buf_.prepare(50);
		uint32_t data_size = _snprintf((char*)buf, 50, "%"PRId64" %"PRIu64, value, cache_id_);

		uint8_t* buf2 = request_buf_.prepare(50);
		uint32_t data_size2 = _snprintf((char*)buf2, 50, "%"PRIu32"\r\n\r\n", data_size);

		add_write_buf((uint8_t*)DEFAULT_RES_200, sizeof(DEFAULT_RES_200) - 1);
		add_write_buf(buf2, data_size2);
		add_write_buf(buf, data_size);

		set_state(PEER_STATUS_WRITE);
		next_state_ = PEER_STATE_NEW_CMD;
	} else {
		write_error(reason);
	}
}

void Peer_Http::process_get_base() {
	LOG_TRACE2("process_get_base");

	uint64_t cache_id;
	uint32_t flags;
	uint32_t expiration;
	bool ret = cache_mgr_.get_base(group_id_, (uint8_t*)key_, key_length_, cache_id, flags, expiration);
	if (ret) {
		uint8_t* buf = request_buf_.prepare(50);
		uint32_t data_size = _snprintf((char*)buf, 50, "%"PRIu64" %"PRIu32" %"PRIu32, cache_id, flags, expiration);

		uint8_t* buf2 = request_buf_.prepare(50);
		uint32_t data_size2 = _snprintf((char*)buf2, 50, "%"PRIu32"\r\n\r\n", data_size);

		add_write_buf((uint8_t*)DEFAULT_RES_200, sizeof(DEFAULT_RES_200) - 1);
		add_write_buf(buf2, data_size2);
		add_write_buf(buf, data_size);

		set_state(PEER_STATUS_WRITE);
		next_state_ = PEER_STATE_NEW_CMD;
	} else {
		write_error(XIXI_REASON_NOT_FOUND);
	}
}

void Peer_Http::process_update_flags() {
	LOG_TRACE2("process_update_flags");

	XIXI_Update_Flags_Req_Pdu pdu;
	pdu.cache_id = cache_id_;
	pdu.flags = flags_;

	uint64_t cache_id = 0;
	bool ret = cache_mgr_.update_flags(group_id_, key_, key_length_, &pdu, cache_id);
	if (ret) {
		uint8_t* buf = request_buf_.prepare(50);
		uint32_t data_size = _snprintf((char*)buf, 50, "%"PRIu64, cache_id);

		uint8_t* buf2 = request_buf_.prepare(50);
		uint32_t data_size2 = _snprintf((char*)buf2, 50, "%"PRIu32"\r\n\r\n", data_size);

		add_write_buf((uint8_t*)DEFAULT_RES_200, sizeof(DEFAULT_RES_200) - 1);
		add_write_buf(buf2, data_size2);
		add_write_buf(buf, data_size);

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

void Peer_Http::process_touch() {
	LOG_TRACE2("process_touch");

	XIXI_Update_Expiration_Req_Pdu pdu;
	pdu.cache_id = cache_id_;
	pdu.expiration = expiration_;
	uint64_t cache_id = 0;
	bool ret = cache_mgr_.update_expiration(group_id_, (uint8_t*)key_, key_length_, &pdu, cache_id);
	if (ret) {
		uint8_t* buf = request_buf_.prepare(50);
		uint32_t data_size = _snprintf((char*)buf, 50, "%"PRIu64, cache_id);

		uint8_t* buf2 = request_buf_.prepare(50);
		uint32_t data_size2 = _snprintf((char*)buf2, 50, "%"PRIu32"\r\n\r\n", data_size);

		add_write_buf((uint8_t*)DEFAULT_RES_200, sizeof(DEFAULT_RES_200) - 1);
		add_write_buf(buf2, data_size2);
		add_write_buf(buf, data_size);

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

uint32_t Peer_Http::process_auth_req_pdu_extras(XIXI_Auth_Req_Pdu* pdu, uint8_t* data, uint32_t data_length) {
	LOG_TRACE2("process_auth_req_pdu_extras");
	uint8_t* base64 = data;
	uint32_t base64_length = pdu->base64_length;

	if (data_length < pdu->base64_length) {
		return 0;
	}

	std::string out;
	auth_.login(base64, pdu->base64_length, out);

	return base64_length;
}

void Peer_Http::process_create_watch() {
	LOG_INFO2("process_create_watch");
	uint32_t watch_id = cache_mgr_.create_watch(group_id_, interval_);

	uint8_t* buf = request_buf_.prepare(50);
	uint32_t data_size = _snprintf((char*)buf, 50, "%"PRIu32, watch_id);

	uint8_t* buf2 = request_buf_.prepare(50);
	uint32_t data_size2 = _snprintf((char*)buf2, 50, "%"PRIu32"\r\n\r\n", data_size);

	add_write_buf((uint8_t*)DEFAULT_RES_200, sizeof(DEFAULT_RES_200) - 1);
	add_write_buf(buf2, data_size2);
	add_write_buf(buf, data_size);

	set_state(PEER_STATUS_WRITE);
	next_state_ = PEER_STATE_NEW_CMD;
}

void Peer_Http::encode_update_list(std::list<uint64_t>& updated_list) {
	uint8_t* buf = request_buf_.prepare(2000);
	uint32_t offset = 0;
	uint32_t total_size = 0;
	bool is_begin = true;
	if (buf != NULL) {
		add_write_buf(NULL, 0); // will update next
		add_write_buf(NULL, 0); // will update next

		while (!updated_list.empty()) {
			uint64_t cache_id = updated_list.front();
			updated_list.pop_front();
			uint32_t data_size = 0;
			if (is_begin) {
				is_begin = false;
				data_size = _snprintf((char*)buf + offset, 30, "%"PRIu64, cache_id);
			} else {
				data_size = _snprintf((char*)buf + offset, 30, ",%"PRIu64, cache_id);
			}
			offset += data_size;
			total_size += data_size;
			if (offset + 30 < 2000) {
				add_write_buf(buf, offset);
				buf = request_buf_.prepare(2000);
				offset = 0;
				if (buf == NULL) {
					break;
				}
			}
		}
		if (buf == NULL) {
			write_error(XIXI_REASON_OUT_OF_MEMORY);
		} else {
			if (offset > 0) {
				add_write_buf(buf, offset);
			}

			uint8_t* buf2 = request_buf_.prepare(50);
			uint32_t data_size2 = _snprintf((char*)buf2, 50, "%"PRIu32"\r\n\r\n", total_size);

			update_write_buf(0, (uint8_t*)DEFAULT_RES_200, sizeof(DEFAULT_RES_200) - 1);
			update_write_buf(1, buf2, data_size2);

			set_state(PEER_STATUS_WRITE);
			next_state_ = PEER_STATE_NEW_CMD;
		}
	} else {
		write_error(XIXI_REASON_OUT_OF_MEMORY);
	}
}

void Peer_Http::process_check_watch() {
	std::list<uint64_t> updated_list;
	uint32_t updated_count = 0;
	boost::shared_ptr<Cache_Watch_Sink> sp = self_;
	bool ret = cache_mgr_.check_watch_and_set_callback(group_id_, watch_id_, updated_list, updated_count, ack_cache_id_, sp, interval_);
	//  LOG_INFO2("process_check_watch_req_pdu_fixed watch_id=" << pdu->watch_id << " ack=" << pdu->ack_cache_id << " updated_count=" << updated_count);

	if (!ret) {
		write_error(XIXI_REASON_WATCH_NOT_FOUND);
	} else {
		if (updated_count > 0) {
			encode_update_list(updated_list);
		} else {
			//    LOG_INFO2("process_check_watch_req_pdu_fixed wait a moment watch_id=" << pdu->watch_id << " updated_count=" << updated_count);
			timer_ = new boost::asio::deadline_timer(socket_->get_io_service());
			timer_->expires_from_now(boost::posix_time::seconds(timeout_));
			timer_->async_wait(boost::bind(&Peer_Http::handle_timer, this,
				boost::asio::placeholders::error, watch_id_));
			set_state(PEER_STATUS_ASYNC_WAIT);
		}
	}
}

void Peer_Http::process_flush() {
	uint32_t flush_count = 0;
	uint64_t flush_size = 0;
	cache_mgr_.flush(group_id_, flush_count, flush_size);

	uint8_t* buf = request_buf_.prepare(50);
	uint32_t data_size = _snprintf((char*)buf, 50, "%"PRIu32" %"PRIu64, flush_count, flush_size);

	uint8_t* buf2 = request_buf_.prepare(50);
	uint32_t data_size2 = _snprintf((char*)buf2, 50, "%"PRIu32"\r\n\r\n", data_size);

	add_write_buf((uint8_t*)DEFAULT_RES_200, sizeof(DEFAULT_RES_200) - 1);
	add_write_buf(buf2, data_size2);
	add_write_buf(buf, data_size);

	set_state(PEER_STATUS_WRITE);
	next_state_ = PEER_STATE_NEW_CMD;
}

void Peer_Http::process_stats() {
	std::string result;
	XIXI_Stats_Req_Pdu pdu;
	pdu.group_id = group_id_;
	pdu.op_flag = sub_op_;
	cache_mgr_.stats(&pdu, result);
	uint32_t size = result.size();

	uint8_t* buf2 = request_buf_.prepare(50);
	uint32_t data_size2 = _snprintf((char*)buf2, 50, "%"PRIu32"\r\n\r\n", size);

	uint8_t* buf = request_buf_.prepare(size);
	memcpy(buf, result.c_str(), size);

	add_write_buf((uint8_t*)DEFAULT_RES_200, sizeof(DEFAULT_RES_200) - 1);
	add_write_buf(buf2, data_size2);
	add_write_buf(buf, size);

	set_state(PEER_STATUS_WRITE);
	next_state_ = PEER_STATE_NEW_CMD;
/*
	
	int buf_size = XIXI_Stats_Res_Pdu::calc_encode_size(size);
	
	if (buf != NULL) {
		XIXI_Stats_Res_Pdu::encode(buf, (uint8_t*)result.c_str(), size);

		add_write_buf(buf, buf_size);

		set_state(PEER_STATUS_WRITE);
		next_state_ = PEER_STATE_NEW_CMD;
	} else {
		write_error(XIXI_REASON_OUT_OF_MEMORY);
	}*/
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
	ack_cache_id_ = 0;
	interval_ = 30;
	timeout_ = 10;
	sub_op_ = 0;
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
		encode_update_list(updated_list);
	}
	try_write();

	delete timer_;
	timer_ = NULL;
	lock_.unlock();
}

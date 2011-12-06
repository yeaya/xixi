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

#include <boost/filesystem.hpp>

#include "peer_http.h"
#include "settings.h"
#include "currtime.h"
#include "stats.h"
#include "log.h"
#include "auth.h"
#include "server.h"

#define DEFAULT_RES_200_KEEP_ALIVE "HTTP/1.1 200 OK\r\nServer: "HTTP_SERVER"\r\nConnection: Keep-Alive\r\nContent-Type: text/html\r\nContent-Length: "
#define DEFAULT_RES_200_CLOSE "HTTP/1.1 200 OK\r\nServer: "HTTP_SERVER"\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: "

#define GET_RES_200_KEEP_ALIVE "HTTP/1.1 200 OK\r\nServer: "HTTP_SERVER"\r\nConnection: Keep-Alive\r\nContent-Type: "
#define GET_RES_200_CLOSE "HTTP/1.1 200 OK\r\nServer: "HTTP_SERVER"\r\nConnection: close\r\nContent-Type: "

#define GET_RES_304_KEEP_ALIVE "HTTP/1.1 304 Not Modified\r\nServer: "HTTP_SERVER"\r\nConnection: Keep-Alive\r\nContent-Type: "
#define GET_RES_304_CLOSE "HTTP/1.1 304 Not Modified\r\nServer: "HTTP_SERVER"\r\nConnection: close\r\nContent-Type: "

#define GET_RES_301_KEEP_ALIVE "HTTP/1.1 301 Moved Permanently\r\nServer: "HTTP_SERVER"\r\nConnection: Keep-Alive\r\nContent-Type: text/html\r\nContent-Length: "
#define GET_RES_301_CLOSE "HTTP/1.1 301 Moved Permanently\r\nServer: "HTTP_SERVER"\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: "

#define DELETE_RES_200_KEEP_ALIVE "HTTP/1.1 200 OK\r\nServer: "HTTP_SERVER"\r\nConnection: Keep-Alive\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n"
#define DELETE_RES_200_CLOSE "HTTP/1.1 200 OK\r\nServer: "HTTP_SERVER"\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n"

#define ERROR_RES_400_KEEP_ALIVE "HTTP/1.1 400 Bad Request\r\nServer: "HTTP_SERVER"\r\nConnection: Keep-Alive\r\nContent-Type: text/html\r\nContent-Length: 24\r\n\r\n<H1>400 Bad Request</H1>"
#define ERROR_RES_400_CLOSE "HTTP/1.1 400 Bad Request\r\nServer: "HTTP_SERVER"\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: 24\r\n\r\n<H1>400 Bad Request</H1>"

#define ERROR_RES_401_KEEP_ALIVE "HTTP/1.1 401 Unauthorized\r\nServer: "HTTP_SERVER"\r\nConnection: Keep-Alive\r\nContent-Type: text/html\r\nContent-Length: 25\r\n\r\n<H1>401 Unauthorized</H1>"
#define ERROR_RES_401_CLOSE "HTTP/1.1 401 Unauthorized\r\nServer: "HTTP_SERVER"\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: 25\r\n\r\n<H1>401 Unauthorized</H1>"

#define ERROR_RES_404_KEEP_ALIVE "HTTP/1.1 404 Not Found\r\nServer: "HTTP_SERVER"\r\nConnection: Keep-Alive\r\nContent-Type: text/html\r\nContent-Length: 22\r\n\r\n<H1>404 Not Found</H1>"
#define ERROR_RES_404_CLOSE "HTTP/1.1 404 Not Found\r\nServer: "HTTP_SERVER"\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: 22\r\n\r\n<H1>404 Not Found</H1>"

#define ERROR_RES_413_KEEP_ALIVE "HTTP/1.1 413 Request Entity Too Large\r\nServer: "HTTP_SERVER"\r\nConnection: Keep-Alive\r\nContent-Type: text/html\r\nContent-Length: 37\r\n\r\n<H1>413 Request Entity Too Large</H1>"
#define ERROR_RES_413_CLOSE "HTTP/1.1 413 Request Entity Too Large\r\nServer: "HTTP_SERVER"\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: 37\r\n\r\n<H1>413 Request Entity Too Large</H1>"

#define ERROR_RES_500_KEEP_ALIVE "HTTP/1.1 500 Internal Server Error\r\nServer: "HTTP_SERVER"\r\nConnection: Keep-Alive\r\nContent-Type: text/html\r\nContent-Length: 34\r\n\r\n<H1>500 Internal Server Error</H1>"
#define ERROR_RES_500_CLOSE "HTTP/1.1 500 Internal Server Error\r\nServer: "HTTP_SERVER"\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: 34\r\n\r\n<H1>500 Internal Server Error</H1>"

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
/*
void tokenize_command(char* command, vector<token_t>& tokens) {
	char* start = command;
	char* end = command;
	token_t t;

	while (true) {
		if (*end == '&') {
			if (start != end) {
				t.value = start;
				t.length = (uint32_t)(end - start);
				*end = '\0';
				tokens.push_back(t);
			}
			start = end + 1;
		} else if (*end == '\0') {
			if (start != end) {
				t.value = start;
				t.length = (uint32_t)(end - start);
				*end = '\0';
				tokens.push_back(t);
			}
			break;
		}
		++end;
	}
}
*/
char* get_arg(char*& arg, uint32_t& arg_size) {
	char* begin = arg;
	char* p = arg;

	while (*p != '\0') {
		if (*p == '&') {
			*p = '\0';
			arg_size = (uint32_t)(p - begin);
			arg = p + 1;
			return begin;
		}
		++p;
	}
	if (p > begin) {
		arg_size = (uint32_t)(p - begin);
		arg = p;
		return begin;
	} else {
		arg_size = 0;
		return NULL;
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
		if (c == '+') {
			c = ' ';
		} else if (c == '%' && i + 2 < length) {
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
		*p = c;
		p++;
		out++;
	}
	*p = '\0';
	return buf;
}

Peer_Http::Peer_Http(boost::asio::ip::tcp::socket* socket) : self_(this), timer_(socket->get_io_service()) {
	LOG_DEBUG2("Peer_Http::Peer_Http()");
	op_count_ = 0;
	socket_ = socket;
	state_ = PEER_STATE_NEW_CMD;
	next_state_ = PEER_STATE_NEW_CMD;
	cache_item_ = NULL;
	write_buf_total_ = 0;
	read_item_buf_ = NULL;
	next_data_len_ = XIXI_PDU_HEAD_LENGTH;
	timer_flag_ = false;

	group_id_ = 0;
	watch_id_ = 0;
	cache_id_ = 0;
	key_ = NULL;
	key_length_ = 0;
	value_ = NULL;
	value_length_ = 0;
	value_content_type_ = "";
	value_content_type_length_ = 0;
	post_data_ = NULL;
	content_length_ = 0;
	flags_ = 0;
	expiration_ = 0;
	touch_flag_ = false;
	delta_ = 1;
	ack_cache_id_ = 0;
	interval_ = 120;
	timeout_ = 30;
	sub_op_ = 0;

	stats_.new_conn();
}

Peer_Http::~Peer_Http() {
	LOG_DEBUG2("~Peer_Http::Peer_Http()");
	cleanup();
	stats_.close_conn();
}

void Peer_Http::reset_for_new_cmd() {
	if (cache_item_ != NULL) {
		cache_mgr_.release_reference(cache_item_);
		cache_item_ = NULL;
	}

	group_id_ = 0;
	watch_id_ = 0;
	cache_id_ = 0;
	key_ = NULL;
	key_length_ = 0;
	value_ = NULL;
	value_length_ = 0;
	value_content_type_ = "";
	value_content_type_length_ = 0;
	post_data_ = NULL;
	content_length_ = 0;
	flags_ = 0;
	expiration_ = 0;
	touch_flag_ = false;
	delta_ = 1;
	ack_cache_id_ = 0;
	interval_ = 120;
	timeout_ = 30;
	sub_op_ = 0;
	http_request_.reset();

	request_buf_.reset();
	write_buf_total_ = 0;
	read_item_buf_ = NULL;
	next_data_len_ = XIXI_PDU_HEAD_LENGTH;
	set_state(PEER_STATE_READ_HEADER);
}

void Peer_Http::cleanup() {
	if (cache_item_ != NULL) {
		cache_mgr_.release_reference(cache_item_);
		cache_item_ = NULL;
	}

	if (socket_ != NULL) {
		delete socket_;
		socket_ = NULL;
	}
}

void Peer_Http::write_error(xixi_reason error_code) {
	const char* res = NULL;
	uint32_t res_size = 0;
	switch (error_code) {
		case XIXI_REASON_NOT_FOUND:
			if (http_request_.keepalive) {
				res = ERROR_RES_404_KEEP_ALIVE;
				res_size = sizeof(ERROR_RES_404_KEEP_ALIVE) - 1;
			} else {
				res = ERROR_RES_404_CLOSE;
				res_size = sizeof(ERROR_RES_404_CLOSE) - 1;
			}
			break;

		case XIXI_REASON_TOO_LARGE:
			if (http_request_.keepalive) {
				res = ERROR_RES_413_KEEP_ALIVE;
				res_size = sizeof(ERROR_RES_413_KEEP_ALIVE) - 1;
			} else {
				res = ERROR_RES_413_KEEP_ALIVE;
				res_size = sizeof(ERROR_RES_413_CLOSE) - 1;
			}
			break;
		case XIXI_REASON_AUTH_ERROR:
			if (http_request_.keepalive) {
				res = ERROR_RES_401_KEEP_ALIVE;
				res_size = sizeof(ERROR_RES_401_KEEP_ALIVE) - 1;
			} else {
				res = ERROR_RES_401_CLOSE;
				res_size = sizeof(ERROR_RES_401_CLOSE) - 1;
			}
			break;
		case XIXI_REASON_OUT_OF_MEMORY:
			if (http_request_.keepalive) {
				res = ERROR_RES_500_KEEP_ALIVE;
				res_size = sizeof(ERROR_RES_500_KEEP_ALIVE) - 1;
			} else {
				res = ERROR_RES_500_CLOSE;
				res_size = sizeof(ERROR_RES_500_CLOSE) - 1;
			}
			break;
		case XIXI_REASON_EXISTS:
		case XIXI_REASON_INVALID_PARAMETER:
		case XIXI_REASON_INVALID_OPERATION:
		case XIXI_REASON_MISMATCH:
		case XIXI_REASON_UNKNOWN_COMMAND:
		case XIXI_REASON_WATCH_NOT_FOUND:
		default:
			if (http_request_.keepalive) {
				res = ERROR_RES_400_KEEP_ALIVE;
				res_size = sizeof(ERROR_RES_400_KEEP_ALIVE) - 1;
			} else {
				res = ERROR_RES_400_CLOSE;
				res_size = sizeof(ERROR_RES_400_CLOSE) - 1;
			}
			break;
	}

	add_write_buf((uint8_t*)res, res_size);
	set_state(PEER_STATUS_WRITE);
	next_state_ = PEER_STATE_NEW_CMD;
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
			if (!http_request_.keepalive) {
				next_state_ = PEER_STATE_CLOSING;
			}
			set_state(next_state_);
			next_state_ = PEER_STATE_NEW_CMD;
			if (state_ == PEER_STATE_NEW_CMD && process_reqest_count < 1) {
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

		return (uint32_t)(p - data + 4);
	}

	if (data_len >= 8192) {
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
		request_line_length = (uint32_t)(p - request_line);
	}
	if (request_line_length >= 6) {
		if (memcmp(request_line + request_line_length - 3, "1.1", 3) == 0) {
			http_request_.http_11 = true;
			http_request_.keepalive = true;
		}

		uint32_t offset = 0;
		uint32_t method;
		if (IS_METHOD(request_header, 'G', 'E', 'T', ' ')) {
			offset = 4;
			method = GET_METHOD;
		} else if (IS_METHOD(request_header, 'P', 'O', 'S', 'T')) {
			offset = 5;
			method = POST_METHOD;
		} else if (IS_METHOD(request_header, 'H', 'E', 'A', 'D')) {
			offset = 5;
			method = HEAD_METHOD;
		} else {
//			LOG_WARNING2("process_request_header error method=" << method);
			write_error(XIXI_REASON_INVALID_PARAMETER);
			return;
		}
		http_request_.method = method;

		http_request_.uri = request_line + offset;
		p = (char*)memchr(http_request_.uri, ' ', request_line_length - offset);
		if (p != NULL) {
			*p = '\0';
			http_request_.uri_length = (uint32_t)(p - http_request_.uri);
		} else {
			http_request_.uri_length = (uint32_t)(request_line_length - offset);
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
		name_length = (uint32_t)(p - name);
		p++;
		while (*p == ' ') {
			p++;
		}
		if (p == NULL) {
			break;
		}
		char* value = p;
		uint32_t value_length = length - (value - request_header_field);
		char* p2 = (char*)memchr(value, '\n', value_length);
		if (p2 != NULL) {
			if (*(p2 - 1) == '\r') {
				*(p2 - 1) = '\0';
				value_length = (uint32_t)((p2 - value)) - 1;
			} else {
				*(p2) = '\0';
				value_length = (uint32_t)(p2 - value);
			}
	//		LOG_INFO2(name << "=" << value);
			if (!handle_request_header_field(name, name_length, value, value_length)) {
				return false;
			}
			name = p2 + 1;
			name_length = 0;
			p = (char*)memchr(name, ':', length - (name - request_header_field));
		} else {
			if (!handle_request_header_field(name, name_length, value, value_length)) {
				return false;
			}
			break;
		}
	}
	return true;
}

bool Peer_Http::handle_request_header_field(char* name, uint32_t name_length, char* value, uint32_t value_length) {
	if (name_length == 10) {
		if (strcasecmp(name, "Connection", name_length) == 0) {
			if (value_length >= 10 && strcasecmp(value, "Keep-Alive", 10) == 0) {
				http_request_.keepalive = true;
			}
		}
	} else if (name_length == 12) {
		if (strcasecmp(name, "content-type", name_length) == 0 && value_length > 0) {
			char* buf = (char*)request_buf_.prepare(value_length + 1);
			if (buf == NULL) {
				return false;
			}
			memcpy(buf, value, value_length);
			buf[value_length] = '\0';
			http_request_.content_type = buf;
			http_request_.content_type_length = value_length;

			if (value_length > 33 && strcasecmp(buf, "application/x-www-form-urlencoded", 33) == 0) {
				buf[33] = '\0';
				http_request_.content_type_length = 33;
			} else if (value_length > 30 && strcasecmp(buf, "multipart/form-data", 19) == 0) {
				char* p = strstr(buf + 19, "boundary=");
				if (p != NULL) {
					http_request_.boundary = p + 9;
					http_request_.boundary_length = value_length - (http_request_.boundary - buf);
				}
				buf[19] = '\0';
				http_request_.content_type_length = 19;
			}
		}
	} else if (name_length == 13) {
		if (strcasecmp(name, "if-none-match", name_length) == 0) {
			char* buf = (char*)request_buf_.prepare(value_length + 1);
			if (buf == NULL) {
				return false;
			}
			memcpy(buf, value, value_length);
			buf[value_length] = '\0';
			http_request_.entity_tag = buf;
			http_request_.entity_tag_length = value_length;
		}
	} else if (name_length == 14) {
		if (strcasecmp(name, "content-length", name_length) == 0) {
			if (!safe_toui32(value, value_length, content_length_)) {
				return false;
			}
		}
	} else if (name_length == 15) {
		if (strcasecmp(name, "Accept-Encoding", name_length) == 0) {
			char* buf = memfind(value, value_length, "gzip", 4);
			http_request_.accept_gzip = (buf != NULL);
		}
	}
	return true;
}

void Peer_Http::process_command() {
	if (http_request_.method != HEAD_METHOD && http_request_.uri_length >= 10 && memcmp(http_request_.uri, "/xixibase/", 10) == 0) {
		uint32_t cmd_length = 0;
		char* arg = strrchr(http_request_.uri, '?');
		if (arg != NULL) {
			cmd_length = (uint32_t)(arg - http_request_.uri) - 10;
			if (!process_request_arg(arg + 1)) {
				LOG_WARNING2("process_command failed arg=" << (arg + 1));
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
			if (memcmp(cmd, "watch", cmd_length) == 0) {
				this->process_watch();
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
}

bool Peer_Http::process_request_arg(char* args) {
//	tokens_.clear();
//	tokenize_command(arg, tokens_);

	char* next_arg = args;
	uint32_t arg_size = 0;

	while (true) {
		
		char* arg = get_arg(next_arg, arg_size);
		if (arg == NULL) {
			break;
		}
	//	next_arg = arg + arg_size + 1;
//	for (size_t i = 0; i < tokens_.size(); i++) {
//		token_t& t = tokens_[i];
		if (arg_size >= 2 && arg[1] == '=') {
			if (arg[0] =='g') {
				if (!safe_toui32(arg + 2, arg_size - 2, group_id_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (arg[0] =='w') {
				if (!safe_toui32(arg + 2, arg_size - 2, watch_id_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (arg[0] =='k') {
				key_ = (uint8_t*)decode_uri(arg + 2, arg_size - 2, key_length_);
				if (key_ == NULL) {
					write_error(XIXI_REASON_OUT_OF_MEMORY);
					return false;
				}
			} else if (arg[0] =='v') {
				value_ = (uint8_t*)decode_uri(arg + 2, arg_size - 2, value_length_);
				if (value_ == NULL) {
					write_error(XIXI_REASON_OUT_OF_MEMORY);
					return false;
				}
			} else if (arg[0] =='f') {
				if (!safe_toui32(arg + 2, arg_size - 2, flags_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (arg[0] =='c') {
				if (!safe_toui64(arg + 2, arg_size - 2, cache_id_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (arg[0] =='d') {
				if (!safe_toi64(arg + 2, arg_size - 2, delta_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (arg[0] =='e') {
				touch_flag_ = true;
				if (!safe_toui32(arg + 2, arg_size - 2, expiration_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (arg[0] =='a') {
				if (!safe_toui64(arg + 2, arg_size - 2, ack_cache_id_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (arg[0] =='i') {
				if (!safe_toui32(arg + 2, arg_size - 2, interval_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (arg[0] =='t') {
				if (!safe_toui32(arg + 2, arg_size - 2, timeout_)) {
					write_error(XIXI_REASON_INVALID_PARAMETER);
					return false;
				}
			} else if (arg[0] =='s') {
				if (!safe_toui32(arg + 2, arg_size - 2, sub_op_)) {
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
			&& strcasecmp(http_request_.content_type, "application/x-www-form-urlencoded", http_request_.content_type_length) == 0) {

		if (!process_request_arg((char*)post_data_)) {
			LOG_WARNING2("process_cahce_arg failed");
			return;
		}
		process_command();
	} else if (http_request_.content_type_length == 19
			&& strcasecmp(http_request_.content_type, "multipart/form-data", http_request_.content_type_length) == 0
			&& http_request_.boundary_length > 0) {
		char* buf = (char*)memfind((char*)post_data_, content_length_, (char*)http_request_.boundary, http_request_.boundary_length);
		while (buf != NULL) {
			buf += http_request_.boundary_length + 2;
			buf = strstr(buf, " name=\"");
			if (buf != NULL) {
				char* name = buf + 7;
				uint32_t name_length = 0;
				char* end = strstr(name, "\"");
				if (end != NULL) {
					name_length = (uint32_t)(end - name);
				}

				if (name_length == 1 && name[0] == 'v')  {
					char* content_type = strstr(name + 2, "\r\nContent-Type:");
					if (content_type != NULL) {
						content_type += 15;
						while (*content_type == ' ') {
							content_type++;
						}
						end = strstr(content_type, "\r\n");
						if (end != NULL) {
							uint32_t content_type_length_ = (uint32_t)(end - content_type);
							if (content_type_length_ < 128) {
								value_content_type_ = content_type;
								value_content_type_length_ = content_type_length_;
							}
						}
					}
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
						if (!safe_toui32(value, (uint32_t)(buf - value) - 4, group_id_)) {
							write_error(XIXI_REASON_INVALID_PARAMETER);
							return;
						}
					} else if (name[0] == 'w') {
						if (!safe_toui32(value, (uint32_t)(buf - value) - 4, watch_id_)) {
							write_error(XIXI_REASON_INVALID_PARAMETER);
							return;
						}
					} else if (name[0] == 'k') {
						key_ = (uint8_t*)value;
						key_length_ = (uint32_t)(buf - value) - 4;
					} else if (name[0] == 'v') {
						value_ = (uint8_t*)value;
						value_length_ = (uint32_t)(buf - value) - 4;
					} else if (name[0] == 'f') {
						if (!safe_toui32(value, (uint32_t)(buf - value) - 4, flags_)) {
							write_error(XIXI_REASON_INVALID_PARAMETER);
							return;
						}
					} else if (name[0] == 'c') {
						if (!safe_toui64(value, (uint32_t)(buf - value) - 4, cache_id_)) {
							write_error(XIXI_REASON_INVALID_PARAMETER);
							return;
						}
					} else if (name[0] == 'd') {
						if (!safe_toi64(value, (uint32_t)(buf - value) - 4, delta_)) {
							write_error(XIXI_REASON_INVALID_PARAMETER);
							return;
						}
					} else if (name[0] == 'e') {
						touch_flag_ = true;
						if (!safe_toui32(value, (uint32_t)(buf - value) - 4, expiration_)) {
							write_error(XIXI_REASON_INVALID_PARAMETER);
							return;
						}
					} else if (name[0] =='a') {
						if (!safe_toui64(value, (uint32_t)(buf - value) - 4, ack_cache_id_)) {
							write_error(XIXI_REASON_INVALID_PARAMETER);
							return;
						}
					} else if (name[0] =='i') {
						if (!safe_toui32(value, (uint32_t)(buf - value) - 4, interval_)) {
							write_error(XIXI_REASON_INVALID_PARAMETER);
							return;
						}
					} else if (name[0] =='t') {
						if (!safe_toui32(value, (uint32_t)(buf - value - 4), timeout_)) {
							write_error(XIXI_REASON_INVALID_PARAMETER);
							return;
						}
					} else if (name[0] =='s') {
						if (!safe_toui32(value, (uint32_t)(buf - value) - 4, sub_op_)) {
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
	xixi_reason reason;
	uint32_t expiration;
	Cache_Item* it = get_cache_item(false, reason, expiration);

	if (it != NULL) {
		cache_item_ = it;
		uint32_t mime_type_length;
		const char* mime_type = get_mime_type(it, mime_type_length);

		char etag[30];
		uint32_t etag_length = _snprintf((char*)etag, sizeof(etag), "\"%"PRIu64"\"", it->cache_id);
		if (etag_length == http_request_.entity_tag_length && memcmp(etag, http_request_.entity_tag, etag_length) == 0) {
			uint8_t* header = request_buf_.prepare(200);
			uint32_t header_size = _snprintf((char*)header, 200, "%s\r\n"
				"CacheID: %"PRIu64"\r\n"
				"Flags: %"PRIu32"\r\n"
				"Expiration: %"PRIu32"\r\n"
				"ETag: %s\r\n\r\n",
				mime_type, it->cache_id, it->flags, expiration, etag);

			if (http_request_.keepalive) {
				add_write_buf((uint8_t*)GET_RES_304_KEEP_ALIVE, sizeof(GET_RES_304_KEEP_ALIVE) - 1);
			} else {
				add_write_buf((uint8_t*)GET_RES_304_CLOSE, sizeof(GET_RES_304_CLOSE) - 1);
			}
			add_write_buf((uint8_t*)header, header_size);
		} else {
			uint32_t gzip_size = 0;
			vector<Const_Data> write_buf;
			if (http_request_.method != HEAD_METHOD && http_request_.accept_gzip
					&& it->data_size >= settings_.min_gzip_size
					&& it->data_size <= settings_.max_gzip_size
					&& settings_.is_gzip_mime_type((const uint8_t*)mime_type, mime_type_length)) {
				gzip_size = gzip_encode(it->get_data(), it->data_size, write_buf);
				if (gzip_size + 50 >= it->data_size) {
					gzip_size = 0;
				}
			}
			uint8_t* header = request_buf_.prepare(300);
			uint32_t header_size;
			if (gzip_size > 0) {
				header_size = _snprintf((char*)header, 300, "%s\r\nContent-Encoding: gzip\r\nContent-Length: %"PRIu32"\r\n"
					"CacheID: %"PRIu64"\r\n"
					"Flags: %"PRIu32"\r\n"
					"Expiration: %"PRIu32"\r\n"
					"ETag: %s\r\n\r\n",
					mime_type, gzip_size, it->cache_id, it->flags, expiration, etag);
			} else {
				header_size = _snprintf((char*)header, 300, "%s\r\nContent-Length: %"PRIu32"\r\n"
					"CacheID: %"PRIu64"\r\n"
					"Flags: %"PRIu32"\r\n"
					"Expiration: %"PRIu32"\r\n"
					"ETag: %s\r\n\r\n",
					mime_type, it->data_size, it->cache_id, it->flags, expiration, etag);
			}
			if (http_request_.keepalive) {
				add_write_buf((uint8_t*)GET_RES_200_KEEP_ALIVE, sizeof(GET_RES_200_KEEP_ALIVE) - 1);
			} else {
				add_write_buf((uint8_t*)GET_RES_200_CLOSE, sizeof(GET_RES_200_CLOSE) - 1);
			}
			add_write_buf(header, header_size);
			if (http_request_.method != HEAD_METHOD) {
				if (gzip_size > 0) {
					for (size_t i = 0; i < write_buf.size(); i++) {
						Const_Data& cd = write_buf[i];
						add_write_buf(cd.data, cd.size); 
					}
				} else {
					add_write_buf(it->get_data(), it->data_size);
				}
			}
		}
		set_state(PEER_STATUS_WRITE);
		next_state_ = PEER_STATE_NEW_CMD;
	} else {
		if (reason == XIXI_REASON_MOVED_PERMANENTLY) {
			uint32_t prepare_size = 150 + key_length_;
			uint8_t* body = request_buf_.prepare(prepare_size);
			uint32_t body_size = _snprintf((char*)body, prepare_size, "<HTML><HEAD><TITLE>301 Moved</TITLE></HEAD><BODY><H1>301 Moved</H1>The document has moved"
				"<A HREF=\"%s/\">here</A>.</BODY></HTML>",
				(char*)key_);
			prepare_size = 35 +key_length_;
			uint8_t* header = request_buf_.prepare(prepare_size);
			uint32_t header_size = _snprintf((char*)header, prepare_size, "%"PRIu32"\r\nLocation: %s/\r\n\r\n",
				body_size, (char*)key_);

			if (http_request_.keepalive) {
				add_write_buf((uint8_t*)GET_RES_301_KEEP_ALIVE, sizeof(GET_RES_301_KEEP_ALIVE) - 1);
			} else {
				add_write_buf((uint8_t*)GET_RES_301_CLOSE, sizeof(GET_RES_301_CLOSE) - 1);
			}
			add_write_buf((uint8_t*)header, header_size);
			add_write_buf((uint8_t*)body, body_size);
			set_state(PEER_STATUS_WRITE);
			next_state_ = PEER_STATE_NEW_CMD;
		} else {
			write_error(reason);
		}
	}
}

const char* Peer_Http::get_mime_type(Cache_Item* it, uint32_t& mime_type_length) {
	const char* content_type = "";
	uint32_t ext_size = it->get_ext_size();
	if (ext_size > 0 && ext_size <= MAX_MIME_TYPE_LENGTH) {
		char* tmp = (char*)this->request_buf_.prepare(ext_size + 1);
		memcpy(tmp, it->get_ext(), ext_size);
		tmp[ext_size] = '\0';
		content_type = tmp;
		mime_type_length = ext_size;
	} else {
		uint32_t suffix_size;
		const char* suffix = get_suffix((const char*)key_, key_length_, suffix_size);
		if (suffix != NULL) {
			const uint8_t* mime_type = settings_.get_mime_type((const uint8_t*)suffix, suffix_size, mime_type_length);
			if (mime_type != NULL && mime_type_length <= MAX_MIME_TYPE_LENGTH) {
				char* tmp = (char*)this->request_buf_.prepare(mime_type_length + 1);
				memcpy(tmp, mime_type, mime_type_length);
				tmp[mime_type_length] = '\0';
				content_type = tmp;
			} else {
				content_type = settings_.get_default_mime_type(mime_type_length);
			}
		} else {
			content_type = settings_.get_default_mime_type(mime_type_length);
		}
	}
	return content_type;
}

Cache_Item* Peer_Http::get_cache_item(bool is_base, xixi_reason& reason, uint32_t& expiration) {
	Cache_Item* it;
	if (touch_flag_) {
		expiration = expiration_;
		it = cache_mgr_.get_touch(group_id_, (uint8_t*)key_, key_length_, watch_id_, expiration, reason);
	} else {
		it = cache_mgr_.get(group_id_, (uint8_t*)key_, key_length_, watch_id_, is_base, expiration, reason);
	}

	// try load from file /webapps
	if (it == NULL && key_length_ > 0 && key_[0] == '/') {
		string filename = settings_.home_dir + "webapps" + (char*)key_;
		LOG_INFO("get_cache_item " << filename);
		try {
			boost::filesystem::path p(filename);
			if (exists(p)) {
				if (is_directory(p)) {
					cout << p << " is a directory" << endl;
					if (key_[key_length_ - 1] == '/') {
						// load welcome file
						it = get_welcome_file(is_base, reason, expiration);
					} else {
						// localion to the directary
						reason = XIXI_REASON_MOVED_PERMANENTLY;
					}
				} else {
					// load from file
					if (!touch_flag_) {
						expiration = settings_.default_cache_expiration;
					}
					it = cache_mgr_.load_from_file(group_id_, (uint8_t*)key_, key_length_, watch_id_, expiration, reason);
				}
			} else {
				cout << p << " no exists" << endl;
				reason = XIXI_REASON_NOT_FOUND;
				return NULL;
			}
		} catch (const boost::system::system_error& ex) {
			cout << ex.what() << endl;
			reason = XIXI_REASON_NOT_FOUND;
			return NULL;
		}
	}
	return it;
}

Cache_Item* Peer_Http::get_welcome_file(bool is_base, xixi_reason& reason, uint32_t& expiration) {
	Cache_Item* it = NULL;
	reason = XIXI_REASON_NOT_FOUND;
	for (size_t i = 0; i < settings_.welcome_file_list.size(); i++) {
		string& welcome = settings_.welcome_file_list[i];
		uint32_t new_key_length = key_length_ + welcome.size();
		uint8_t* new_key = request_buf_.prepare(new_key_length + 1);
		memcpy(new_key, key_, key_length_);
		memcpy(new_key + key_length_, welcome.c_str(), welcome.size());
		new_key[new_key_length] = '\0';

		if (touch_flag_) {
			expiration = expiration_;
			it = cache_mgr_.get_touch(group_id_, (uint8_t*)new_key, new_key_length, watch_id_, expiration, reason);
		} else {
			it = cache_mgr_.get(group_id_, (uint8_t*)new_key, new_key_length, watch_id_, is_base, expiration, reason);
		}
		if (it != NULL) {
			reason = XIXI_REASON_SUCCESS;
			break;
		} else {
			if (!touch_flag_) {
				expiration = settings_.default_cache_expiration;
			}
			it = cache_mgr_.load_from_file(group_id_, (uint8_t*)new_key, new_key_length, watch_id_, expiration, reason);
			if (reason == XIXI_REASON_NOT_FOUND) {
				continue;
			} else {
				break;
			}
		}
	}
	return it;
}

void Peer_Http::process_update(uint8_t sub_op) {
	cache_item_ = cache_mgr_.alloc_item(group_id_, key_length_, flags_,
		expiration_, value_length_, value_content_type_length_);

	if (cache_item_ == NULL) {
		if (cache_mgr_.item_size_ok(key_length_, value_length_, value_content_type_length_)) {
			write_error(XIXI_REASON_OUT_OF_MEMORY);
		} else {
			write_error(XIXI_REASON_TOO_LARGE);
		}
		return;
	}

	memcpy(cache_item_->get_key(), key_, key_length_);
	memcpy(cache_item_->get_data(), value_, value_length_);
	cache_item_->set_ext((uint8_t*)value_content_type_);

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
		uint8_t* body = request_buf_.prepare(50);
		uint32_t body_size = _snprintf((char*)body, 50, "{\"cacheid\":%"PRIu64"}", cache_id);
		uint8_t* header = request_buf_.prepare(30);
		uint32_t header_size = _snprintf((char*)header, 30, "%"PRIu32"\r\n\r\n", body_size);

		if (http_request_.keepalive) {
			add_write_buf((uint8_t*)DEFAULT_RES_200_KEEP_ALIVE, sizeof(DEFAULT_RES_200_KEEP_ALIVE) - 1);
		} else {
			add_write_buf((uint8_t*)DEFAULT_RES_200_CLOSE, sizeof(DEFAULT_RES_200_CLOSE) - 1);
		}
		add_write_buf(header, header_size);
		add_write_buf(body, body_size);
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
		if (http_request_.keepalive) {
			add_write_buf((uint8_t*)DELETE_RES_200_KEEP_ALIVE, sizeof(DELETE_RES_200_KEEP_ALIVE) - 1);
		} else {
			add_write_buf((uint8_t*)DELETE_RES_200_CLOSE, sizeof(DELETE_RES_200_CLOSE) - 1);
		}

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
	xixi_reason reason = cache_mgr_.delta(group_id_, (uint8_t*)key_, key_length_, incr, delta_, cache_id_, value);
	if (reason == XIXI_REASON_SUCCESS) {
		uint8_t* body = request_buf_.prepare(100);
		uint32_t body_size = _snprintf((char*)body, 100, "{\"value\":%"PRId64",\"cacheid\":%"PRIu64"}", value, cache_id_);

		uint8_t* header = request_buf_.prepare(50);
		uint32_t header_size = _snprintf((char*)header, 50, "%"PRIu32"\r\n\r\n", body_size);

		if (http_request_.keepalive) {
			add_write_buf((uint8_t*)DEFAULT_RES_200_KEEP_ALIVE, sizeof(DEFAULT_RES_200_KEEP_ALIVE) - 1);
		} else {
			add_write_buf((uint8_t*)DEFAULT_RES_200_CLOSE, sizeof(DEFAULT_RES_200_CLOSE) - 1);
		}
		add_write_buf(header, header_size);
		add_write_buf(body, body_size);

		set_state(PEER_STATUS_WRITE);
		next_state_ = PEER_STATE_NEW_CMD;
	} else {
		write_error(reason);
	}
}

void Peer_Http::process_get_base() {
	LOG_TRACE2("process_get_base");

	xixi_reason reason;
	uint32_t expiration;
	Cache_Item* it = get_cache_item(true, reason, expiration);

	if (it != NULL) {
		uint32_t mime_type_length;
		const char* mime_type = get_mime_type(it, mime_type_length);

		uint8_t* body = request_buf_.prepare(200);
		uint32_t body_size = _snprintf((char*)body, 200,
			"{\"cacheid\":%"PRIu64",\"flags\":%"PRIu32",\"expiration\":%"PRIu32",\"mime_type\":\"%s\",\"size\":%"PRIu32"}",
			it->cache_id, it->flags, expiration, mime_type, it->data_size);

		cache_mgr_.release_reference(it);
		it = NULL;

		uint8_t* header = request_buf_.prepare(30);
		uint32_t header_size = _snprintf((char*)header, 30, "%"PRIu32"\r\n\r\n", body_size);

		if (http_request_.keepalive) {
			add_write_buf((uint8_t*)DEFAULT_RES_200_KEEP_ALIVE, sizeof(DEFAULT_RES_200_KEEP_ALIVE) - 1);
		} else {
			add_write_buf((uint8_t*)DEFAULT_RES_200_CLOSE, sizeof(DEFAULT_RES_200_CLOSE) - 1);
		}
		add_write_buf(header, header_size);
		add_write_buf(body, body_size);

		set_state(PEER_STATUS_WRITE);
		next_state_ = PEER_STATE_NEW_CMD;
	} else {
		write_error(reason);
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
		uint8_t* body = request_buf_.prepare(50);
		uint32_t body_size = _snprintf((char*)body, 50, "{\"cacheid\":%"PRIu64"}", cache_id);

		uint8_t* header = request_buf_.prepare(50);
		uint32_t header_size = _snprintf((char*)header, 50, "%"PRIu32"\r\n\r\n", body_size);

		if (http_request_.keepalive) {
			add_write_buf((uint8_t*)DEFAULT_RES_200_KEEP_ALIVE, sizeof(DEFAULT_RES_200_KEEP_ALIVE) - 1);
		} else {
			add_write_buf((uint8_t*)DEFAULT_RES_200_CLOSE, sizeof(DEFAULT_RES_200_CLOSE) - 1);
		}
		add_write_buf(header, header_size);
		add_write_buf(body, body_size);

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
		uint8_t* body = request_buf_.prepare(50);
		uint32_t body_size = _snprintf((char*)body, 50, "{\"cacheid\":%"PRIu64"}", cache_id);

		uint8_t* header = request_buf_.prepare(50);
		uint32_t header_size = _snprintf((char*)header, 50, "%"PRIu32"\r\n\r\n", body_size);

		if (http_request_.keepalive) {
			add_write_buf((uint8_t*)DEFAULT_RES_200_KEEP_ALIVE, sizeof(DEFAULT_RES_200_KEEP_ALIVE) - 1);
		} else {
			add_write_buf((uint8_t*)DEFAULT_RES_200_CLOSE, sizeof(DEFAULT_RES_200_CLOSE) - 1);
		}
		add_write_buf(header, header_size);
		add_write_buf(body, body_size);

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

	uint8_t* body = request_buf_.prepare(50);
	uint32_t body_size = _snprintf((char*)body, 50, "{\"watchid\":%"PRIu32"}", watch_id);

	uint8_t* header = request_buf_.prepare(50);
	uint32_t header_size = _snprintf((char*)header, 50, "%"PRIu32"\r\n\r\n", body_size);

	if (http_request_.keepalive) {
		add_write_buf((uint8_t*)DEFAULT_RES_200_KEEP_ALIVE, sizeof(DEFAULT_RES_200_KEEP_ALIVE) - 1);
	} else {
		add_write_buf((uint8_t*)DEFAULT_RES_200_CLOSE, sizeof(DEFAULT_RES_200_CLOSE) - 1);
	}
	add_write_buf(header, header_size);
	add_write_buf(body, body_size);

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
		uint32_t write_buf_count = 2;

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
			if (offset + 30 > 2000) {
				add_write_buf(buf, offset);
				write_buf_count++;
				buf = request_buf_.prepare(2000);
				offset = 0;
				if (buf == NULL) {
					break;
				}
			}
		}
		if (buf == NULL) {
			pop_write_buf(write_buf_count);
			write_error(XIXI_REASON_OUT_OF_MEMORY);
		} else {
			if (offset > 0) {
				add_write_buf(buf, offset);
			}

			uint8_t* buf2 = request_buf_.prepare(50);
			uint32_t data_size2 = _snprintf((char*)buf2, 50, "%"PRIu32"\r\n\r\n", total_size);

			if (http_request_.keepalive) {
				update_write_buf(0, (uint8_t*)DEFAULT_RES_200_KEEP_ALIVE, sizeof(DEFAULT_RES_200_KEEP_ALIVE) - 1);
			} else {
				update_write_buf(0, (uint8_t*)DEFAULT_RES_200_CLOSE, sizeof(DEFAULT_RES_200_CLOSE) - 1);
			}
			update_write_buf(1, buf2, data_size2);

			set_state(PEER_STATUS_WRITE);
			next_state_ = PEER_STATE_NEW_CMD;
		}
	} else {
		write_error(XIXI_REASON_OUT_OF_MEMORY);
	}
}

void Peer_Http::process_watch() {
	std::list<uint64_t> updated_list;
	uint32_t updated_count = 0;
	boost::shared_ptr<Cache_Watch_Sink> sp = self_;
	timer_flag_ = false;
	bool ret = cache_mgr_.check_watch_and_set_callback(group_id_, watch_id_, updated_list, updated_count, ack_cache_id_, sp, interval_);
	//  LOG_INFO2("process_check_watch_req_pdu_fixed watch_id=" << pdu->watch_id << " ack=" << pdu->ack_cache_id << " updated_count=" << updated_count);

	if (!ret) {
		write_error(XIXI_REASON_WATCH_NOT_FOUND);
	} else {
		if (updated_count > 0) {
			encode_update_list(updated_list);
		} else {
			//    LOG_INFO2("process_check_watch_req_pdu_fixed wait a moment watch_id=" << pdu->watch_id << " updated_count=" << updated_count);
			timer_lock_.lock();
			timer_.expires_from_now(boost::posix_time::seconds(timeout_));
			timer_.async_wait(boost::bind(&Peer_Http::handle_timer, this,
				boost::asio::placeholders::error, watch_id_));
			if (timer_flag_) {
				boost::system::error_code ec;
				timer_.cancel(ec);
				LOG_INFO2("process_check_watch timer cancel");
			} else {
				timer_flag_ = true;
			}
			timer_lock_.unlock();
			set_state(PEER_STATUS_ASYNC_WAIT);
		}
	}
}

void Peer_Http::process_flush() {
	uint32_t flush_count = 0;
	uint64_t flush_size = 0;
	cache_mgr_.flush(group_id_, flush_count, flush_size);

	uint8_t* body = request_buf_.prepare(50);
	uint32_t body_size = _snprintf((char*)body, 50, "{\"flushcount\":%"PRIu32",\"flushsize\":%"PRIu64"}", flush_count, flush_size);

	uint8_t* header = request_buf_.prepare(50);
	uint32_t header_size = _snprintf((char*)header, 50, "%"PRIu32"\r\n\r\n", body_size);

	if (http_request_.keepalive) {
		add_write_buf((uint8_t*)DEFAULT_RES_200_KEEP_ALIVE, sizeof(DEFAULT_RES_200_KEEP_ALIVE) - 1);
	} else {
		add_write_buf((uint8_t*)DEFAULT_RES_200_CLOSE, sizeof(DEFAULT_RES_200_CLOSE) - 1);
	}
	add_write_buf(header, header_size);
	add_write_buf(body, body_size);

	set_state(PEER_STATUS_WRITE);
	next_state_ = PEER_STATE_NEW_CMD;
}

void Peer_Http::process_stats() {
	std::string result;
	XIXI_Stats_Req_Pdu pdu;
	pdu.group_id = group_id_;
	pdu.op_flag = sub_op_;
	cache_mgr_.stats(&pdu, result);
	uint32_t size = (uint32_t)result.size();

	uint8_t* buf2 = request_buf_.prepare(50);
	uint32_t data_size2 = _snprintf((char*)buf2, 50, "%"PRIu32"\r\n\r\n", size);

	uint8_t* buf = request_buf_.prepare(size);
	memcpy(buf, result.c_str(), size);

	if (http_request_.keepalive) {
		add_write_buf((uint8_t*)DEFAULT_RES_200_KEEP_ALIVE, sizeof(DEFAULT_RES_200_KEEP_ALIVE) - 1);
	} else {
		add_write_buf((uint8_t*)DEFAULT_RES_200_CLOSE, sizeof(DEFAULT_RES_200_CLOSE) - 1);
	}
	add_write_buf(buf2, data_size2);
	add_write_buf(buf, size);

	set_state(PEER_STATUS_WRITE);
	next_state_ = PEER_STATE_NEW_CMD;
}

void Peer_Http::on_cache_watch_notify(uint32_t watch_id) {
	timer_lock_.lock();
		if (timer_flag_) {
			boost::system::error_code ec;
			timer_.cancel(ec);
		} else {
			timer_flag_ = true;
		}
	timer_lock_.unlock();
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

void Peer_Http::handle_read(const boost::system::error_code& err, std::size_t length) {
	LOG_TRACE2("handle_read length=" << length << " err=" << err.message() << " err_value=" << err.value());
	lock_.lock();
	--op_count_;
	if (!err) {
		read_buffer_.read_data_size_ += (uint32_t)length;

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
	if (op_count_ == 0) {
		++op_count_;
		read_buffer_.handle_processed();
		socket_->async_read_some(boost::asio::buffer(read_buffer_.get_read_buf(), (std::size_t)read_buffer_.get_read_buf_size()),
			make_custom_alloc_handler(handler_allocator_,
			boost::bind(&Peer_Http::handle_read, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred)));
		LOG_TRACE2("try_read async_read_some get_read_buf_size=" << read_buffer_.get_read_buf_size());
	}
}

bool Peer_Http::try_write() {
	if (op_count_ == 0) {
		if (!write_buf_.empty()) {
			++op_count_;
			async_write(*socket_, write_buf_,
				make_custom_alloc_handler(handler_allocator_,
				boost::bind(&Peer_Http::handle_write, this,
				boost::asio::placeholders::error)));
			LOG_TRACE2("try_write async_write write_buf.count=" << write_buf_.size());
			write_buf_.clear();
			return true;
		}
	}
	return false;
}

uint32_t Peer_Http::read_some(uint8_t* buf, uint32_t length) {
	boost::system::error_code ec;
	if (socket_->available(ec) == 0) {
		return 0;
	}
	return (uint32_t)socket_->read_some(boost::asio::buffer(buf, (std::size_t)length), ec);
}

void Peer_Http::handle_timer(const boost::system::error_code& err, uint32_t watch_id) {
	LOG_DEBUG("Peer_Http::handle_timer err=" << err);
	std::list<uint64_t> updated_list;
	uint32_t updated_count = 0;
	lock_.lock();
	bool ret = cache_mgr_.check_watch_and_clear_callback(watch_id, updated_list, updated_count);
	LOG_DEBUG("handle_timer watch_id=" << watch_id << " updated_count=" << updated_count);

	if (!ret) {
		write_error(XIXI_REASON_WATCH_NOT_FOUND);
	} else {
		encode_update_list(updated_list);
	}
	try_write();
	lock_.unlock();
}

#include "zlib.h"

static void out_uint32(uint8_t* out, uint32_t x) {
    out[0] = (uint8_t)(x & 0xff);
    out[1] = (uint8_t)((x & 0xff00) >> 8);
    out[2] = (uint8_t)((x & 0xff0000) >> 16);
    out[3] = (uint8_t)((x & 0xff000000) >> 24);
}

uint32_t Peer_Http::gzip_encode(uint8_t* data_in, uint32_t data_in_size, vector<Const_Data>& data_out) {
	static const uint8_t gzip_header[10] = { '\037', '\213', Z_DEFLATED, 0,
		  0, 0, 0, 0, // mtime
		  0, 0x03 // Unix OS_CODE
	};
	if (data_in_size == 0) {
		return 0;
	}

	#define DEFAULT_COMPRESSION Z_DEFAULT_COMPRESSION
	#define DEFAULT_WINDOWSIZE -15
	#define DEFAULT_MEMLEVEL 9

	z_stream stream;
	memset(&stream, 0, sizeof(stream));
    int zRC = deflateInit2(&stream, DEFAULT_COMPRESSION, Z_DEFLATED,
                       DEFAULT_WINDOWSIZE, DEFAULT_MEMLEVEL,
                       Z_DEFAULT_STRATEGY);

    if (zRC != Z_OK) {
		LOG_ERROR2("deflateInit2 error, " << zRC);
		deflateEnd(&stream);
		return 0;
    }

	data_out.push_back(Const_Data(gzip_header, 10));
	uint32_t data_out_size = 10;

    stream.next_in = data_in;
    stream.avail_in = data_in_size;

	uint32_t chunk_size = 4096;
	if (data_in_size < 4096) {
		chunk_size = data_in_size;
	}
	uint8_t* chunk = request_buf_.prepare(chunk_size);
	if (chunk == NULL) {
		LOG_ERROR2("gzip_encode, out of memory, " << chunk_size);
		deflateEnd(&stream);
		return 0;
	}
	stream.avail_out = chunk_size;

	stream.next_out= chunk;

    zRC = deflate(&(stream), Z_FINISH);

	while (zRC == Z_OK && stream.avail_out == 0) {
		data_out.push_back(Const_Data(chunk, chunk_size));
		data_out_size += chunk_size;
		chunk = request_buf_.prepare(chunk_size);
		if (chunk == NULL) {
			LOG_ERROR2("gzip_encode, out of memory, " << chunk_size);
			deflateEnd(&stream);
			return 0;
		}
		stream.avail_out = chunk_size;
		stream.next_out= chunk;
		zRC = deflate(&stream, Z_FINISH);
	}
	if (zRC = Z_STREAM_END) {
		uint32_t size = chunk_size - stream.avail_out;
		data_out.push_back(Const_Data(chunk, size));
		data_out_size += size;
	} else if (zRC != Z_OK) {
		LOG_ERROR2("deflateInit2 error, " << zRC);
		deflateEnd(&stream);
		return 0;
    }

	uint8_t* validation = request_buf_.prepare(8);
	if (validation == NULL) {
		LOG_ERROR2("gzip_encode, out of memory, " << 8);
		deflateEnd(&stream);
		return 0;
	}
	uint32_t crc = crc32(0, (const Bytef *)data_in, data_in_size);
    out_uint32(&validation[0], crc);
    out_uint32(&validation[4], stream.total_in);
	data_out.push_back(Const_Data(validation, 8));
	data_out_size += 8;

	zRC = deflateEnd(&stream);
	if (zRC != Z_OK) {
		LOG_ERROR2("deflateEnd error, " << zRC);
	}

	return data_out_size;
}

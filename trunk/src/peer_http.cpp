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

Peer_Http::Peer_Http(boost::asio::ip::tcp::socket* socket) : self_(this) {
	LOG_DEBUG2("Peer_Http::Peer_Http()");
	rop_count_ = 0;
	wop_count_ = 0;
	socket_ = socket;
	read_pdu_ = NULL;
	state_ = PEER_STATE_NEW_CMD;
	next_state_ = PEER_STATE_NEW_CMD;
	cache_item_ = NULL;
	write_buf_total_ = 0;
	read_item_buf_ = NULL;
	swallow_size_ = 0;
	next_data_len_ = XIXI_PDU_HEAD_LENGTH;
	timer_ = NULL;

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
	uint8_t* cb = cache_buf_.prepare(XIXI_PDU_SIMPLE_RES_LENGTH);
	XIXI_Simple_Res_Pdu::encode(cb, choice);

	add_write_buf(cb, XIXI_PDU_SIMPLE_RES_LENGTH);

	set_state(PEER_STATUS_WRITE);
	next_state_ = PEER_STATE_NEW_CMD;
}

void Peer_Http::write_simple_res(xixi_choice choice, uint32_t request_id) {
	uint8_t* cb = cache_buf_.prepare(XIXI_PDU_SIMPLE_RES_WITH_REQID_LENGTH);
	XIXI_Simple_Res_With_ReqID_Pdu::encode(cb, request_id, choice);

	add_write_buf(cb, XIXI_PDU_SIMPLE_RES_WITH_REQID_LENGTH);

	set_state(PEER_STATUS_WRITE);
	next_state_ = PEER_STATE_NEW_CMD;
}

void Peer_Http::write_error(xixi_reason error_code, uint32_t swallow, bool reply) {
	if (reply) {
		uint8_t* cb = cache_buf_.prepare(XIXI_PDU_ERROR_RES_LENGTH);
		XIXI_Error_Res_Pdu::encode(cb, error_code);

		add_write_buf(cb, XIXI_PDU_ERROR_RES_LENGTH);

		set_state(PEER_STATUS_WRITE);
		if (swallow > 0) {
			swallow_size_ = swallow;
			next_state_ = PEER_STATE_SWALLOW;
		} else {
			next_state_ = PEER_STATE_NEW_CMD;
		}
	} else {
		if (swallow > 0) {
			swallow_size_ = swallow;
			set_state(PEER_STATE_SWALLOW);
			next_state_ = PEER_STATE_NEW_CMD;
		} else {
			set_state(PEER_STATE_NEW_CMD);
			next_state_ = PEER_STATE_NEW_CMD;
		}
	}
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

	case PEER_STATE_READ_BODY_FIXED:
		if (read_buffer_.read_data_size_ >= next_data_len_) {
			bool ret = XIXI_Pdu::decode_pdu((uint8_t*)read_pdu_fixed_body_buffer_, read_pdu_header_, read_buffer_.read_curr_, read_buffer_.read_data_size_);
			offset += next_data_len_;
			read_buffer_.read_curr_ += next_data_len_;
			read_buffer_.read_data_size_ -= next_data_len_;
			if (ret) {
				read_pdu_ = (XIXI_Pdu*)read_pdu_fixed_body_buffer_;
				read_pdu_->choice = read_pdu_header_.choice;
				process_pdu_fixed(read_pdu_);
			} else {
				LOG_WARNING2("process can not decode pdu cateory=" << (int)read_pdu_header_.category() << " command=" << (int)read_pdu_header_.command());
				write_error(XIXI_REASON_UNKNOWN_COMMAND, 0, true);
				next_state_ = PEER_STATE_CLOSING;
			}
		} else {
			read_buffer_.handle_processed();
			uint32_t size = read_some(read_buffer_.get_read_buf(), read_buffer_.get_read_buf_size());
			if (size == 0) {
				run = false;
			} else {
				read_buffer_.read_data_size_ += size;
				//  LOG_INFO2("read_some PEER_STATE_READ_BODY_FIXED, size=" << size);
			}
		}
		break;

	case PEER_STATE_READ_BODY_EXTRAS:
		if (next_data_len_ == 0) {
			process_pdu_extras(read_pdu_);
		} else if (read_buffer_.read_data_size_ > 0) {
			tmp = read_buffer_.read_data_size_ > next_data_len_ ? next_data_len_ : read_buffer_.read_data_size_;
			if (read_item_buf_ != read_buffer_.read_curr_) {
				memmove(read_item_buf_, read_buffer_.read_curr_, tmp);
			}
			offset += tmp;
			read_item_buf_ += tmp;
			next_data_len_ -= tmp;
			read_buffer_.read_curr_ += tmp;
			read_buffer_.read_data_size_ -= tmp;
			//  if (next_data_len_ == 0) {
			//    process_pdu_extras(read_pdu_);
			//  }
		} else {
			//  read_buffer_.handle_processed();
			//  uint32_t size = read_some(read_buffer_.get_read_buf(), read_buffer_.get_read_buf_size());
			uint32_t size = read_some(read_item_buf_, next_data_len_);
			if (size == 0) {
				run = false;
			} else {
				offset += size;
				read_item_buf_ += size;
				next_data_len_ -= size;
				//  read_buffer_.read_data_size_ += size;
				//  LOG_INFO2("read_some PEER_STATE_READ_BODY_EXTRAS, size=" << size);
			}
		}
		break;

	case PEER_STATE_READ_BODY_EXTRAS2:
		tmp = process_pdu_extras2(read_pdu_, read_buffer_.read_curr_, read_buffer_.read_data_size_);
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

uint32_t Peer_Http::try_read_command(char* data, uint32_t data_len) {
	LOG_TRACE2("process_header choice=" << read_pdu_header_.choice);

	char* p = memfind(data, data_len, "\r\n\r\n", 4);
	if (p != NULL) {
		*p = '\0';

		assert(p < (data + data_len));

		process_command(data, (uint32_t)(p - data));

		return (p - data + 4);
	}

	if (data_len >= 4096) {
		set_state(PEER_STATE_CLOSING);
		return 0;
	}

	set_state(PEER_STATE_READ_HEADER);
	return 0;
}

void Peer_Http::process_command(char* command, uint32_t length) {
	if (length > 15) {
		uint32_t offset = 0;
		uint32_t method = *((uint32_t*)command);
		bool is_get = true;
		if (method == GET_METHOD) {
			offset = 4;
		} else if (method == POST_METHOD) {
			offset = 5;
			is_get = false;
		} else {
			LOG_WARNING2("try_read_command error method=" << method);
			set_state(PEER_STATE_CLOSING);
			return;
		}
		if (memcmp(command + offset, "/xixibase/", 10) == 0) {
			offset += 10;
			if (memcmp(command + offset, "get?", 4) == 0) {
				process_get(command + offset + 4, length - offset - 4);
			} else if (memcmp(command + offset, "set?", 4) == 0) {
			}
		} else {
			LOG_WARNING2("try_read_command error method=" << method);
			set_state(PEER_STATE_CLOSING);
		}
	}
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
		} else if (*end == ' ' || *end == '\0') {
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

void Peer_Http::process_get(char* command, uint32_t length) {
	uint32_t group_id = 0;
	uint32_t watch_id = 0;
	char* key = NULL;
	uint32_t key_length = 0;
	uint32_t expiration = 0;
	bool touch_flag = false;
	tokens_.clear();
	tokenize_command(command, tokens_);
	for (size_t i = 0; i < tokens_.size(); i++) {
		token_t& t = tokens_[i];
		if (t.length >= 3) {
			if (memcmp(t.value, "g=", 2) == 0) {
				if (!safe_toui32(t.value + 2, t.length, group_id)) {
					// response error
				}
			} else if (memcmp(t.value, "w=", 2) == 0) {
				if (!safe_toui32(t.value + 2, t.length, watch_id)) {
					// response error
				}
			} else if (memcmp(t.value, "k=", 2) == 0) {
				key = t.value + 2;
				key_length = t.length - 2;
			} else if (memcmp(t.value, "e=", 2) == 0) {
				touch_flag = true;
				if (!safe_toui32(t.value + 2, t.length, expiration)) {
					// response error
				}
			}
		}
	}

	Cache_Item* it;
	bool watch_error = false;
	if (touch_flag) {
		it = cache_mgr_.get_touch(group_id, (uint8_t*)key, key_length, watch_id, expiration, watch_error);
	} else {
		it = cache_mgr_.get(group_id, (uint8_t*)key, key_length, watch_id, expiration, watch_error);
	}	

	char* res = "HTTP/1.1 200 OK\r\n\r\n";
	add_write_buf((uint8_t*)res, strlen(res));
	set_state(PEER_STATUS_WRITE);
	next_state_ = PEER_STATE_NEW_CMD;
/*
	if (it != NULL) {
		cache_item_ = it;
		cache_items_.push_back(it);

		uint8_t* cb = cache_buf_.prepare(XIXI_Get_Res_Pdu::calc_encode_size());
		XIXI_Get_Res_Pdu gsp;
		gsp.cache_id = it->cache_id;
		gsp.flags = it->flags;
		gsp.expiration = expiration;
		gsp.data_length = it->data_size;
		gsp.encode(cb);

		add_write_buf(cb, XIXI_Get_Res_Pdu::calc_encode_size());
		add_write_buf(it->get_data(), it->data_size);

		set_state(PEER_STATUS_WRITE);
		next_state_ = PEER_STATE_NEW_CMD;
	} else {
		if (watch_error) {
			write_error(XIXI_REASON_WATCH_NOT_FOUND, 0, true);
		} else {
			write_error(XIXI_REASON_NOT_FOUND, 0, true);
		}
	}*/
}

void Peer_Http::process_pdu_fixed(XIXI_Pdu* pdu) {
	LOG_TRACE2("process_pdu_fixed choice=" << read_pdu_header_.choice);
	switch (read_pdu_header_.choice) {
  case XIXI_CHOICE_GET_REQ:
	  next_data_len_ = ((XIXI_Get_Req_Pdu*)pdu)->key_length;
	  set_state(PEER_STATE_READ_BODY_EXTRAS2);
	  break;
  case XIXI_CHOICE_GET_TOUCH_REQ:
	  next_data_len_ = ((XIXI_Get_Touch_Req_Pdu*)pdu)->key_length;
	  set_state(PEER_STATE_READ_BODY_EXTRAS2);
	  break;
  case XIXI_CHOICE_UPDATE_REQ:
	  process_update_req_pdu_fixed((XIXI_Update_Req_Pdu*)pdu);
	  break;
  case XIXI_CHOICE_UPDATE_FLAGS_REQ:
	  next_data_len_ = ((XIXI_Update_Flags_Req_Pdu*)pdu)->key_length;
	  set_state(PEER_STATE_READ_BODY_EXTRAS2);
	  break;
  case XIXI_CHOICE_UPDATE_EXPIRATION_REQ:
	  next_data_len_ = ((XIXI_Update_Expiration_Req_Pdu*)pdu)->key_length;
	  set_state(PEER_STATE_READ_BODY_EXTRAS2);
	  break;
  case XIXI_CHOICE_DELETE_REQ:
	  process_delete_req_pdu_fixed((XIXI_Delete_Req_Pdu*)pdu);
	  break;
  case XIXI_CHOICE_AUTH_REQ:
	  process_auth_req_pdu_fixed((XIXI_Auth_Req_Pdu*)pdu);
	  break;
  case XIXI_CHOICE_DELTA_REQ:
	  process_delta_req_pdu_fixed((XIXI_Delta_Req_Pdu*)pdu);
	  break;
  case XIXI_CHOICE_GET_BASE_REQ:
	  next_data_len_ = ((XIXI_Get_Base_Req_Pdu*)pdu)->key_length;
	  set_state(PEER_STATE_READ_BODY_EXTRAS2);
	  break;
  case XIXI_CHOICE_FLUSH_REQ:
	  process_flush_req_pdu_fixed((XIXI_Flush_Req_Pdu*)pdu);
	  break;
  case XIXI_CHOICE_STATS_REQ:
	  process_stats_req_pdu_fixed((XIXI_Stats_Req_Pdu*)pdu);
	  break;
  case XIXI_CHOICE_CREATE_WATCH_REQ:
	  process_create_watch_req_pdu_fixed((XIXI_Create_Watch_Req_Pdu*)pdu);
	  break;
  case XIXI_CHOICE_CHECK_WATCH_REQ:
	  process_check_watch_req_pdu_fixed((XIXI_Check_Watch_Req_Pdu*)pdu);
	  break;
  default:
	  LOG_WARNING2("process_pdu_fixed unknown cateory=" << (int)read_pdu_header_.category() << " command=" << (int)read_pdu_header_.command());
	  write_error(XIXI_REASON_UNKNOWN_COMMAND, 0, true);
	  next_state_ = PEER_STATE_CLOSING;
	  break;
	}
}

void Peer_Http::process_pdu_extras(XIXI_Pdu* pdu) {
	LOG_TRACE2("process_pdu_extras choice=" << read_pdu_header_.choice);
	switch (read_pdu_header_.choice) {
  case XIXI_CHOICE_UPDATE_REQ:
	  process_update_req_pdu_extras((XIXI_Update_Req_Pdu*)pdu);
	  break;
  default:
	  LOG_WARNING2("process_pdu_extras unknown cateory=" << (int)read_pdu_header_.category() << " command=" << (int)read_pdu_header_.command());
	  write_error(XIXI_REASON_UNKNOWN_COMMAND, 0, true);
	  next_state_ = PEER_STATE_CLOSING;
	  break;
	}
}

uint32_t Peer_Http::process_pdu_extras2(XIXI_Pdu* pdu, uint8_t* data, uint32_t data_length) {
	LOG_TRACE2("process_pdu_extras2 choice=" << read_pdu_header_.choice << " date_length=" << data_length);
	switch (read_pdu_header_.choice) {
	case XIXI_CHOICE_GET_REQ:
		return process_get_req_pdu_extras((XIXI_Get_Req_Pdu*)pdu, data, data_length);
	case XIXI_CHOICE_GET_TOUCH_REQ:
		return process_get_touch_req_pdu_extras((XIXI_Get_Touch_Req_Pdu*)pdu, data, data_length);
	case XIXI_CHOICE_UPDATE_FLAGS_REQ:
		return process_update_flags_req_pdu_extras((XIXI_Update_Flags_Req_Pdu*)pdu, data, data_length);
	case XIXI_CHOICE_UPDATE_EXPIRATION_REQ:
		return process_update_expiration_req_pdu_extras((XIXI_Update_Expiration_Req_Pdu*)pdu, data, data_length);
	case XIXI_CHOICE_DELETE_REQ:
		return process_delete_req_pdu_extras((XIXI_Delete_Req_Pdu*)pdu, data, data_length);
	case XIXI_CHOICE_AUTH_REQ:
		return process_auth_req_pdu_extras((XIXI_Auth_Req_Pdu*)pdu, data, data_length);
	case XIXI_CHOICE_DELTA_REQ:
		return process_delta_req_pdu_extras((XIXI_Delta_Req_Pdu*)pdu, data, data_length);
	case XIXI_CHOICE_GET_BASE_REQ:
		return process_get_base_req_pdu_extras((XIXI_Get_Base_Req_Pdu*)pdu, data, data_length);
	}
	LOG_WARNING2("process_pdu_extras2 unknown cateory=" << (int)read_pdu_header_.category() << " command=" << (int)read_pdu_header_.command());
	write_error(XIXI_REASON_UNKNOWN_COMMAND, 0, true);
	next_state_ = PEER_STATE_CLOSING;
	return 0;
}

uint32_t Peer_Http::process_get_req_pdu_extras(XIXI_Get_Req_Pdu* pdu, uint8_t* data, uint32_t data_length) {
	LOG_TRACE2("process_get_req_pdu_extras");
	uint8_t* key = data;
	size_t key_length = pdu->key_length;
	if (data_length < key_length) {
		return 0;
	}

	bool watch_error = false;
	uint32_t expiration;
	Cache_Item* it = cache_mgr_.get(pdu->group_id, key, key_length, pdu->watch_id, expiration, watch_error);
	if (it != NULL) {
		cache_item_ = it;
		cache_items_.push_back(it);

		uint8_t* cb = cache_buf_.prepare(XIXI_Get_Res_Pdu::calc_encode_size());
		XIXI_Get_Res_Pdu gsp;
		gsp.cache_id = it->cache_id;
		gsp.flags = it->flags;
		gsp.expiration = expiration;
		gsp.data_length = it->data_size;
		gsp.encode(cb);

		add_write_buf(cb, XIXI_Get_Res_Pdu::calc_encode_size());
		add_write_buf(it->get_data(), it->data_size);

		set_state(PEER_STATUS_WRITE);
		next_state_ = PEER_STATE_NEW_CMD;
	} else {
		if (watch_error) {
			write_error(XIXI_REASON_WATCH_NOT_FOUND, 0, true);
		} else {
			write_error(XIXI_REASON_NOT_FOUND, 0, true);
		}
	}

	return key_length;
}

uint32_t Peer_Http::process_get_touch_req_pdu_extras(XIXI_Get_Touch_Req_Pdu* pdu, uint8_t* data, uint32_t data_length) {
	LOG_TRACE2("process_get_touch_req_pdu_extras");
	uint8_t* key = data;
	size_t key_length = pdu->key_length;
	if (data_length < key_length) {
		return 0;
	}


	bool watch_error = false;
	Cache_Item* it = cache_mgr_.get_touch(pdu->group_id, key, key_length, pdu->watch_id, pdu->expiration, watch_error);
	if (it != NULL) {
		cache_item_ = it;  
		cache_items_.push_back(it);

		uint8_t* cb = cache_buf_.prepare(XIXI_Get_Res_Pdu::calc_encode_size());
		XIXI_Get_Res_Pdu gsp;
		gsp.cache_id = it->cache_id;
		gsp.flags = it->flags;
		gsp.expiration = pdu->expiration;
		gsp.data_length = it->data_size;
		gsp.encode(cb);

		add_write_buf(cb, XIXI_Get_Res_Pdu::calc_encode_size());
		add_write_buf(it->get_data(), it->data_size);

		set_state(PEER_STATUS_WRITE);
		next_state_ = PEER_STATE_NEW_CMD;
	} else {
		if (watch_error) {
			write_error(XIXI_REASON_WATCH_NOT_FOUND, 0, true);
		} else {
			write_error(XIXI_REASON_NOT_FOUND, 0, true);
		}
	}

	return key_length;
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
		uint8_t* cb = cache_buf_.prepare(XIXI_Get_Base_Res_Pdu::calc_encode_size());
		XIXI_Get_Base_Res_Pdu rs;
		rs.cache_id = cache_id;
		rs.flags = flags;
		rs.expiration = expiration;
		rs.encode(cb);

		add_write_buf(cb, XIXI_Get_Base_Res_Pdu::calc_encode_size());

		set_state(PEER_STATUS_WRITE);
		next_state_ = PEER_STATE_NEW_CMD;
	} else {
		write_error(XIXI_REASON_NOT_FOUND, 0, true);
	}

	return key_length;
}

void Peer_Http::process_update_req_pdu_fixed(XIXI_Update_Req_Pdu* pdu) {
	LOG_TRACE2("process_update_req_pdu_fixed");
	//  if (pdu->key_length == 0) {
	//    write_error(read_pdu_header_, XIXI_REASON_INVALID_PARAMETER, pdu->data_length);
	//    return;
	//  }

	uint32_t data_len = pdu->key_length + pdu->data_length;

	cache_item_ = cache_mgr_.alloc_item(pdu->group_id, pdu->key_length, pdu->flags,
		pdu->expiration, pdu->data_length);

	if (cache_item_ == NULL) {
		if (cache_mgr_.item_size_ok(pdu->key_length, pdu->data_length)) {
			write_error(XIXI_REASON_OUT_OF_MEMORY, data_len, pdu->reply());
		} else {
			write_error(XIXI_REASON_TOO_LARGE, data_len, pdu->reply());
		}
	} else {
		read_item_buf_ = cache_item_->get_key();
		next_data_len_ = data_len;
		set_state(PEER_STATE_READ_BODY_EXTRAS);
	}
}

void Peer_Http::process_update_req_pdu_extras(XIXI_Update_Req_Pdu* pdu) {
	LOG_TRACE2("process_update_req_pdu_extras");
	uint8_t* key = cache_item_->get_key();
	uint8_t* data = cache_item_->get_data();

	uint32_t data_len = pdu->key_length + pdu->data_length;

	cache_item_->calc_hash_value();

	cache_item_->cache_id = pdu->cache_id;

	uint64_t cache_id;
	xixi_reason reason;

	switch (pdu->sub_op()) {
	case XIXI_UPDATE_SUB_OP_SET:
		reason = cache_mgr_.set(cache_item_, pdu->watch_id, cache_id);
		break;
	case XIXI_UPDATE_SUB_OP_ADD:
		reason = cache_mgr_.add(cache_item_, pdu->watch_id, cache_id);
		break;
	case XIXI_UPDATE_SUB_OP_REPLACE:
		reason = cache_mgr_.replace(cache_item_, pdu->watch_id, cache_id);
		break;
	case XIXI_UPDATE_SUB_OP_APPEND:
		reason = cache_mgr_.append(cache_item_, pdu->watch_id, cache_id);
		break;
	case XIXI_UPDATE_SUB_OP_PREPEND:
		reason = cache_mgr_.prepend(cache_item_, pdu->watch_id, cache_id);
		break;
	default:
		reason = XIXI_REASON_UNKNOWN_COMMAND;
		break;
	}
	if (pdu->reply()) {
		if (reason == XIXI_REASON_SUCCESS) {
			uint8_t* cb = cache_buf_.prepare(XIXI_Update_Res_Pdu::calc_encode_size());
			XIXI_Update_Res_Pdu::encode(cb, cache_id);

			add_write_buf(cb, XIXI_Update_Res_Pdu::calc_encode_size());

			set_state(PEER_STATUS_WRITE);
			next_state_ = PEER_STATE_NEW_CMD;
		} else {
			write_error(reason, 0, true);
		}
	} else {
		set_state(PEER_STATE_NEW_CMD);
		next_state_ = PEER_STATE_NEW_CMD;
	}

	cache_mgr_.release_reference(cache_item_);
	cache_item_ = NULL;
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
			uint8_t* cb = cache_buf_.prepare(XIXI_Update_Flags_Res_Pdu::calc_encode_size());
			XIXI_Update_Flags_Res_Pdu::encode(cb, cache_id);

			add_write_buf(cb, XIXI_Update_Flags_Res_Pdu::calc_encode_size());

			set_state(PEER_STATUS_WRITE);
			next_state_ = PEER_STATE_NEW_CMD;
		} else {
			if (cache_id != 0) {
				write_error(XIXI_REASON_MISMATCH, 0, true);
			} else {
				write_error(XIXI_REASON_NOT_FOUND, 0, true);
			}
		}
	} else {
		set_state(PEER_STATE_NEW_CMD);
		next_state_ = PEER_STATE_NEW_CMD;
	}
	return key_length;
}

uint32_t Peer_Http::process_update_expiration_req_pdu_extras(XIXI_Update_Expiration_Req_Pdu* pdu, uint8_t* data, uint32_t data_length) {
	LOG_TRACE2("process_update_expiration_req_pdu_extras");
	uint8_t* key = data;
	size_t key_length = pdu->key_length;
	if (data_length < key_length) {
		return 0;
	}
	uint64_t cache_id = 0;
	bool ret = cache_mgr_.update_expiration(pdu->group_id, key, key_length, pdu, cache_id);
	if (pdu->reply()) {
		if (ret) {
			uint8_t* cb = cache_buf_.prepare(XIXI_Update_Expiration_Res_Pdu::calc_encode_size());
			XIXI_Update_Expiration_Res_Pdu::encode(cb, cache_id);

			add_write_buf(cb, XIXI_Update_Expiration_Res_Pdu::calc_encode_size());

			set_state(PEER_STATUS_WRITE);
			next_state_ = PEER_STATE_NEW_CMD;
		} else {
			if (cache_id != 0) {
				write_error(XIXI_REASON_MISMATCH, 0, true);
			} else {
				write_error(XIXI_REASON_NOT_FOUND, 0, true);
			}
		}
	} else {
		set_state(PEER_STATE_NEW_CMD);
		next_state_ = PEER_STATE_NEW_CMD;
	}
	return key_length;
}

void Peer_Http::process_delete_req_pdu_fixed(XIXI_Delete_Req_Pdu* pdu) {
	LOG_TRACE2("process_delete_req_pdu_fixed");
	next_data_len_ = pdu->key_length;
	set_state(PEER_STATE_READ_BODY_EXTRAS2);
}

uint32_t Peer_Http::process_delete_req_pdu_extras(XIXI_Delete_Req_Pdu* pdu, uint8_t* data, uint32_t data_length) {
	LOG_TRACE2("process_delete_req_pdu_extras");
	uint8_t* key = data;
	uint32_t key_length = pdu->key_length;

	if (data_length < pdu->key_length) {
		return 0;
	}

	LOG_DEBUG2("Deleteing " << string((char*)key, key_length));

	xixi_reason reason = cache_mgr_.remove(pdu->group_id, key, key_length, pdu->cache_id);
	if (pdu->reply()) {
		if (reason == XIXI_REASON_SUCCESS) {
			write_simple_res(XIXI_CHOICE_DELETE_RES);
		} else {
			write_error(reason, 0, true);
		}
	} else {
		set_state(PEER_STATE_NEW_CMD);
		next_state_ = PEER_STATE_NEW_CMD;
	}
	return key_length;
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

void Peer_Http::process_delta_req_pdu_fixed(XIXI_Delta_Req_Pdu* pdu) {
	LOG_TRACE2("process_delta_req_pdu_fixed");
	next_data_len_ = pdu->key_length;
	set_state(PEER_STATE_READ_BODY_EXTRAS2);
}

uint32_t Peer_Http::process_delta_req_pdu_extras(XIXI_Delta_Req_Pdu* pdu, uint8_t* data, uint32_t data_length) {
	LOG_TRACE2("process_delta_req_pdu_extras");
	uint8_t* key = data;
	uint32_t key_length = pdu->key_length;

	if (data_length < pdu->key_length) {
		return 0;
	}

	LOG_DEBUG2("Delta " << string((char*)key, key_length));

	int64_t value;
	uint64_t cache_id = pdu->cache_id;
	xixi_reason reason = cache_mgr_.delta(pdu->group_id, key, key_length, pdu->sub_op() == XIXI_DELTA_SUB_OP_INCR, pdu->delta, cache_id, value);
	if (pdu->reply()) {
		if (reason == XIXI_REASON_SUCCESS) {
			uint8_t* cb = cache_buf_.prepare(XIXI_Delta_Res_Pdu::calc_encode_size());
			XIXI_Delta_Res_Pdu::encode(cb, value, cache_id);

			add_write_buf(cb, XIXI_Delta_Res_Pdu::calc_encode_size());

			set_state(PEER_STATUS_WRITE);
			next_state_ = PEER_STATE_NEW_CMD;
		} else {
			write_error(reason, 0, true);
		}
	} else {
		set_state(PEER_STATE_NEW_CMD);
		next_state_ = PEER_STATE_NEW_CMD;
	}

	return key_length;
}

void Peer_Http::process_hello_req_pdu_fixed() {
	LOG_TRACE2("process_hello_req_pdu_fixed");
	write_simple_res(XIXI_CHOICE_HELLO_RES);
}

void Peer_Http::process_create_watch_req_pdu_fixed(XIXI_Create_Watch_Req_Pdu* pdu) {
	LOG_INFO2("process_create_watch_req_pdu_fixed");
	uint32_t watchID = cache_mgr_.create_watch(pdu->group_id, pdu->max_next_check_interval);

	uint8_t* cb = cache_buf_.prepare(XIXI_Create_Watch_Res_Pdu::get_body_size());
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
		write_error(XIXI_REASON_WATCH_NOT_FOUND, 0, true);
	} else {
		if (updated_count > 0) {
			uint32_t size = XIXI_Check_Watch_Res_Pdu::calc_encode_size(updated_count);

			uint8_t* buf = cache_buf_.prepare(size);
			if (buf != NULL) {
				XIXI_Check_Watch_Res_Pdu::encode(buf, updated_count, updated_list);

				add_write_buf(buf, size);

				set_state(PEER_STATUS_WRITE);
				next_state_ = PEER_STATE_NEW_CMD;
			} else {
				write_error(XIXI_REASON_OUT_OF_MEMORY, 0, true);
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

void Peer_Http::process_flush_req_pdu_fixed(XIXI_Flush_Req_Pdu* pdu) {
	uint8_t* cb = cache_buf_.prepare(XIXI_Flush_Res_Pdu::calc_encode_size());
	XIXI_Flush_Res_Pdu res_pdu;
	cache_mgr_.flush(pdu->group_id, res_pdu.flush_count, res_pdu.flush_size);
	res_pdu.encode(cb);

	add_write_buf(cb, XIXI_Flush_Res_Pdu::calc_encode_size());

	set_state(PEER_STATUS_WRITE);
	next_state_ = PEER_STATE_NEW_CMD;
}

void Peer_Http::process_stats_req_pdu_fixed(XIXI_Stats_Req_Pdu* pdu) {
	std::string result;
	cache_mgr_.stats(pdu, result);
	int size = result.size();
	int buf_size = XIXI_Stats_Res_Pdu::calc_encode_size(size);
	uint8_t* buf = cache_buf_.prepare(buf_size);
	if (buf != NULL) {
		XIXI_Stats_Res_Pdu::encode(buf, (uint8_t*)result.c_str(), size);

		add_write_buf(buf, buf_size);

		set_state(PEER_STATUS_WRITE);
		next_state_ = PEER_STATE_NEW_CMD;
	} else {
		write_error(XIXI_REASON_OUT_OF_MEMORY, 0, true);
	}
}

void Peer_Http::reset_for_new_cmd() {
	read_pdu_ = NULL;
	cache_item_ = NULL;
	for (uint32_t i = 0; i < cache_items_.size(); i++) {
		cache_mgr_.release_reference(cache_items_[i]);
	}
	cache_items_.clear();

	cache_buf_.reset();
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
//		read_buffer_.handle_processed();

		if (!is_closed()) {
			uint32_t size = try_write();
			if (size == 0) {
				try_read();
			}
		} else {
			LOG_INFO2("Peer_Http::start closed");
			socket_->get_io_service().post(boost::bind(&Peer_Http::destroy, this));
		}
	} else {
		LOG_INFO2("Peer_Http::start invalid parameter:data_length=" << data_length);
		socket_->get_io_service().post(boost::bind(&Peer_Http::destroy, this));
	}
	lock_.unlock();
}

void Peer_Http::handle_read(const boost::system::error_code& err, size_t length) {
	LOG_TRACE2("handle_read length=" << length << " err=" << err.message() << " err_value=" << err.value());

	lock_.lock();
	--rop_count_;
	if (!err) {
		read_buffer_.read_data_size_ += length;
		process();
	//	read_buffer_.handle_processed();

		if (state_ != PEER_STATUS_ASYNC_WAIT) {
			if (!is_closed()) {
				uint32_t size = try_write();
				if (size == 0) {
					try_read();
				}
			} else {
				LOG_DEBUG2("handle_read peer is closed, rop_count_=" << rop_count_ << " wop_count_=" << wop_count_);
			}
		} else {
			try_write();
		}
	} else {
		LOG_DEBUG2("handle_read error rop_count_=" << rop_count_ << " wop_count_=" << wop_count_ << " err=" << err);
	}

	if (rop_count_ + wop_count_ == 0 && state_ != PEER_STATUS_ASYNC_WAIT) {
		LOG_DEBUG2("handle_read destroy rop_count_=" << rop_count_ << " wop_count_=" << wop_count_);
		socket_->get_io_service().post(boost::bind(&Peer_Http::destroy, this));
	} else {
		LOG_TRACE2("handle_read end rop_count_=" << rop_count_ << " wop_count_=" << wop_count_);
	}
	lock_.unlock();
}

void Peer_Http::handle_write(const boost::system::error_code& err) {
	LOG_TRACE2("handle_write err=" << err.message() << " err_value=" << err.value());

	lock_.lock();
	--wop_count_;
	if (!err) {
		write_buf_.clear();
		process();
	//	read_buffer_.handle_processed();

		if (state_ != PEER_STATUS_ASYNC_WAIT) {
			if (!is_closed()) {
				uint32_t size = try_write();
				if (size == 0) {
					try_read();
				}
			} else {
				LOG_DEBUG2("handle_write peer is closed, rop_count_=" << rop_count_ << " wop_count_=" << wop_count_);
			}
		} else {
			try_write();
		}
	}

	if (rop_count_ + wop_count_ == 0 && state_ != PEER_STATUS_ASYNC_WAIT) {
		LOG_DEBUG2("handle_write destroy rop_count_=" << rop_count_ << " wop_count_=" << wop_count_);
		socket_->get_io_service().post(boost::bind(&Peer_Http::destroy, this));
	} else {
		LOG_TRACE2("handle_write end rop_count_=" << rop_count_ << " wop_count_=" << wop_count_);
	}
	lock_.unlock();
}

void Peer_Http::try_read() {
	if (rop_count_ == 0) {
		++rop_count_;
		read_buffer_.handle_processed();
		socket_->async_read_some(boost::asio::buffer(read_buffer_.get_read_buf(), read_buffer_.get_read_buf_size()),
			make_custom_alloc_handler(handler_allocator_,
			boost::bind(&Peer_Http::handle_read, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred)));
		LOG_TRACE2("try_read async_read_some get_read_buf_size=" << read_buffer_.get_read_buf_size());
	}
}

uint32_t Peer_Http::try_write() {
	if (wop_count_ == 0) {
		if (!write_buf_.empty()) {
			++wop_count_;
			async_write(*socket_, write_buf_,
				make_custom_alloc_handler(handler_allocator_,
				boost::bind(&Peer_Http::handle_write, this,
				boost::asio::placeholders::error)));
			LOG_TRACE2("try_write async_write write_buf.count=" << write_buf_.size());
		}
		return write_buf_.size();
	}
	return 0;
}

uint32_t Peer_Http::read_some(uint8_t* buf, uint32_t length) {
	boost::system::error_code ec;
	if (socket_->available(ec) == 0) {
		return 0;
	}
	return socket_->read_some(boost::asio::buffer(buf, length), ec);
}

static volatile uint32_t lastDestroyPeer = 0;

void Peer_Http::destroy(Peer_Http* peer) {
	lastDestroyPeer = peer->peer_id_;
	peer->next_state_ = PEER_STATE_CLOSED;
	peer->self_.reset();
}

void Peer_Http::handle_timer(const boost::system::error_code& err, uint32_t watch_id) {
	//  LOG_INFO("Peer_Http::handle_timer err=" << err);
	std::list<uint64_t> updated_list;
	uint32_t updated_count = 0;
	lock_.lock();
	bool ret = cache_mgr_.check_watch_and_clear_callback(watch_id, updated_list, updated_count);
	//  LOG_INFO2("handle_timer watch_id=" << watch_id << " updated_count=" << updated_count);

	if (!ret) {
		write_error(XIXI_REASON_WATCH_NOT_FOUND, 0, true);
	} else {
		uint32_t size = XIXI_Check_Watch_Res_Pdu::calc_encode_size(updated_count);

		uint8_t* buf = cache_buf_.prepare(size);
		if (buf != NULL) {
			XIXI_Check_Watch_Res_Pdu::encode(buf, updated_count, updated_list);

			add_write_buf(buf, size);

			set_state(PEER_STATE_NEW_CMD);
			next_state_ = PEER_STATE_NEW_CMD;
			try_write();
		} else {
			write_error(XIXI_REASON_OUT_OF_MEMORY, 0, true);
		}
	}

	delete timer_;
	timer_ = NULL;
	lock_.unlock();
}

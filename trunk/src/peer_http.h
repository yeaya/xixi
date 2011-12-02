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

#ifndef PEER_HTTP_H
#define PEER_HTTP_H

#include "defines.h"
#include <boost/asio/buffer.hpp>
#include <boost/asio.hpp>
#include "cache_buffer.hpp"
#include "util.h"
#include "cache.h"
#include "peer.h"
#include "peer_cache_pdu.h"
#include "handler_allocator.hpp"

#define IS_METHOD(data, b0, b1, b2, b3) (data[0] == b0 && data[1] == b1 &&data[2] == b2 && data[3] == b3)

const uint32_t HEAD_METHOD = 'H'; // "HEAD"
const uint32_t GET_METHOD = 'G'; // "GET "
const uint32_t POST_METHOD = 'P'; // "POST"

typedef struct token_s {
    char* value;
    uint32_t length;
} token_t;

class Http_Request {
public:
	Http_Request() {
		reset();
	}
	void reset() {
		http_11 = false;
		keepalive = false;
		method = 0;
		uri = NULL;
		uri_length = 0;
		content_type = NULL;
		content_type_length = 0;
		entity_tag = NULL;
		entity_tag_length = 0;
		boundary = NULL;
		boundary_length = 0;
	}
	bool http_11;
	bool keepalive;
	uint32_t method;
	char* uri;
	uint32_t uri_length;
	char* content_type;
	uint32_t content_type_length;
	char* entity_tag;
	uint32_t entity_tag_length;
	char* boundary;
	uint32_t boundary_length;
};

////////////////////////////////////////////////////////////////////////////////
// Peer_Http
//
class Peer_Http : public Peer, public Cache_Watch_Sink {
public:
	Peer_Http(boost::asio::ip::tcp::socket* socket);

	virtual ~Peer_Http();
	void start(uint8_t* data, uint32_t data_length);

protected:
	virtual void on_cache_watch_notify(uint32_t watch_id);

	void process();
	inline bool is_closed() { return state_ == PEER_STATE_CLOSED; }

	inline void set_state(peer_state state);

	inline uint32_t try_read_command(char* data, uint32_t data_len);
	inline void process_request_header(char* request_header, uint32_t length);
	inline bool process_request_header_fields(char* request_header_field, uint32_t length);
	inline bool handle_request_header_field(char* name, uint32_t name_length, char* value, uint32_t value_length);
	inline void process_command();
	inline bool process_request_arg(char* agr);
	inline char* decode_uri(char* uri, uint32_t length, uint32_t& out);
	inline void process_post();

	// get
	inline void process_get();

	// get cache item
	inline Cache_Item* get_cache_item(bool is_base, xixi_reason& reason, uint32_t& expiration);

	// get welcome file
	Cache_Item* get_welcome_file(bool is_base, xixi_reason& reason, uint32_t& expiration);

	// get base
	inline void process_get_base();

	// update
	inline void process_update(uint8_t sub_op);

	// update base
	inline void process_update_flags();

	// update expiration
	inline void process_touch();

	// delete
	inline void process_delete();

	// auth
	inline void process_auth(XIXI_Auth_Req_Pdu* pdu);
	inline uint32_t process_auth_req_pdu_extras(XIXI_Auth_Req_Pdu* pdu, uint8_t* data, uint32_t data_length);

	// delta
	inline void process_delta(bool incr);

	// create watch
	inline void process_create_watch();

	// check watch
	inline void process_watch();

	// flush
	inline void process_flush();

	// stats
	inline void process_stats();

	inline void reset_for_new_cmd();
	inline void write_error(xixi_reason error_code);

	inline void cleanup();

	inline void handle_read(const boost::system::error_code& err, std::size_t length);

	inline void handle_write(const boost::system::error_code& err);

	inline void try_read();
	inline bool try_write();
	inline uint32_t read_some(uint8_t* buf, uint32_t length);
	inline void add_write_buf(const uint8_t* buf, uint32_t size) {
		write_buf_.push_back(boost::asio::const_buffer(buf, size));
		write_buf_total_ += size;
	}
	inline void update_write_buf(uint32_t index, const uint8_t* buf, uint32_t size) {
		if (write_buf_.size() > index) {
			write_buf_[index] = boost::asio::const_buffer(buf, size);
			write_buf_total_ += size;
		}
	}
	void pop_write_buf(uint32_t pop_count) {
		for (std::size_t i = 0; i < pop_count && write_buf_.size() > 0; i++) {
			write_buf_.pop_back();
		}
	}
	inline void encode_update_list(std::list<uint64_t>& updated_list);

	void handle_timer(const boost::system::error_code& err, uint32_t watch_id);

protected:
	boost::shared_ptr<Peer_Http> self_;

	mutex lock_;
	mutex timer_lock_;
	peer_state  state_;
	peer_state  next_state_;

	uint32_t next_data_len_;

	Http_Request http_request_;

	vector<token_t> tokens_;
	uint32_t group_id_;
	uint32_t watch_id_;
	uint64_t cache_id_;
	uint8_t* key_;
	uint32_t key_length_;
	uint8_t* value_;
	uint32_t value_length_;
	char* value_content_type_;
	uint32_t value_content_type_length_;
	uint8_t* post_data_;
	uint32_t content_length_;
	uint32_t flags_;
	uint32_t expiration_;
	bool touch_flag_;
	int64_t delta_;
	uint64_t ack_cache_id_;
	uint32_t interval_;
	uint32_t timeout_;
	uint32_t sub_op_;

	uint32_t write_buf_total_;

	uint8_t* read_item_buf_;

	Cache_Item* cache_item_;
//	vector<Cache_Item*> cache_items_;

	Cache_Buffer<2048> request_buf_;

	Receive_Buffer<2048, 8192> read_buffer_;
	vector<boost::asio::const_buffer> write_buf_;

	boost::asio::deadline_timer timer_;
	bool timer_flag_;

	boost::asio::ip::tcp::socket* socket_;
	int op_count_;
	Handler_Allocator<> handler_allocator_;
};

#endif // PEER_HTTP_H

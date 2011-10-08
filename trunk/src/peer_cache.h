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

#ifndef PEER_CACHE_H
#define PEER_CACHE_H

#include "xixi_buffer.hpp"
#include "cache_buffer.hpp"
#include "util.h"
#include "cache.h"
#include "peer.h"
#include "peer_cache_pdu.h"
#include "handler_allocator.hpp"

////////////////////////////////////////////////////////////////////////////////
// Peer_Cache
//
class Peer_Cache : public Peer, public Cache_Watch_Sink {
public:
  Peer_Cache(boost::asio::ip::tcp::socket* socket);

  virtual ~Peer_Cache();

  uint32_t process();
  void close() {}
  bool is_closed() { return state_ == PEER_STATE_CLOSED; }
  void start(uint8_t* data, uint32_t data_length);

  virtual void on_cache_watch_notify(uint32_t watch_id) {
    lock_.lock();
    if (timer_ != NULL) {
      timer_->cancel();
    }
    lock_.unlock();
  }

protected:
  inline void set_state(peer_state state);

  inline uint32_t process_header(uint8_t* data, uint32_t data_len);
  inline void process_pdu_fixed(XIXI_Pdu* pdu);
  inline void process_pdu_extras(XIXI_Pdu* pdu);
  inline uint32_t process_pdu_extras2(XIXI_Pdu* pdu, uint8_t* data, uint32_t data_length);

  // get
  inline uint32_t process_get_req_pdu_extras(XIXI_Get_Req_Pdu* pdu, uint8_t* data, uint32_t data_length);

  // get touch
  inline uint32_t process_get_touch_req_pdu_extras(XIXI_Get_Touch_Req_Pdu* pdu, uint8_t* data, uint32_t data_length);

  // get base
  inline uint32_t process_get_base_req_pdu_extras(XIXI_Get_Base_Req_Pdu* pdu, uint8_t* data, uint32_t data_length);

  // update
  inline void process_update_req_pdu_fixed(XIXI_Update_Req_Pdu* pdu);
  inline void process_update_req_pdu_extras(XIXI_Update_Req_Pdu* pdu);

  // update base
  inline uint32_t process_update_base_req_pdu_extras(XIXI_Update_Base_Req_Pdu* pdu, uint8_t* data, uint32_t data_length);

  // delete
  inline void process_delete_req_pdu_fixed(XIXI_Delete_Req_Pdu* pdu);
  inline uint32_t process_delete_req_pdu_extras(XIXI_Delete_Req_Pdu* pdu, uint8_t* data, uint32_t data_length);

  // auth
  inline void process_auth_req_pdu_fixed(XIXI_Auth_Req_Pdu* pdu);
  inline uint32_t process_auth_req_pdu_extras(XIXI_Auth_Req_Pdu* pdu, uint8_t* data, uint32_t data_length);

  // delta
  inline void process_delta_req_pdu_fixed(XIXI_Delta_Req_Pdu* pdu);
  inline uint32_t process_delta_req_pdu_extras(XIXI_Delta_Req_Pdu* pdu, uint8_t* data, uint32_t data_length);

  // hello
  inline void process_hello_req_pdu_fixed();

  // create watch
  inline void process_create_watch_req_pdu_fixed(XIXI_Create_Watch_Req_Pdu* pdu);

  // check watch
  inline void process_check_watch_req_pdu_fixed(XIXI_Check_Watch_Req_Pdu* pdu);

  // flush
  inline void process_flush_req_pdu_fixed(XIXI_Flush_Req_Pdu* pdu);

  // stats
  inline void process_stats_req_pdu_fixed(XIXI_Stats_Req_Pdu* pdu);

  inline void reset_for_new_cmd();
  inline void write_simple_res(xixi_choice choice, uint32_t request_id);
  inline void write_simple_res(xixi_choice choice);
  inline void write_error(xixi_reason error_code, uint32_t swallow, bool reply);

  inline void cleanup();

  inline void handle_read(const boost::system::error_code& err, size_t length);

  inline void handle_write(const boost::system::error_code& err);

  inline void try_read();
  inline uint32_t try_write();
  inline uint32_t read_some(uint8_t* buf, uint32_t length);
  inline void add_write_buf(const uint8_t* buf, uint32_t size) {
    write_buf_.push_back(boost::asio::const_buffer(buf, size));
    write_buf_total_ += size;
  }

  static void destroy(Peer_Cache* peer);

  void handle_timer(const boost::system::error_code& err, uint32_t watch_id);

protected:
  boost::shared_ptr<Peer_Cache> self_;

  mutex lock_;
  peer_state  state_;
  peer_state  next_state_;

  uint32_t next_data_len_;

  XIXI_Pdu_Header read_pdu_header_;
  XIXI_Pdu* read_pdu_;
  uint8_t read_pdu_fixed_body_buffer_[MAX_PDU_FIXED_BODY_LENGTH];

  uint32_t write_buf_total_;

  uint8_t* read_item_buf_;

  Cache_Item* cache_item_;
  vector<Cache_Item*> cache_items_;

  uint32_t    swallow_size_;

  Cache_Buffer<MAX_PDU_FIXED_LENGTH * 5> cache_buf_;

  Receive_Buffer<2048, 8192> read_buffer_;
  vector<boost::asio::const_buffer> write_buf_;

  boost::asio::deadline_timer* timer_;

  boost::asio::ip::tcp::socket* socket_;
  int rop_count_;
  int wop_count_;
  Handler_Allocator<> handler_allocator_;
};

#endif // PEER_CACHE_H


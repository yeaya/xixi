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

#ifndef PEER_XIXI_HA_CLIENT_H
#define PEER_XIXI_HA_CLIENT_H

#include "xixi_buffer.hpp"
#include "cache_buffer.hpp"
#include "util.h"
#include "peer.h"
#include "peer_ha_pdu.h"
#include "concurrent.h"
#include "session.h"
#include "handler_allocator.hpp"

////////////////////////////////////////////////////////////////////////////////
// Peer_XIXI_HA_CLI
//
class Peer_XIXI_HA_CLI : public Peer {
public:
  Peer_XIXI_HA_CLI(const std::string& host_name, const std::string& service_name);
  virtual ~Peer_XIXI_HA_CLI();

  virtual void close();
  virtual void dispatch_message(XIXI_Pdu* pdu);

  void connect();
  void create_committee();
  void announce_leader();
  void keepalive();
  virtual void send_data();
  bool is_present_responsed() {
    return present_res_flag_;
  }
  bool is_keepalive_timeout(uint32_t timeout);
  std::string& get_server_id() {
    return server_id_;
  }
  bool need_reconnect() {
    return false;
  }

protected:
  virtual uint32_t process();
  virtual bool is_closed() { return state_ == PEER_STATE_CLOSED; }
  virtual uint32_t get_tcp_write_buf(vector<boost::asio::const_buffer>& bufs, uint32_t max_count, uint32_t max_size);

  inline void set_state(peer_state state);

  inline uint32_t process_header(uint8_t* data, uint32_t data_len);
  inline void process_pdu_fixed(XIXI_Pdu* pdu);
  inline void process_pdu_extras(XIXI_Pdu* pdu);
  inline uint32_t process_pdu_extras2(XIXI_Pdu* pdu, uint8_t* data, uint32_t data_length);

  // present res
  inline uint32_t process_ha_present_res_pdu(uint8_t* data, uint32_t data_length);

  // create committee res
  inline uint32_t process_ha_create_committee_res_pdu(uint8_t* data, uint32_t data_length);

  // announce leader res
  inline uint32_t process_ha_announce_leader_res_pdu(uint8_t* data, uint32_t data_length);

  // keepalive res
  inline uint32_t process_ha_keepalive_res_pdu(uint8_t* data, uint32_t data_length);

  // MUTEX create
  inline uint32_t process_ha_mutex_create_res_pdu(XIXI_HA_Mutex_Create_Res_Pdu* pdu, uint8_t* data, uint32_t data_length);

  // MUTEX lock
  inline uint32_t process_ha_mutex_lock_res_pdu(XIXI_HA_Mutex_Lock_Res_Pdu* pdu, uint8_t* data, uint32_t data_length);

  // MUTEX unlock
  inline uint32_t process_ha_mutex_unlock_res_pdu(XIXI_HA_Mutex_Unlock_Res_Pdu* pdu, uint8_t* data, uint32_t data_length);

  // MUTEX destory
  inline uint32_t process_ha_mutex_destroy_res_pdu(XIXI_HA_Mutex_Destroy_Res_Pdu* pdu, uint8_t* data, uint32_t data_length);

  inline void reset_for_new_cmd();
  inline void write_error(xixi_reason error_code, uint32_t swallow, bool reply);
  inline void write_error(uint32_t request_id, xixi_reason error_code, uint32_t swallow, bool reply);
  inline void write_one(uint32_t length);
  inline void write_one(uint8_t* buf, uint32_t length);

  inline void cleanup();

  // socket
  void handle_resolve(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_it);
  void handle_connect(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_it);

  inline void handle_read(const boost::system::error_code& err, size_t length);

  inline void handle_write(const boost::system::error_code& err);

  inline void try_read();
  inline uint32_t try_write();
  inline void do_write();

  static void destroy(Peer_XIXI_HA_CLI* peer);

protected:
  mutex lock_;
  peer_state  state_;
  peer_state  next_state_;

  uint32_t next_data_len_;

  XIXI_Pdu_Header read_pdu_header_;
  XIXI_Pdu* read_pdu_;

  list<Const_Data> write_bufs_;
  uint32_t write_buf_total_;

  uint8_t* read_item_buf_;

  uint32_t    swallow_size_;

  Cache_Buffer<1024> cache_buf_;
  uint8_t write_pdu_fixed_buffer[MAX_PDU_FIXED_LENGTH];
  list<XIXI_Pdu*> pdu_list_;

  bool present_res_flag_;
  uint32_t keepalive_res_timestamp_;
  std::string server_id_;

  // socket
  boost::asio::ip::tcp::socket* socket_;

  Receive_Buffer<2048, 8192> read_buffer_;

  vector<boost::asio::const_buffer> write_buf_;

  int rop_count_;
  int wop_count_;
  Handler_Allocator<> read_allocator_;
  Handler_Allocator<> write_allocator_;

  std::string host_name_;
  std::string service_name_;
};

#endif // PEER_XIXI_HA_CLIENT_H

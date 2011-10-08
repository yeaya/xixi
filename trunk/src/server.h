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

#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/thread/condition.hpp>
#include <string>
#include <map>
#include "io_service_pool.h"
#include "currtime.h"
#include "peer_ha_pdu.h"
#include "peer_xixi_ha.h"
#include "peer_xixi_ha_cli.h"

class Leader_Selector {
public:

private:
  std::map<Peer_XIXI_HA_CLI*, std::string> ha_clients_;
  std::list<Peer_XIXI_HA_CLI*> ha_present_list_;
  std::vector<string> ha_votes_;
};

class HA_Client_Status {
public:
  HA_Client_Status() {
    present_res_flag = false;
  }

  bool present_res_flag;
};

class Server {

public:
  Server(std::size_t pool_size, std::size_t thread_size);
  ~Server();

  void start();
  void stop();
  void run();

  boost::asio::ip::tcp::socket* create_socket();
  boost::asio::ip::tcp::resolver& get_resolver() { return resolver_; }

  std::string& get_server_id() {
    return server_id_;
  }

  xixi_ha_status get_status() {
    return ha_status_;
  }

  const char* get_status_string(xixi_ha_status status) {
    static const char* sss[] = {
      "LOOKING",
      "LEADING",
      "FOLLOWING",
      "Unkown"
    };
    if (status > 2) {
      status = 3;
    }
    return sss[status];
  }

  void remove_ha_client(Peer_XIXI_HA_CLI* client) {
    lock_.lock();
    ha_clients_.erase(client);
    lock_.unlock();
  }

  void process_ha_present_req_pdu(Peer_XIXI_HA* client, XIXI_HA_Present_Req_Pdu* req_pdu, XIXI_HA_Present_Res_Pdu* res_pdu);
  void process_ha_create_committee_req_pdu(Peer_XIXI_HA* client, XIXI_HA_Create_Committee_Req_Pdu* req_pdu, XIXI_HA_Create_Committee_Res_Pdu* res_pdu);
  void process_ha_announce_leader_req_pdu(Peer_XIXI_HA* client, XIXI_HA_Announce_Leader_Req_Pdu* req_pdu, XIXI_HA_Announce_Leader_Res_Pdu* res_pdu);
  void process_ha_keepalive_req_pdu(Peer_XIXI_HA* client, XIXI_HA_Keepalive_Req_Pdu* req_pdu, XIXI_HA_Keepalive_Res_Pdu* res_pdu);

  void process_ha_pdu(XIXI_Pdu* pdu);

  void pdu_process();

private:
  void process_ha_present_res_pdu(XIXI_HA_Present_Res_Pdu* pdu);
  void process_ha_create_committee_res_pdu(XIXI_HA_Create_Committee_Res_Pdu* pdu);
  void process_ha_announce_leader_res_pdu(XIXI_HA_Announce_Leader_Res_Pdu* pdu);
  void process_ha_keepalive_res_pdu(XIXI_HA_Keepalive_Res_Pdu* pdu);

  void handle_accept(boost::asio::ip::tcp::socket* socket, const boost::system::error_code& err);
  void handle_timer(const boost::system::error_code& err);
  void handle_ha_keepalive_timer(const boost::system::error_code& err);
  void handle_ha_present_timeout(const boost::system::error_code& err);
  void handle_vote_recover_timer(const boost::system::error_code& err);

  void send_present();
  void create_committee();
  void announce_leader();
  void send_keepalive();

  uint32_t check_leader_keepalive();
  void check_follow_keepalive();
  void clear_ha_client();
  void do_clear_ha_client();

private:
  std::string server_id_;
  mutex lock_;

  io_service_pool io_service_pool_;
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ip::tcp::resolver resolver_;
  boost::asio::deadline_timer timer_;
  boost::asio::deadline_timer ha_timer_;
  volatile bool stop_flag_;

  xixi_ha_status ha_status_;
  bool vote_flag_;
  uint32_t vote_recover_interval_ms_;
  boost::asio::deadline_timer vote_recover_timer_;
  std::vector<std::pair<string, string> > ha_servers;
  std::map<Peer_XIXI_HA_CLI*, HA_Client_Status> ha_clients_;

  uint32_t ha_present_res_count_;

  std::vector<string> ha_votes_;
  uint32_t follow_keepalive_timestamp_;

  shared_ptr<boost::thread> pdu_process_thread_;
  std::list<XIXI_Pdu*> pdu_list_;
  mutex pdu_list_lock_;
  boost::condition pdu_list_condition_;
};

extern Server* svr_;

#endif // SERVER_H

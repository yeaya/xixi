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

#include "server.h"
#include "settings.h"
#include "log.h"
#include "peer_xixi.h"
#include "peer_cache.h"
#include "cache.h"
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

#define PRESENT_TIMEOUT_MS 5000
#define KEEPALIVE_TIMEOUT_S 15
#define KEEPALIVE_INTERVAL_MS 5000

Server* svr_ = NULL;

class Connection_Help {
public:
  Connection_Help(boost::asio::ip::tcp::socket* socket) {
    socket_ = socket;
    read_data_size_ = 0;
  }

  ~Connection_Help() {
    if (socket_ != NULL) {
      delete socket_;
      socket_ = NULL;
    }
  }

  void start() {
    boost::system::error_code ec;

    boost::asio::ip::tcp::socket::non_blocking_io non_blocking_io(true);
    socket_->io_control(non_blocking_io, ec);
    if (ec) {
      LOG_ERROR("socket io_control non_blocking_io, error=" << ec);
      socket_->get_io_service().post(boost::bind(&Connection_Help::destroy, this));
      return;
    }

    static bool print_buffer_size = false;
    if (!print_buffer_size) {
      print_buffer_size = true;
      boost::asio::socket_base::receive_buffer_size option1;
      socket_->get_option(option1, ec);
      int size = option1.value();
      LOG_INFO("receive_buffer_size=" << size);

      boost::asio::socket_base::send_buffer_size option2;
      socket_->get_option(option2, ec);
      size = option2.value();
      LOG_INFO("send_buffer_size=" << size);
    }

    boost::asio::ip::tcp::no_delay no_delay(true);
    socket_->set_option(no_delay, ec);
    if (ec) {
      LOG_ERROR("socket set_option error=" << ec);
      socket_->get_io_service().post(boost::bind(&Connection_Help::destroy, this));
      return;
    }

    socket_->async_read_some(boost::asio::buffer(read_buf_ + read_data_size_, sizeof(read_buf_) - read_data_size_),
      make_custom_alloc_handler(read_allocator_,
        boost::bind(&Connection_Help::handle_first_read, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred)));
  }

  void handle_first_read(const boost::system::error_code& err, size_t length) {
    LOG_TRACE("handle_first_read length=" << length << " err=" << err.message() << " err_value=" << err.value());

    if (!err) {
      read_data_size_ += length;
      if (read_data_size_ >= 1) {
        boost::asio::io_service& io_service_ = socket_->get_io_service();
        start_peer(read_buf_, read_data_size_);
      //  if (socket_ == NULL) {
          io_service_.post(boost::bind(&Connection_Help::destroy, this));
      //  }
      } else {
        socket_->async_read_some(boost::asio::buffer(read_buf_ + read_data_size_, sizeof(read_buf_) - read_data_size_),
          make_custom_alloc_handler(read_allocator_,
            boost::bind(&Connection_Help::handle_first_read, this,
              boost::asio::placeholders::error,
              boost::asio::placeholders::bytes_transferred)));
        LOG_TRACE("handle_first_read async_read_some get_read_buf_size=" << (sizeof(read_buf_) - read_data_size_));
      }
    } else {
      LOG_INFO("handle_first_read destroy");
      socket_->get_io_service().post(boost::bind(&Connection_Help::destroy, this));
    }
  }

  void start_peer(uint8_t* data, uint32_t data_len) {
    if (data_len >= 1) {
/*      if (data[0] == BINARY_MAGIC_REQ) {
        Peer* peer = new Peer_MC_Binary();
        if (peer != NULL) {
          TcpConnection* conn = new TcpConnection(socket_);
          conn->start(read_buf_, read_data_size_, peer);
          socket_ = NULL;
        }
        return;
      } else */if (data[0] == XIXI_CATEGORY_COMMON || data[0] == XIXI_CATEGORY_CC) {
        Peer_XIXI* peer = new Peer_XIXI(socket_);
        peer->start(read_buf_, read_data_size_);
        socket_ = NULL;
        return;
      } else if (data[0] == XIXI_CATEGORY_CACHE) {
        Peer_Cache* peer = new Peer_Cache(socket_);
        peer->start(read_buf_, read_data_size_);
        socket_ = NULL;
        return;
      } else if (data[0] == XIXI_CATEGORY_HA) {
        Peer_XIXI_HA* peer_ha = new Peer_XIXI_HA(socket_);
        peer_ha->start(read_buf_, read_data_size_);
        socket_ = NULL;
        return;
      }
    }
/*    if (data_len >= 3) {
      if (::memcmp(data, "get", 3) == 0
          || ::memcmp(data, "get", 3) == 0
          || ::memcmp(data, "bget", 3) == 0
          || ::memcmp(data, "add", 3) == 0
          || ::memcmp(data, "set", 3) == 0
          || ::memcmp(data, "replace", 3) == 0
          || ::memcmp(data, "prepend", 3) == 0
          || ::memcmp(data, "append", 3) == 0
          || ::memcmp(data, "cas", 3) == 0
          || ::memcmp(data, "incr", 3) == 0
          || ::memcmp(data, "gets", 3) == 0
          || ::memcmp(data, "decr", 3) == 0
          || ::memcmp(data, "delete", 3) == 0
          || ::memcmp(data, "stats", 3) == 0
          || ::memcmp(data, "flush_all", 3) == 0
          || ::memcmp(data, "version", 3) == 0
          || ::memcmp(data, "quit", 3) == 0
          || ::memcmp(data, "verbosity", 3) == 0) {
        Peer* peer = new Peer_MC_Ascii();
        if (peer != NULL) {
          TcpConnection* conn = new TcpConnection(socket_);
          conn->start(read_buf_, read_data_size_, peer);
          socket_ = NULL;
        }
        return;
      }
    }*/
    
  }

  static void destroy(Connection_Help* conn) {
    delete conn;
  }

private:
  boost::asio::ip::tcp::socket* socket_;

  uint8_t  read_buf_[1024];
  uint32_t read_data_size_;
  Handler_Allocator<> read_allocator_;
};

Server::Server(std::size_t pool_size, std::size_t thread_size) :
    io_service_pool_(pool_size, thread_size),
    acceptor_(io_service_pool_.get_io_service()),
    resolver_(io_service_pool_.get_io_service()),
    timer_(io_service_pool_.get_io_service(), boost::posix_time::millisec(500)),
    ha_timer_(io_service_pool_.get_io_service(), boost::posix_time::millisec(50)),
    vote_recover_timer_(io_service_pool_.get_io_service(), boost::posix_time::millisec(50)) {
  boost::uuids::basic_random_generator<boost::mt19937> gen;
  boost::uuids::uuid id = gen();

  server_id_ = boost::uuids::to_string(id);
  LOG_INFO("UUID: " << id);
//  server_id_ = boost::lexical_cast<std::string>(server_id_);
//  LOG_INFO("UUID: " << id);
  ha_status_ = XIXI_HA_STATUS_LOOKING;

  stop_flag_ = false;
  curr_time_.set_current_time();

  ha_servers.push_back(pair<string, string>("127.0.0.1", "7788"));
  ha_servers.push_back(pair<string, string>("127.0.0.1", "7789"));
  ha_servers.push_back(pair<string, string>("127.0.0.1", "7790"));
  ha_present_res_count_ = 0;
  vote_flag_ = false;
  vote_recover_interval_ms_ = ha_servers.size() * PRESENT_TIMEOUT_MS;
  follow_keepalive_timestamp_ = curr_time_.get_current_time();
}

Server::~Server() {
}

void Server::start() {
  timer_.async_wait(boost::bind(&Server::handle_timer, this,
            boost::asio::placeholders::error));
//  ha_timer_.expires_at(ha_timer_.expires_at() + boost::posix_time::millisec(2000));
//  ha_timer_.async_wait(boost::bind(&Server::handle_ha_keepalive_timer, this,
//            boost::asio::placeholders::error));

  boost::asio::ip::address address = boost::asio::ip::address::from_string(settings_.inter);
  boost::asio::ip::tcp::endpoint endpoint(address, settings_.port);
  boost::asio::ip::tcp::endpoint endpoint1(address, settings_.port + 1);
  boost::asio::ip::tcp::endpoint endpoint2(address, settings_.port + 2);
//  boost::asio::ip::udp::endpoint udpendpoint(address, settings_.udpport);

  cache_mgr_.init(settings_.maxbytes, settings_.item_size_max, settings_.item_size_min, settings_.factor);

  boost::system::error_code err_code;

  acceptor_.open(endpoint.protocol());
//  acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(1));
  acceptor_.bind(endpoint, err_code);
  if (err_code) {
    LOG_INFO("bind error, on " << settings_.inter << ":" << settings_.port);
    acceptor_.bind(endpoint1, err_code);
    if (err_code) {
      LOG_INFO("bind error, on " << settings_.inter << ":" << (settings_.port + 1));
      acceptor_.bind(endpoint2, err_code);
      if (err_code) {
        LOG_INFO("bind error, on " << settings_.inter << ":" << (settings_.port + 2));
        return;
      } else {
        LOG_INFO("listen on " << settings_.inter << ":" << (settings_.port + 2));
      }
    } else {
      LOG_INFO("listen on " << settings_.inter << ":" << (settings_.port + 1));
    }
  } else {
    LOG_INFO("listen on " << settings_.inter << ":" << settings_.port);
  }
  acceptor_.listen();

  boost::asio::ip::tcp::socket* socket = new boost::asio::ip::tcp::socket(io_service_pool_.get_io_service());
  acceptor_.async_accept(*socket,
    boost::bind(&Server::handle_accept, this, socket,
    boost::asio::placeholders::error));

//  send_present();
  shared_ptr<boost::thread> spt(new boost::thread(boost::bind(&Server::pdu_process, this)));
  pdu_process_thread_ = spt;
}

void Server::stop() {
  LOG_INFO("Server::stop enter");
  stop_flag_ = true;
  acceptor_.close();
  resolver_.cancel();
  pdu_list_condition_.notify_one();
  pdu_process_thread_->join();
//  timer_.cancel();
//  ha_timer_.cancel();
//  vote_recover_timer_.cancel();
  io_service_pool_.stop();
  LOG_INFO("Server::stop leave");
}

void Server::run() {
  io_service_pool_.run();
}

boost::asio::ip::tcp::socket* Server::create_socket() {
  return new boost::asio::ip::tcp::socket(io_service_pool_.get_io_service());
}

void Server::handle_accept(boost::asio::ip::tcp::socket* socket, const boost::system::error_code& err) {
  LOG_DEBUG("handle_accept error=" << err.message());
  if (!err) {
    Connection_Help* conn = new Connection_Help(socket);
    conn->start();
    socket = new boost::asio::ip::tcp::socket(io_service_pool_.get_io_service());
    acceptor_.async_accept(*socket,
      boost::bind(&Server::handle_accept, this, socket,
      boost::asio::placeholders::error));
  } else {
    delete socket;
    if (!stop_flag_) {
      LOG_ERROR("handle accept error:" << err.message());
      socket = new boost::asio::ip::tcp::socket(io_service_pool_.get_io_service());
      acceptor_.async_accept(*socket,
        boost::bind(&Server::handle_accept, this, socket,
        boost::asio::placeholders::error));
    }
  }
}

void Server::handle_timer(const boost::system::error_code& err) {
  curr_time_.set_current_time();
  cache_mgr_.check_expired();
  cache_mgr_.print_stats();

  timer_.expires_at(timer_.expires_at() + boost::posix_time::millisec(500));
  timer_.async_wait(boost::bind(&Server::handle_timer, this,
            boost::asio::placeholders::error));
//  static int i = 0; 
//  printf("tt%d\n", i++);
}

void Server::handle_ha_keepalive_timer(const boost::system::error_code& err) {
  LOG_INFO("Server::handle_ha_keepalive_timer status=" << this->get_status_string(ha_status_) << "  err=" << err);
  if (!err) {
    switch (ha_status_) {
      case XIXI_HA_STATUS_LOOKING:
  //      send_present();
        return;
        break;

      case XIXI_HA_STATUS_LEADING: {
          uint32_t size = check_leader_keepalive();
          if (size >= ha_servers.size() / 2) {
            send_keepalive();
          } else {
            clear_ha_client();
            send_present();
          }
        }
        break;

      case XIXI_HA_STATUS_FOLLOWING:
        check_follow_keepalive();
        break;
    }

    ha_timer_.expires_from_now(boost::posix_time::millisec(KEEPALIVE_INTERVAL_MS));
    ha_timer_.async_wait(boost::bind(&Server::handle_ha_keepalive_timer, this,
              boost::asio::placeholders::error));
  }

//  static int i = 0; 
//  printf("tt%d\n", i++);
}

void Server::handle_ha_present_timeout(const boost::system::error_code& err) {
  LOG_INFO("handle_ha_present_timeout err=" << err);
  if (!err) {
    clear_ha_client();
    send_present();
//    ha_timer_.expires_at(ha_timer_.expires_at() + boost::posix_time::millisec(1000));
//    ha_timer_.async_wait(boost::bind(&Server::handle_ha_timer, this,
//              boost::asio::placeholders::error));
  }
}

void Server::handle_vote_recover_timer(const boost::system::error_code& err) {
  LOG_INFO("handle_vote_recover_timer err=" << err);
  if (!err) {
    this->vote_flag_ = false;
  }
}

void Server::send_present() {
  LOG_INFO("send_present");

  lock_.lock();
//  if (!ha_clients_.empty()) {
//    return;
//  }

  ha_status_ = XIXI_HA_STATUS_LOOKING;
  ha_present_res_count_ = 0;
//  vote_flag_ = false;
  ha_votes_.clear();

  for (size_t i = 0; i < ha_servers.size(); ++i) {
    Peer_XIXI_HA_CLI* peer1 = new Peer_XIXI_HA_CLI(ha_servers[i].first, ha_servers[i].second);
    ha_clients_.insert(make_pair<Peer_XIXI_HA_CLI*, HA_Client_Status>(peer1, HA_Client_Status()));
  }

  std::map<Peer_XIXI_HA_CLI*, HA_Client_Status>::iterator it = ha_clients_.begin();
  while (it != ha_clients_.end()) {
    it->first->connect();
    ++it;
  }

  ha_timer_.expires_from_now(boost::posix_time::millisec(PRESENT_TIMEOUT_MS));
  ha_timer_.async_wait(boost::bind(&Server::handle_ha_present_timeout, this,
            boost::asio::placeholders::error));
  lock_.unlock();
}

void Server::process_ha_present_req_pdu(Peer_XIXI_HA* client, XIXI_HA_Present_Req_Pdu* req_pdu, XIXI_HA_Present_Res_Pdu* res_pdu) {
  LOG_INFO("Server::process_ha_present_req_pdu pdu.status=" << get_status_string(req_pdu->status) << " pdu.id=" << req_pdu->server_id);
  lock_.lock();
  res_pdu->status = get_status();
  res_pdu->server_id = get_server_id();
  lock_.unlock();
}

void Server::process_ha_create_committee_req_pdu(Peer_XIXI_HA* client, XIXI_HA_Create_Committee_Req_Pdu* pdu, XIXI_HA_Create_Committee_Res_Pdu* res_pdu) {
  LOG_INFO("Server::process_ha_create_committee_req_pdu pdu.status=" << get_status_string(pdu->status) << " pdu.id=" << pdu->server_id);
  lock_.lock();
  res_pdu->server_id = server_id_;
  if (!vote_flag_) {
    res_pdu->votes.push_back(server_id_);
    vote_flag_ = true;
    vote_recover_timer_.expires_from_now(boost::posix_time::millisec(vote_recover_interval_ms_));
    vote_recover_timer_.async_wait(boost::bind(&Server::handle_vote_recover_timer, this,
            boost::asio::placeholders::error));
  }
  if (pdu->server_id > this->server_id_) {
    for (size_t i = 0; i < ha_votes_.size(); i++) {
      res_pdu->votes.push_back(ha_votes_[i]);
    }
    ha_votes_.clear();
  }
  lock_.unlock();
}

void Server::process_ha_announce_leader_req_pdu(Peer_XIXI_HA* client, XIXI_HA_Announce_Leader_Req_Pdu* pdu, XIXI_HA_Announce_Leader_Res_Pdu* res_pdu) {
  LOG_INFO("Server::process_ha_announce_leader_req_pdu pdu.status=" << get_status_string(pdu->status) << " pdu.id=" << pdu->server_id);

  lock_.lock();
  res_pdu->status = this->get_status();
  res_pdu->server_id = server_id_;

  if (pdu->server_id == server_id_) {
    this->ha_status_ = XIXI_HA_STATUS_LEADING;
    std::map<Peer_XIXI_HA_CLI*, HA_Client_Status>::iterator it = ha_clients_.begin();
    while (it != ha_clients_.end()) {
      if (it->first->get_server_id() == server_id_) {
        LOG_INFO("Server::process_ha_announce_leader_req_pdu close the self client " << server_id_);
        it->first->close();
        ha_clients_.erase(it);
        break;
      }
      ++it;
    }
  //  this->ha_clients_.erase(client);
  //  client->close();
  } else {
    follow_keepalive_timestamp_ = curr_time_.get_current_time();
    this->ha_status_ = XIXI_HA_STATUS_FOLLOWING;
  }
  lock_.unlock();

  if (pdu->server_id == server_id_) {
    this->send_keepalive();
  } else {
    this->clear_ha_client();
  }

  ha_timer_.cancel();
  ha_timer_.expires_from_now(boost::posix_time::millisec(KEEPALIVE_INTERVAL_MS));
  ha_timer_.async_wait(boost::bind(&Server::handle_ha_keepalive_timer, this,
            boost::asio::placeholders::error));
}

void Server::process_ha_keepalive_req_pdu(Peer_XIXI_HA* client, XIXI_HA_Keepalive_Req_Pdu* pdu, XIXI_HA_Keepalive_Res_Pdu* res_pdu) {
  LOG_INFO("Server::process_ha_keepalive_req_pdu pdu.status=" << get_status_string(pdu->status) << " pdu.id=" << pdu->server_id);
  lock_.lock();
  follow_keepalive_timestamp_ = curr_time_.get_current_time();
  if (this->ha_status_ == XIXI_HA_STATUS_LOOKING) {
    ha_status_ = XIXI_HA_STATUS_FOLLOWING;
    do_clear_ha_client();
//    ha_timer_.cancel();
    ha_timer_.expires_from_now(boost::posix_time::millisec(KEEPALIVE_INTERVAL_MS));
    ha_timer_.async_wait(boost::bind(&Server::handle_ha_keepalive_timer, this,
              boost::asio::placeholders::error));
  }
  res_pdu->status = this->get_status();
  res_pdu->server_id = this->get_server_id();
  lock_.unlock();
}

void Server::create_committee() {
  LOG_INFO("Server::create_committee server_id=" << server_id_);
  lock_.lock();
  std::map<Peer_XIXI_HA_CLI*, HA_Client_Status>::iterator it = ha_clients_.begin();
  while (it != ha_clients_.end()) {
    if (it->first->is_present_responsed()) {
      it->first->create_committee();
    }
    ++it;
  }
  lock_.unlock();
}

void Server::announce_leader() {
  LOG_INFO("Server::announce_leader server_id=" << server_id_);
  lock_.lock();
  std::map<Peer_XIXI_HA_CLI*, HA_Client_Status>::iterator it = ha_clients_.begin();
  while (it != ha_clients_.end()) {
    it->first->announce_leader();
    ++it;
  }
  lock_.unlock();
}

void Server::send_keepalive() {
  LOG_INFO("Server::send_keepalive status=" << get_status_string(get_status()));
  lock_.lock();
  std::map<Peer_XIXI_HA_CLI*, HA_Client_Status>::iterator it = ha_clients_.begin();
  while (it != ha_clients_.end()) {
    it->first->keepalive();
    ++it;
  }
  lock_.unlock();
}

uint32_t Server::check_leader_keepalive() {
  LOG_INFO("Server::check_leader_keepalive status=" << get_status_string(get_status()));
  uint32_t live_count = 0;
  lock_.lock();
  std::map<Peer_XIXI_HA_CLI*, HA_Client_Status>::iterator it = ha_clients_.begin();
  Peer_XIXI_HA_CLI* client = NULL;
  while (it != ha_clients_.end()) {
    client = it->first;
    if (client->need_reconnect()) {
      client->connect();
    }
    if (client->is_keepalive_timeout(KEEPALIVE_TIMEOUT_S)) {
      LOG_INFO("Server::check_leader_keepalive timeout");
      client->connect();
//      client->close();
//      ha_clients_.erase(it++);
      ++it;
    } else {
      ++it;
      ++live_count;
    }
  }
//  uint32_t size = ha_clients_.size();
  lock_.unlock();
  LOG_INFO("Server::check_leader_keepalive status=" << get_status_string(get_status()) << " live_count=" << live_count);
  return live_count;
}

void Server::check_follow_keepalive() {
  LOG_INFO("Server::check_follow_keepalive status=" << get_status_string(get_status()));
  if (curr_time_.is_timeout(follow_keepalive_timestamp_, KEEPALIVE_TIMEOUT_S)) {
    LOG_INFO("Server::check_follow_keepalive timeout");
    clear_ha_client();
    send_present();
//    ha_status_ = XIXI_HA_STATUS_LOOKING;
  }
}

void Server::do_clear_ha_client() {
  LOG_INFO("Server::do_clear_ha_client status=" << get_status_string(get_status()));
  std::map<Peer_XIXI_HA_CLI*, HA_Client_Status>::iterator it = ha_clients_.begin();
  while (it != ha_clients_.end()) {
    it->first->close();
  //  delete it->first;
    ++it;
  }
  ha_clients_.clear();

//  ha_present_res_count_ = 0;
//  ha_votes_.clear();
}

void Server::clear_ha_client() {
  LOG_INFO("Server::clear_ha_client status=" << get_status_string(get_status()));
  lock_.lock();
  do_clear_ha_client();
  lock_.unlock();
}

void Server::process_ha_present_res_pdu(XIXI_HA_Present_Res_Pdu* pdu) {
  LOG_INFO("Server::process_ha_present_res_pdu pdu.status=" << get_status_string(pdu->status) << " pdu.id=" << pdu->server_id);
  lock_.lock();
  switch (ha_status_) {
    case XIXI_HA_STATUS_LOOKING:
      if (pdu->status == XIXI_HA_STATUS_LOOKING) {
        ha_present_res_count_++;
        if (ha_present_res_count_ > ha_servers.size() / 2) {
          LOG_INFO("Server::process_ha_present_res_pdu ha_present_res_count_=" <<ha_present_res_count_);
          io_service_pool_.get_io_service().post(boost::bind(&Server::create_committee, this));
        }
      }
      break;

    case XIXI_HA_STATUS_LEADING:
      break;

    case XIXI_HA_STATUS_FOLLOWING:
      break;
  }
  lock_.unlock();
  delete pdu;
}

void Server::process_ha_create_committee_res_pdu(XIXI_HA_Create_Committee_Res_Pdu* pdu) {
  LOG_INFO("Server::process_ha_create_committee_res_pdu pdu.status=" << get_status_string(pdu->status) << " pdu.id=" << pdu->server_id);
  lock_.lock();
  for (size_t i = 0; i < pdu->votes.size(); i++) {
    LOG_INFO("Server::process_ha_create_committee_res_pdu get vote=" << pdu->votes[i]);
    ha_votes_.push_back(pdu->votes[i]);
    if (ha_votes_.size() > ha_servers.size() / 2) {
      io_service_pool_.get_io_service().post(boost::bind(&Server::announce_leader, this));
    }
  }
  lock_.unlock();
  delete pdu;
}

void Server::process_ha_announce_leader_res_pdu(XIXI_HA_Announce_Leader_Res_Pdu* pdu) {
  LOG_INFO("Server::process_ha_announce_leader_res_pdu pdu.status=" << get_status_string(pdu->status) << " pdu.id=" << pdu->server_id);
//  lock_.lock();

//  lock_.unlock();
  delete pdu;
}

void Server::process_ha_keepalive_res_pdu(XIXI_HA_Keepalive_Res_Pdu* pdu) {
  LOG_INFO("Server::process_ha_keepalive_res_pdu pdu.status=" << get_status_string(pdu->status) << " pdu.id=" << pdu->server_id);
//  io_service_pool_.get_io_service().post(boost::bind(&Server::update_keepalive, this, pdu));
//  lock_.lock();

//  lock_.unlock();
  delete pdu;
}

void Server::process_ha_pdu(XIXI_Pdu* pdu) {
  LOG_INFO("Server::process_ha_pdu enter");
  pdu_list_lock_.lock();
  pdu_list_.push_back(pdu);
  pdu_list_condition_.notify_one();
  pdu_list_lock_.unlock();
  LOG_INFO("Server::process_ha_pdu leave");
}

void Server::pdu_process() {
  LOG_INFO("Server::pdu_process enter");
  std::list<XIXI_Pdu*> list;
  XIXI_Pdu* pdu = NULL;
  while (!stop_flag_) {
    pdu_list_lock_.lock();
    if (pdu_list_.empty()) {
      pdu_list_condition_.wait(pdu_list_lock_);
    }
    pdu_list_.swap(list);
    pdu_list_lock_.unlock();
    while (!list.empty()) {
      pdu = list.front();
      list.pop_front();
      switch (pdu->choice) {
        case XIXI_CHOICE_HA_PRESENT_RES:
          process_ha_present_res_pdu((XIXI_HA_Present_Res_Pdu*)pdu);
          break;
        case XIXI_CHOICE_HA_CREATE_COMMITTEE_RES:
          process_ha_create_committee_res_pdu((XIXI_HA_Create_Committee_Res_Pdu*)pdu);
          break;
        case XIXI_CHOICE_HA_ANNOUNCE_LEADER_RES:
          process_ha_announce_leader_res_pdu((XIXI_HA_Announce_Leader_Res_Pdu*)pdu);
          break;
        case XIXI_CHOICE_HA_KEEPALIVE_RES:
          process_ha_keepalive_res_pdu((XIXI_HA_Keepalive_Res_Pdu*)pdu);
          break;
        default:
          delete pdu;
          break;
      }
    }
  }
  LOG_INFO("Server::pdu_process leave");
}

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
#include "peer_cache.h"
#include "cache.h"
#include "currtime.h"
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

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
      if (data[0] == XIXI_CATEGORY_CACHE) {
        Peer_Cache* peer = new Peer_Cache(socket_);
        peer->start(read_buf_, read_data_size_);
        socket_ = NULL;
        return;
      }
	}
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
    timer_(io_service_pool_.get_io_service(), boost::posix_time::millisec(500)) {
  boost::uuids::basic_random_generator<boost::mt19937> gen;
  boost::uuids::uuid id = gen();

  server_id_ = boost::uuids::to_string(id);
//LOG_INFO("UUID: " << id);

  stop_flag_ = false;
  curr_time_.set_current_time();
}

Server::~Server() {
}

void Server::start() {
  timer_.async_wait(boost::bind(&Server::handle_timer, this,
            boost::asio::placeholders::error));

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
}

void Server::stop() {
  LOG_INFO("Server::stop enter");
  stop_flag_ = true;
  acceptor_.close();
  resolver_.cancel();
//  timer_.cancel();
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
}

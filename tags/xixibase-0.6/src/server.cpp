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
#include "peer_http.h"
#include "cache.h"
#include "currtime.h"
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

Server* svr_ = NULL;

class Connection_Help {
public:
	Connection_Help(boost::asio::io_service& io_service, boost::asio::ip::tcp::acceptor* acceptor) {
		socket_ = new boost::asio::ip::tcp::socket(io_service);
		acceptor_ = acceptor;
		read_data_size_ = 0;
	}

	~Connection_Help() {
		if (socket_ != NULL) {
			socket_->close();
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

		uint32_t length = (uint32_t)socket_->read_some(boost::asio::buffer(read_buf_, (std::size_t)sizeof(read_buf_) - 1), ec);
		if (length > 0) {
			process(length);
		} else {
			socket_->async_read_some(boost::asio::buffer(read_buf_ + read_data_size_, sizeof(read_buf_) - 1 - read_data_size_),
				make_custom_alloc_handler(read_allocator_,
				boost::bind(&Connection_Help::handle_first_read, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred)));
		}
	}

	void process(size_t length) {
		read_data_size_ += (uint32_t)length;
		if (read_data_size_ >= 4) {
//			boost::asio::io_service& io_service_ = socket_->get_io_service();
			start_peer(read_buf_, read_data_size_);
//			io_service_.post(boost::bind(&Connection_Help::destroy, this));
			delete this;
		} else {
			socket_->async_read_some(boost::asio::buffer(read_buf_ + read_data_size_, sizeof(read_buf_) - 1 - read_data_size_),
				make_custom_alloc_handler(read_allocator_,
				boost::bind(&Connection_Help::handle_first_read, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred)));
			LOG_TRACE("handle_first_read async_read_some get_read_buf_size=" << (sizeof(read_buf_) - 1 - read_data_size_));
		}
	}

	void handle_first_read(const boost::system::error_code& err, size_t length) {
		LOG_TRACE("handle_first_read length=" << length << " err=" << err.message() << " err_value=" << err.value());
		if (!err) {
			process(length);
		} else {
			LOG_DEBUG("handle_first_read destroy");
		//	socket_->get_io_service().post(boost::bind(&Connection_Help::destroy, this));
			delete this;
		}
	}

	void start_peer(uint8_t* data, uint32_t data_len) {
		if (data_len >= 4) {
			if (data[0] == XIXI_CATEGORY_CACHE) {
				Peer_Cache* peer = new Peer_Cache(socket_);
				peer->start(read_buf_, read_data_size_);
				socket_ = NULL;
				return;
			}

			if (IS_METHOD(data, 'G', 'E', 'T', ' ') || IS_METHOD(data, 'P', 'O', 'S', 'T') || IS_METHOD(data, 'H', 'E', 'A', 'D')) {
				Peer_Http* peer = new Peer_Http(socket_);
				data[data_len] = '\0';
				peer->start(read_buf_, read_data_size_);
				socket_ = NULL;
				return;
			}
		}
	}

	boost::asio::ip::tcp::socket* get_socket() {
		return socket_;
	}

	boost::asio::ip::tcp::acceptor* get_acceptor() {
		return acceptor_;
	}

	static void destroy(Connection_Help* conn) {
		delete conn;
	}

protected:
	boost::asio::ip::tcp::socket* socket_;
	boost::asio::ip::tcp::acceptor* acceptor_;
	uint8_t  read_buf_[1025];
	uint32_t read_data_size_;
	Handler_Allocator<> read_allocator_;
};

class Connection_SSL_Help {
public:
	Connection_SSL_Help(boost::asio::io_service& io_service, boost::asio::ip::tcp::acceptor* acceptor, boost::asio::ssl::context& context) {
		socket_ = new boost::asio::ssl::stream<boost::asio::ip::tcp::socket>(io_service, context);
		acceptor_ = acceptor;
		read_data_size_ = 0;
	}

	~Connection_SSL_Help() {
		if (socket_ != NULL) {
			boost::system::error_code ec;
			socket_->shutdown(ec);
			delete socket_;
			socket_ = NULL;
		}
	}

	void start() {
		boost::system::error_code ec;

		boost::asio::ip::tcp::socket::non_blocking_io non_blocking_io(true);
		socket_->lowest_layer().io_control(non_blocking_io, ec);
		if (ec) {
			LOG_ERROR("socket io_control non_blocking_io, error=" << ec);
		//	socket_->get_io_service().post(boost::bind(&Connection_SSL_Help::destroy, this));
			delete this;
			return;
		}

		static bool print_buffer_size = false;
		if (!print_buffer_size) {
			print_buffer_size = true;
			boost::asio::socket_base::receive_buffer_size option1;
			socket_->lowest_layer().get_option(option1, ec);
			int size = option1.value();
			LOG_INFO("receive_buffer_size=" << size);

			boost::asio::socket_base::send_buffer_size option2;
			socket_->lowest_layer().get_option(option2, ec);
			size = option2.value();
			LOG_INFO("send_buffer_size=" << size);
		}

		boost::asio::ip::tcp::no_delay no_delay(true);
		socket_->lowest_layer().set_option(no_delay, ec);
		if (ec) {
			LOG_ERROR("socket set_option error=" << ec);
	//		socket_->get_io_service().post(boost::bind(&Connection_SSL_Help::destroy, this));
			delete this;
		} else {
			socket_->async_handshake(boost::asio::ssl::stream_base::server,
				boost::bind(&Connection_SSL_Help::handle_handshake, this,
					boost::asio::placeholders::error));
		}
	}

	void handle_handshake(const boost::system::error_code& err) {
		if (!err) {
			boost::system::error_code ec;
			uint32_t length = (uint32_t)socket_->read_some(boost::asio::buffer(read_buf_, (std::size_t)sizeof(read_buf_) - 1), ec);
			if (length > 0) {
				process(length);
			} else {
				socket_->async_read_some(boost::asio::buffer(read_buf_, sizeof(read_buf_) - 1),
					boost::bind(&Connection_SSL_Help::handle_first_read, this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
			}
		} else {
			LOG_ERROR("socket set_option error=" << err.message() << " err_value=" << err.value());
			delete this;
		}
	}

	void process(size_t length) {
		read_data_size_ += (uint32_t)length;
		if (read_data_size_ >= 4) {
//			boost::asio::io_service& io_service_ = socket_->get_io_service();
			start_peer(read_buf_, read_data_size_);
//			io_service_.post(boost::bind(&Connection_SSL_Help::destroy, this));
			delete this;
		} else {
			socket_->async_read_some(boost::asio::buffer(read_buf_ + read_data_size_, sizeof(read_buf_) - 1 - read_data_size_),
				make_custom_alloc_handler(read_allocator_,
				boost::bind(&Connection_SSL_Help::handle_first_read, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred)));
			LOG_TRACE("handle_first_read async_read_some get_read_buf_size=" << (sizeof(read_buf_) - 1 - read_data_size_));
		}
	}

	void handle_first_read(const boost::system::error_code& err, size_t length) {
		LOG_TRACE("handle_first_read length=" << length << " err=" << err.message() << " err_value=" << err.value());
		if (!err) {
			process(length);
		} else {
			LOG_INFO("handle_first_read destroy length=" << length << " err=" << err.message() << " err_value=" << err.value());
		//	socket_->get_io_service().post(boost::bind(&Connection_SSL_Help::destroy, this));
			delete this;
		}
	}

	void start_peer(uint8_t* data, uint32_t data_len) {
		if (data_len >= 4) {
			if (data[0] == XIXI_CATEGORY_CACHE) {
				Peer_Cache* peer = new Peer_Cache(socket_);
				peer->start(read_buf_, read_data_size_);
				socket_ = NULL;
				return;
			}

			if (IS_METHOD(data, 'G', 'E', 'T', ' ') || IS_METHOD(data, 'P', 'O', 'S', 'T') || IS_METHOD(data, 'H', 'E', 'A', 'D')) {
				Peer_Http* peer = new Peer_Http(socket_);
				data[data_len] = '\0';
				peer->start(read_buf_, read_data_size_);
				socket_ = NULL;
				return;
			}
		}
	}

	boost::asio::ssl::stream<boost::asio::ip::tcp::socket>* get_socket() {
		return socket_;
	}

	boost::asio::ip::tcp::acceptor* get_acceptor() {
		return acceptor_;
	}

//	static void destroy(Connection_SSL_Help* conn) {
//		delete conn;
//	}

protected:
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket>* socket_;
	boost::asio::ip::tcp::acceptor* acceptor_;
	uint8_t  read_buf_[1025];
	uint32_t read_data_size_;
	Handler_Allocator<> read_allocator_;
};

Server::Server(std::size_t pool_size, std::size_t thread_size) :
io_service_pool_(pool_size, thread_size),
resolver_(io_service_pool_.get_io_service()),
timer_(io_service_pool_.get_io_service(), boost::posix_time::millisec(500)),
context_(boost::asio::ssl::context::sslv23_server) {
//	boost::uuids::basic_random_generator<boost::mt19937> gen;
//	boost::uuids::uuid id = gen();

//	server_id_ = boost::uuids::to_string(id);
//	LOG_INFO("UUID: " << id);

	stop_flag_ = false;
	curr_time_.set_current_time();
}

Server::~Server() {
}

bool Server::start() {
	for (size_t i = 0; i < settings_.connectors.size(); i++) {
		Connector* c = settings_.connectors[i].get();
		if (!listen(c->address, c->port, c->reuse_address, c->ssl)) {
			return false;
		}
	}

//	context_.set_password_callback(boost::bind(&Server::get_password, this), err_code);
//	if (err_code) {
//		LOG_FATAL("failed on set_password_callback, error:" << err_code.message());
//		return false;
//	}
	boost::system::error_code err_code;

	context_.use_certificate_chain_file(settings_.home_dir + "conf/cacert.pem", err_code);
	if (err_code) {
		LOG_FATAL("failed on use_certificate_chain_file, error:" << err_code.message());
		return false;
	}
	context_.use_private_key_file(settings_.home_dir + "conf/privkey.pem", boost::asio::ssl::context::pem, err_code);
	if (err_code) {
		LOG_FATAL("failed on use_private_key_file, error:" << err_code.message());
		return false;
	}

	cache_mgr_.init(settings_.max_bytes, settings_.item_size_max, settings_.item_size_min, settings_.factor);

	timer_.async_wait(boost::bind(&Server::handle_timer, this,
		boost::asio::placeholders::error));
	return true;
}

void Server::stop() {
	LOG_INFO("Server::stop enter");
	stop_flag_ = true;

	for (size_t i = 0; i < acceptors_.size(); i++) {
		acceptors_[i]->close();
		delete acceptors_[i];
	}
	acceptors_.clear();

	for (size_t i = 0; i < acceptors_ssl_.size(); i++) {
		acceptors_ssl_[i]->close();
		delete acceptors_ssl_[i];
	}
	acceptors_ssl_.clear();

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

std::string Server::get_password() const {
	return "test";
}

bool Server::listen(const string& address_s, uint32_t port, bool reuse_address, bool ssl) {
	boost::system::error_code err_code;

	boost::asio::ip::address address = boost::asio::ip::address::from_string(address_s, err_code);
	if (err_code) {
		LOG_FATAL("failed on parse address " << address << ", error:" << err_code.message());
		return false;
	}
	boost::asio::ip::tcp::endpoint endpoint(address, port);
	boost::asio::ip::tcp::acceptor* acceptor = new boost::asio::ip::tcp::acceptor(io_service_pool_.get_io_service());
	
	acceptor->open(endpoint.protocol(), err_code);
	if (err_code) {
		LOG_FATAL("acceptor open error, on " << address << ":" << port << " " << err_code.message());
		return false;
	}
	if (reuse_address) {
		acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(1), err_code);
		if (err_code) {
			LOG_FATAL("acceptor set_option error, on " << address << ":" << port << " " << err_code.message());
			return false;
		}
	}
	acceptor->bind(endpoint, err_code);
	if (err_code) {
		LOG_FATAL("acceptor bind error, on " << address << ":" << port << " " << err_code.message());
		return false;
	} else {
		acceptor->listen(boost::asio::socket_base::max_connections, err_code);

		if (ssl) {
			LOG_INFO("Listen on " << address << ":" << port << "(SSL)");
			acceptors_ssl_.push_back(acceptor);
			start_accept_ssl(acceptor);
		} else {
			LOG_INFO("Listen on " << address << ":" << port);
			acceptors_.push_back(acceptor);
			start_accept(acceptor);
		}
	}

	return true;
}

void Server::start_accept(boost::asio::ip::tcp::acceptor* acceptor) {
	Connection_Help* help = new Connection_Help(io_service_pool_.get_io_service(), acceptor);
	acceptor->async_accept(*help->get_socket(),
		boost::bind(&Server::handle_accept, this, help,
			boost::asio::placeholders::error));
}

void Server::handle_accept(Connection_Help* help, const boost::system::error_code& err) {
	LOG_DEBUG("handle_accept error=" << err.message());
	if (!err) {
		start_accept(help->get_acceptor());
		help->start();
	} else {
		if (!stop_flag_) {
			LOG_ERROR("handle accept error:" << err.message());

			start_accept(help->get_acceptor());
		}
		delete help;
	}
}

void Server::start_accept_ssl(boost::asio::ip::tcp::acceptor* acceptor) {
	Connection_SSL_Help* help = new Connection_SSL_Help(io_service_pool_.get_io_service(), acceptor, context_);
	acceptor->async_accept(help->get_socket()->lowest_layer(),
		boost::bind(&Server::handle_accept_ssl, this, help,
			boost::asio::placeholders::error));
}

void Server::handle_accept_ssl(Connection_SSL_Help* help, const boost::system::error_code& err) {
	LOG_DEBUG("handle_accept_ssl error=" << err.message());
	if (!err) {
		start_accept_ssl(help->get_acceptor());
		help->start();
	} else {
		if (!stop_flag_) {
			LOG_ERROR("handle accept error:" << err.message());

			start_accept_ssl(help->get_acceptor());
		}
		delete help;
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

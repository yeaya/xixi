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

#include "defines.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/thread/condition.hpp>
#include "io_service_pool.h"

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

private:
	void handle_accept(boost::asio::ip::tcp::socket* socket, const boost::system::error_code& err);
	void handle_timer(const boost::system::error_code& err);

private:
	std::string server_id_;
	mutex lock_;

	io_service_pool io_service_pool_;
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::ip::tcp::resolver resolver_;
	boost::asio::deadline_timer timer_;
	volatile bool stop_flag_;
};

extern Server* svr_;

#endif // SERVER_H

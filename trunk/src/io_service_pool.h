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

#ifndef IO_SERVICE_POOL_H
#define IO_SERVICE_POOL_H

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include "defines.h"

class io_service_pool : private boost::noncopyable {
public:
	explicit io_service_pool(std::size_t pool_size, std::size_t thread_size);
	~io_service_pool();

	void run();

	void stop();

	boost::asio::io_service& get_io_service();

	size_t get_thread_size() { return thread_size_; }

private:
	vector<boost::asio::io_service*> io_services_;

	vector<boost::asio::io_service::work*> work_;

	size_t next_io_service_;

	size_t thread_size_;
};

#endif // IO_SERVICE_POOL_H

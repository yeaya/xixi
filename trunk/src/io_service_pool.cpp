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

#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include "io_service_pool.h"
#include "log.h"

io_service_pool::io_service_pool(std::size_t pool_size, std::size_t thread_size) :
    next_io_service_(0) {
  if (pool_size <= 0) {
    pool_size = 1; // use default 1
  }
  thread_size_ = thread_size;
  if (thread_size_ <= 0) {
    thread_size_ = 1; // use default 1
  }

  for (std::size_t i = 0; i < pool_size; ++i)  {
    boost::asio::io_service* io_service = new boost::asio::io_service();
    boost::asio::io_service::work* work = new boost::asio::io_service::work(*io_service);
    io_services_.push_back(io_service);
    work_.push_back(work);
  }
}

io_service_pool::~io_service_pool() {
  for (std::size_t i = 0; i < work_.size(); ++i)  {
    delete work_[i];
  }
  work_.clear();

  for (std::size_t i = 0; i < io_services_.size(); ++i)  {
    delete io_services_[i];
  }
  io_services_.clear();
}

void io_service_pool::run() {
  std::vector<boost::shared_ptr<boost::thread> > threads;
  
  for (std::size_t t = 0; t < thread_size_; ++t) {
    std::size_t index = t % io_services_.size();
    boost::shared_ptr<boost::thread> thread(new boost::thread(
      boost::bind(&boost::asio::io_service::run, io_services_[index])));
    threads.push_back(thread);
  }

  for (std::size_t i = 0; i < threads.size(); ++i) {
    threads[i]->join();
    LOG_INFO("io_service_pool::run thread" << i << " stop");
  }
}

void io_service_pool::stop() {
  for (std::size_t i = 0; i < io_services_.size(); ++i) {
    io_services_[i]->stop();
  }
}

boost::asio::io_service& io_service_pool::get_io_service() {
  boost::asio::io_service& io_service = *io_services_[next_io_service_];
  next_io_service_ = (next_io_service_ + 1) % io_services_.size();

  return io_service;
}

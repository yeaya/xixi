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

#ifndef XIXI_BUFFER_HPP
#define XIXI_BUFFER_HPP

//#include <boost/cstdint.hpp>
#include <boost/asio/buffer.hpp>
//using namespace boost;

namespace xixi {
  typedef boost::asio::const_buffer Const_Buffer;

} // namespace xixi

#endif // XIXI_BUFFER_HPP

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

#include <boost/serialization/pfto.hpp>

#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/tokenizer.hpp>
#include "auth.h"

using namespace std; 
using namespace boost::archive::iterators; 

typedef base64_from_binary<transform_width<char*, 6, 8> > base64_t; 
//typedef transform_width<binary_from_base64<string::const_iterator>, 8, 6> binary_t; 
typedef transform_width<binary_from_base64<char*>, 8, 6> binary_t; 

Auth auth_;

Auth::Auth() {
  init();
}

void Auth::init() {

}

void Auth::login(uint8_t* base64, uint32_t base64_length, std::string& out) {
  string dec(binary_t(base64), binary_t(base64 + base64_length));

  string method;

  tokenizer<> tok(dec);
  std::vector<string> para;
  for(tokenizer<>::iterator it = tok.begin(); it != tok.end(); ++it){
    para.push_back(*it);
  }
  if (para.size() > 1) {
    if (para[0] == "PLAIN") {
      if (para.size() >=3) {
        login_plain(para[1], para[2], out);
      } else {
        out = "INVALID PARAMETER";
      }
    }
  } else {
    out = "INVALID PARAMETER";
  }
}

void Auth::login_plain(const string& user_name, const string& password, string& out) {
  string result = "SUCCESS";
  const char* str = result.c_str();
  string enc(base64_t(str), base64_t(str + result.size()));
  out = result;
}

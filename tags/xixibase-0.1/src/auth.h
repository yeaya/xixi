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

#ifndef AUTH__H
#define AUTH__H

#include "defines.h"
#include "xixibase.h"

class Auth {
public:
	Auth();
	void init();

	void login(uint8_t* base64, uint32_t base64_length, std::string& out);

protected:
	void login_plain(const string& user_name, const string& password, string& out);
};

extern Auth auth_;

#endif // AUTH__H


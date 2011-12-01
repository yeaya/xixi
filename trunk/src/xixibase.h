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

#ifndef XIXIBASE_H
#define XIXIBASE_H

#include "defines.h"

#ifndef VERSION
#define VERSION "Xixibase/0.5"
#endif

#ifndef HTTP_SERVER
#define HTTP_SERVER "Xixibase/0.5"
#endif

typedef uint16_t xixi_reason;
const xixi_reason XIXI_REASON_SUCCESS = 0;
const xixi_reason XIXI_REASON_NOT_FOUND = 1;
const xixi_reason XIXI_REASON_EXISTS = 2;
const xixi_reason XIXI_REASON_TOO_LARGE = 3;
const xixi_reason XIXI_REASON_INVALID_PARAMETER = 4;
const xixi_reason XIXI_REASON_INVALID_OPERATION = 5;
const xixi_reason XIXI_REASON_MISMATCH = 6;
const xixi_reason XIXI_REASON_AUTH_ERROR = 7;
const xixi_reason XIXI_REASON_UNKNOWN_COMMAND = 8;
const xixi_reason XIXI_REASON_OUT_OF_MEMORY = 9;
//const xixi_reason XIXI_REASON_WAIT_FOR_ME = 10;
//const xixi_reason XIXI_REASON_PLEASE_TRY_AGAIN = 11;
const xixi_reason XIXI_REASON_WATCH_NOT_FOUND = 12;
const xixi_reason XIXI_REASON_IO_ERROR = 13;

const xixi_reason XIXI_REASON_MOVED_PERMANENTLY = 20;

#endif // XIXIBASE_H

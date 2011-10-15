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

#include "currtime.h"
#include "time.h"

Current_Time curr_time_;

Current_Time::Current_Time()
: current_time_(8), last_time_(0) {
	start_time_ = last_time_ = (uint32_t)time(NULL);
}

void Current_Time::set_current_time() {
	uint32_t tt = (uint32_t)time(NULL);
	if (tt > last_time_) {
		uint32_t d = tt - last_time_;
		if (d > 10) {
			d = 10;
		}
		current_time_ += d;
	}
	last_time_ = tt;
}

uint32_t Current_Time::get_start_time() {
	return start_time_;
}

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

#ifndef CURRENT_TIME_H
#define CURRENT_TIME_H

#include "defines.h"

class Current_Time
{
public:
	Current_Time();

	void set_current_time();
	uint32_t get_current_time() {
		return current_time_;
	}
	uint32_t get_start_time();
	uint32_t realtime(uint32_t expiration) {
		if (expiration == 0) {
			return 0;
		} else {
			uint64_t t = expiration;
			t += current_time_;
			if (t <= 0xFFFFFFFFL) {
				return (uint32_t)t;
			}
		}
		return 0;
	}
	static uint32_t realtime(uint32_t curr_time, uint32_t expiration) {
		if (expiration == 0) {
			return 0;
		} else {
			uint64_t t = expiration;
			t += curr_time;
			if (t <= 0xFFFFFFFFL) {
				return (uint32_t)t;
			}
		}
		return 0;
	}
	bool is_timeout(uint32_t last_time, uint32_t timeout) {
		if (last_time == 0) {
			return false;
		}
		return last_time + timeout <= current_time_;
	}

private:
	uint32_t start_time_;
	volatile uint32_t current_time_;
	volatile uint32_t last_time_;
};

extern Current_Time curr_time_;

#endif // CURRENT_TIME_H


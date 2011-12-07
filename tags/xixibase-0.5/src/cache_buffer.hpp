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

#ifndef CACHE_BUFFER_H
#define CACHE_BUFFER_H

#include "defines.h"

struct Cache_Buffer_Extend {
	Cache_Buffer_Extend* next;
	uint32_t buf_size;
	uint8_t buf[1];
};

template<int BASE_SIZE>
class Cache_Buffer {
public:
	Cache_Buffer() : total_size_(0), offset_(0), extend_(NULL) {
	}

	~Cache_Buffer()  {
		reset();
	}

	uint8_t* prepare(uint32_t size) {
		if (size + total_size_ <= BASE_SIZE) {
			uint8_t* buf = base_buf_ + offset_;
			offset_ += size;
			total_size_ += size;
			return buf;
		} else {
		if (extend_ == NULL || size + offset_ > extend_->buf_size) {
			uint32_t buf_size = BASE_SIZE;
			if (buf_size < size) {
				buf_size = size;
			}
			Cache_Buffer_Extend* cbe = (Cache_Buffer_Extend*)malloc(sizeof(Cache_Buffer_Extend) + buf_size);
			if (cbe != NULL) {
				cbe->buf_size = buf_size;
				cbe->next = extend_;
				extend_ = cbe;
				offset_ = 0;
			} else {
				return NULL;
			}
		}
		uint8_t* buf = extend_->buf + offset_;
		offset_ += size;
		total_size_ += size;
		return buf;
		}
	}

	void reset() {
		while (extend_ != NULL) {
			Cache_Buffer_Extend* next = extend_->next;
			free(extend_);
			extend_ = next;
		}
		total_size_ = 0;
		offset_ = 0;
	}

	uint32_t totla_size() {
		return total_size_;
	}

private:
	uint32_t total_size_;
	uint32_t offset_;
	uint8_t base_buf_[BASE_SIZE];
	Cache_Buffer_Extend* extend_;
};

#endif // CACHE_BUFFER_H

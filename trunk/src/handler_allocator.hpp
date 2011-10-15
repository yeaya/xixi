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

#ifndef HANDLER_ALLOCATOR_HPP
#define HANDLER_ALLOCATOR_HPP

#include <boost/asio.hpp>

template <int DEFAULT_SIZE = 256>
class Handler_Allocator : private boost::noncopyable {
public:
	inline Handler_Allocator()
		: in_use_(false) {
	}
	inline ~Handler_Allocator() {
	}

	inline void* alloc(std::size_t size) {
		//    static int max = 0;
		//    if (size > max) {
		//      max = size;
		//      printf("max=%d\n", max);
		//    }
		if (!in_use_ && size < DEFAULT_SIZE) {
			in_use_ = true;
			return buffer_;
		}

		return ::operator new(size);
	}

	inline void free(void* pointer) {
		if (pointer == buffer_) {
			in_use_ = false;
		} else {
			::operator delete(pointer);
		}
	}

private:
	uint8_t buffer_[DEFAULT_SIZE];
	bool in_use_;
};

template <typename Handler>
class custom_alloc_handler {
public:
	inline custom_alloc_handler(Handler_Allocator<>& ha, Handler h) :
	allocator_(ha), handler_(h) {
	}

	template <typename Arg1>
	inline void operator()(Arg1 arg1) {
		handler_(arg1);
	}

	template <typename Arg1, typename Arg2>
	inline void operator()(Arg1 arg1, Arg2 arg2) {
		handler_(arg1, arg2);
	}

	inline friend void* asio_handler_allocate(std::size_t size,
		custom_alloc_handler<Handler>* this_handler) {
			return this_handler->allocator_.alloc(size);
	}

	inline friend void asio_handler_deallocate(void* pointer, std::size_t size,
		custom_alloc_handler<Handler>* this_handler) {
			this_handler->allocator_.free(pointer);
	}

private:
	Handler_Allocator<>& allocator_;
	Handler handler_;
};

template <typename Handler>
inline custom_alloc_handler<Handler> make_custom_alloc_handler(Handler_Allocator<>& ha, Handler h) {
	return custom_alloc_handler<Handler>(ha, h);
}

#endif // HANDLER_ALLOCATOR_HPP

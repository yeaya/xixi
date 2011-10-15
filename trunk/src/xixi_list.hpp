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

#ifndef XIXI_LIST_H
#define XIXI_LIST_H

namespace xixi {

	template <class T, int N = 1>
	class list_node_base {
	public:
		inline list_node_base() {
			for (int i = 0; i < N; ++i) {
				next_[i] = prev_[i] = NULL;
			}
		}

		inline void reset_list_node_base() {
			for (int i = 0; i < N; ++i) {
				next_[i] = prev_[i] = NULL;
			}
		}

		inline T* next() {
			return next_[0];
		}

		inline T* next(int index) {
			return next_[index];
		}

		inline T* prev() {
			return prev_[0];
		}

		inline T* prev(int index) {
			return prev_[index];
		}

		T* next_[N];
		T* prev_[N];
	};

	template <class T, int N = 1>
	class list_node_ext : public T {
	public:
		inline list_node_ext() {
			for (int i = 0; i < N; ++i) {
				next_[i] = prev_[i] = NULL;
			}
		}

		inline void reset_list_node_ext() {
			for (int i = 0; i < N; ++i) {
				next_[i] = prev_[i] = NULL;
			}
		}

		inline list_node_ext<T, N>* next() {
			return next_[0];
		}

		inline list_node_ext<T, N>* next(int index) {
			return next_[index];
		}

		inline list_node_ext<T, N>* prev() {
			return prev_[0];
		}

		inline list_node_ext<T, N>* prev(int index) {
			return prev_[index];
		}

		list_node_ext<T, N>* next_[N];
		list_node_ext<T, N>* prev_[N];
	};

	template <class T, int N = 1>
	class list_node_cont {
	public:
		inline list_node_cont() {
			for (int i = 0; i < N; ++i) {
				next_[i] = prev_[i] = NULL;
			}
			value_ = NULL;
		}

		inline void reset_list_node_cont() {
			for (int i = 0; i < N; ++i) {
				next_[i] = prev_[i] = NULL;
			}
			value_ = NULL;
		}

		inline list_node_cont<T, N>* next() {
			return next_[0];
		}

		inline list_node_cont<T, N>* next(int index) {
			return next_[index];
		}

		inline list_node_cont<T, N>* prev() {
			return prev_[0];
		}

		inline list_node_cont<T, N>* prev(int index) {
			return prev_[index];
		}

		list_node_cont<T, N>* next_[N];
		list_node_cont<T, N>* prev_[N];
		T value_;
	};

	template <class T, int INDEX = 0, class size_type = uint32_t>
	class list {
	public:
		list() {
			head_ = NULL;
			tail_ = NULL;
			size_ = 0;
		}

		inline bool empty() {
			return head_ == NULL;
		}

		inline bool is_linked(T* p) {
			return (p->next_[INDEX] != NULL) || (p->prev_[INDEX] != NULL) || (p == head_ && p == tail_);
		}

		inline T* front() {
			return head_;
		}

		inline T* back() {
			return tail_;
		}

		inline T* next(T* p) {
			if (p != NULL) {
				return p->next_[INDEX];
			}
			return NULL;
		}

		inline T* prev(T* p) {
			if (p != NULL) {
				return p->prev_[INDEX];
			}
			return NULL;
		}

		inline void push_front(T* p) {
			assert(p != NULL);
			assert(p->prev_[INDEX] == NULL);
			assert(p->next_[INDEX] == NULL);

			if (head_ != NULL) {
				head_->prev_[INDEX] = p;
			}
			p->prev_[INDEX] = NULL;
			p->next_[INDEX] = head_;
			head_ = p;
			if (tail_ == NULL) {
				tail_ = p;
			}
			++size_;
		}

		inline void push_back(T* p) {
			assert(p != NULL);
			assert(p->prev_[INDEX] == NULL);
			assert(p->next_[INDEX] == NULL);

			if (tail_ != NULL) {
				tail_->next_[INDEX] = p;
			}
			p->next_[INDEX] = NULL;
			p->prev_[INDEX] = tail_;
			tail_ = p;
			if (head_ == NULL) {
				head_ = p;
			}
			++size_;
		}

		inline T* pop_front() {
			if (head_ != NULL) {
				T* p = head_;
				head_ = head_->next_[INDEX];
				if (head_ != NULL) {
					head_->prev_[INDEX] = NULL;
				}
				if (tail_ == p) {
					tail_ = NULL;
				}
				--size_;
				p->next_[INDEX] = NULL;
				return p;
			} else {
				return NULL;
			}
		}

		inline T* pop_back() {
			if (tail_ != NULL) {
				T* p = tail_;
				tail_ = tail_->prev_[INDEX];
				if (tail_ != NULL) {
					tail_->next_[INDEX] = NULL;
				}
				if (head_ == p) {
					head_ = NULL;
				}
				--size_;
				p->prev_[INDEX] = NULL;
				return p;
			} else {
				return NULL;
			}
		}

		inline void remove(T* p) {
			assert(p->next_[INDEX] != p);
			assert(p->prev_[INDEX] != p);

			if (head_ == p) {
				assert(p->prev_[INDEX] == NULL);
				head_ = p->next_[INDEX];
			}
			if (tail_ == p) {
				assert(p->next_[INDEX] == NULL);
				tail_ = p->prev_[INDEX];
			}

			if (p->next_[INDEX] != NULL) {
				p->next_[INDEX]->prev_[INDEX] = p->prev_[INDEX];
			}
			if (p->prev_[INDEX] != NULL) {
				p->prev_[INDEX]->next_[INDEX] = p->next_[INDEX];
			}

			p->next_[INDEX] = p->prev_[INDEX] = NULL;

			--size_;
		}

		inline void move_to_front(T* p) {
			assert(p->next_[INDEX] != p);
			assert(p->prev_[INDEX] != p);

			if (head_ != p) {
				if (tail_ != p) {
					assert(p->next_[INDEX] != NULL);
					p->next_[INDEX]->prev_[INDEX] = p->prev_[INDEX];        
				} else {
					assert(p->next_[INDEX] == NULL);
					tail_ = p->prev_[INDEX];
				}
				assert(p->prev_[INDEX] != NULL);
				p->prev_[INDEX]->next_[INDEX] = p->next_[INDEX];

				p->prev_[INDEX] = NULL;
				p->next_[INDEX] = head_;
				head_->prev_[INDEX] = p;
				head_ = p;
			}
		}

		inline void move_to_back(T* p) {
			assert(p->next_[INDEX] != p);
			assert(p->prev_[INDEX] != p);

			if (tail_ != p) {
				if (head_ != p) {
					assert(p->prev_[INDEX] != NULL);
					p->prev_[INDEX]->next_[INDEX] = p->next_[INDEX];
				} else {
					assert(p->prev_[INDEX] == NULL);
					head_ = p->next_[INDEX];
				}
				assert(p->next_[INDEX] != NULL);
				p->next_[INDEX]->prev_[INDEX] = p->prev_[INDEX];

				p->next_[INDEX] = NULL;
				p->prev_[INDEX] = tail_;
				tail_->next_[INDEX] = p;
				tail_ = p;
			}
		}

		inline size_type size() { return size_; }

	private:
		T* head_;
		T* tail_;
		size_type size_;
	};

} // namespace xixi

#endif // XIXI_LIST_H

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

#include <new>
#include <assert.h>
#include "cache.h"
#include "currtime.h"
#include "stats.h"
#include "util.h"
#include "log.h"
#include "peer_cache_pdu.h"

// #define NOTIFY_EXPIRATION_REDUCE
// #define USING_BOOST_POOL

Cache_Mgr cache_mgr_;

#define CALC_ITEM_SIZE(k, d) (sizeof(Cache_Item) + k + d)
#define CHUNK_ALIGN_BYTES 8

Cache_Watch::Cache_Watch(uint32_t watch_id, uint32_t expire_time) {
	watch_id_ = watch_id;
	expire_time_ = expire_time;
	updated_count_ = 0;
	wait_updated_count_ = 0;
}

void Cache_Watch::check_and_set_callback(std::list<uint64_t>& updated_list, uint32_t& updated_count, uint64_t ack_cache_id,
										 boost::shared_ptr<Cache_Watch_Sink>& sp, uint32_t expire_time) {
	expire_time_ = expire_time;
	if (!wait_updated_list_.empty()) {
		if (wait_updated_list_.front() == ack_cache_id) {
			wait_updated_list_.clear();
			wait_updated_count_ = 0;
		} else {
			std::list<uint64_t>::iterator it = wait_updated_list_.begin();
			while (it != wait_updated_list_.end()) {
				updated_list.push_back(*it);
				++it;
			}
			updated_count = wait_updated_count_;
			return;
		}
	}
	if (updated_count_ > 0) {
		std::list<uint64_t>::iterator it = updated_list_.begin();
		while (it != updated_list_.end()) {
			updated_list.push_back(*it);
			++it;
		}
		wait_updated_count_ = updated_count = updated_count_;
		updated_list_.swap(wait_updated_list_);
		updated_count_ = 0;
	} else {
		boost::shared_ptr<Cache_Watch_Sink> p = wp_.lock();
		if (p != NULL) {
			p->on_cache_watch_notify(watch_id_);
		}
		wp_ = sp;
	}
}

void Cache_Watch::check_and_clear_callback(std::list<uint64_t>& updated_list, uint32_t& updated_count) {
	if (updated_count_ > 0) {
		updated_list_.swap(updated_list);
		updated_count = updated_count_;
		updated_count_ = 0;
	}
	wp_.reset();
}

void Cache_Watch::notify_watch(uint64_t cache_id) {
	updated_list_.push_back(cache_id);
	updated_count_++;
	boost::shared_ptr<Cache_Watch_Sink> p = wp_.lock();
	if (p != NULL) {
		p->on_cache_watch_notify(watch_id_);
		wp_.reset();
	}
}

Cache_Mgr::Cache_Mgr() {
	last_cache_id_ = 0;
	class_id_max_ = 0;
	mem_limit_ = 0;
	mem_used_ = 0;
	memset(max_size_, 0, sizeof(max_size_));
	memset(&pools_, 0, sizeof(pools_));
	last_class_id_ = CLASSID_MIN;

	last_print_stats_time_ = 0;

	last_check_expired_time_ = 0;
	last_watch_id_ = 0;

	memset(&expire_check_time_, 0, sizeof(expire_check_time_));
	for (int i = 0; i < 32; i++) {
		expiration_time_[i] = 1 << i;
	}
	expiration_time_[32] = 0xFFFFFFFF;
	//  for (int i = 0; i < 33; i++) {
	//    LOG_INFO("expiration_time_" << i << " =" << expiration_time_[i]);
	//  }
}

Cache_Mgr::~Cache_Mgr() {
}

void Cache_Mgr::init(uint64_t limit, uint32_t item_size_max, uint32_t item_size_min, double factor) {
	LOG_INFO("Cache_Mgr::init, limit=" << limit << " item_size_max=" << item_size_max << " item_size_min=" << item_size_min
		<< " factor=" << factor << " sizeof_item=" << sizeof(Cache_Item));

	uint32_t size = sizeof(Cache_Item) + item_size_min;

	mem_limit_ = limit;

	class_id_max_ = CLASSID_MIN;
	for (; class_id_max_ < CLASSID_MAX && size <= (sizeof(Cache_Item) + item_size_max) / factor; ++class_id_max_) {

		if (size % CHUNK_ALIGN_BYTES) {
			size += CHUNK_ALIGN_BYTES - (size % CHUNK_ALIGN_BYTES);
		}

		max_size_[class_id_max_] = size;
		pools_[class_id_max_] = new boost::pool<>(size);
		size = (uint32_t)(size * factor);
	}
	max_size_[class_id_max_] = sizeof(Cache_Item) + item_size_max;
	stats_.set_max_class_id(class_id_max_);
	LOG_INFO("Cache_Mgr::init, class_id_max=" << class_id_max_ << " max_size=" << max_size_[class_id_max_]);
}

uint32_t Cache_Mgr::get_class_id(size_t size) {
	if (size < max_size_[last_class_id_]) {
		if (size > max_size_[last_class_id_ - 1]) {
			return last_class_id_;
		} else {
			for (--last_class_id_; last_class_id_ >= CLASSID_MIN; --last_class_id_) {
				if (size > max_size_[last_class_id_ - 1]) {
					break;
				}
			}
			return last_class_id_;
		}
	} else {
		for (; last_class_id_ <= class_id_max_; ++last_class_id_) {
			if (size <= max_size_[last_class_id_]) {
				return last_class_id_;
			}
		}
	}
	return 0;
}

uint64_t Cache_Mgr::get_cache_id() {
	if (++last_cache_id_ == 0) {
		last_cache_id_ = 1;
	}
	return last_cache_id_;
}

void Cache_Mgr::check_expired() {
	uint32_t curr_time = curr_time_.get_current_time();
	if (curr_time != last_check_expired_time_) {
		last_check_expired_time_ = curr_time;
		cache_lock_.lock();

		expire_watchs(curr_time);
		expire_items(curr_time);
		free_flushed_items();

		cache_lock_.unlock();
	}
}

void Cache_Mgr::stats(const XIXI_Stats_Req_Pdu* pdu, std::string& result) {
	cache_lock_.lock();
	switch (pdu->sub_op()) {
	case XIXI_STATS_SUB_OP_ADD_GROUP:
		if (stats_.add_group(pdu->group_id)) {
			result = "success";
		} else {
			result = "fail";
		}
		break;
	case XIXI_STATS_SUB_OP_REMOVE_GROUP:
		if (stats_.remove_group(pdu->group_id)) {
			result = "success";
		} else {
			result = "fail";
		}
		break;
	case XIXI_STATS_SUB_OP_GET_STATS_GROUP_ONLY:
		stats_.get_stats(pdu->group_id, pdu->class_id, result);
		break;
	case XIXI_STATS_SUB_OP_GET_AND_CLEAR_STATS_GROUP_ONLY:
		stats_.get_and_clear_stats(pdu->group_id, pdu->class_id, result);
		break;
	case XIXI_STATS_SUB_OP_GET_STATS_SUM_ONLY:
		stats_.get_stats(pdu->class_id, result);
		break;
	case XIXI_STATS_SUB_OP_GET_AND_CLEAR_STATS_SUM_ONLY:
		stats_.get_and_clear_stats(pdu->class_id, result);
		break;
	default:
		result = "unknown sub command";
		break;
	}
	cache_lock_.unlock();
}

void Cache_Mgr::print_stats() {
	uint32_t curr_time = curr_time_.get_current_time();
	if (curr_time >= last_print_stats_time_ + 5) {
		last_print_stats_time_ = curr_time;
		cache_lock_.lock();

		stats_.print();

		cache_lock_.unlock();
	}
}

void Cache_Mgr::expire_items(uint32_t curr_time) {
	for (int i = 0; i < 33; i++) {
		if (curr_time >= expire_check_time_[i]) {
			expire_check_time_[i] = curr_time_.realtime(curr_time, expiration_time_[i]);

			Cache_Item* it = expire_list_[i].front();
			while (it != NULL) {
				if (it->expire_time <= curr_time) {
					Cache_Item* next = it->next();
					do_unlink(it);
					it = next;
				} else {
					Cache_Item* next = it->next();
					uint32_t expiration = it->expire_time - curr_time;
					for (int j = i; j >= 0; j--) {
						if (expiration >= expiration_time_[j]) {
							expire_list_[i].remove(it);
							it->expiration_id = j;
							expire_list_[j].push_front(it);
							break;
						}
					}
					it = next;
				}
			}
		}
	}
}

Cache_Item* Cache_Mgr::do_alloc(uint32_t group_id, size_t key_length, uint32_t flags, 
								uint32_t expire_time, uint32_t data_size) {
	uint32_t item_size = CALC_ITEM_SIZE(key_length, data_size);

	uint32_t id = get_class_id(item_size);
	if (id == 0) {
		return NULL;
	}
	item_size = max_size_[id];

	Cache_Item* it = free_cache_list_[id].pop_front();
	if (it == NULL) {
		if (mem_used_ + item_size <= mem_limit_) {
#ifdef USING_BOOST_POOL
			void* buf = pools_[id]->malloc();
#else
			void* buf = malloc(item_size);
#endif
			if (buf != NULL) {
				it = new (buf) Cache_Item;
				mem_used_ += item_size;
			} else {
				return NULL;
			}
		} else {
			return NULL;
		}
	}

	it->class_id = (uint8_t)id;
	it->expiration_id = (uint8_t)get_expiration_id(curr_time_.get_current_time(), expire_time);
	it->ref_count = 1;
	it->group_id = group_id;
	it->key_length = key_length;
	it->data_size = data_size;
	it->expire_time = expire_time;
	it->flags = flags;

	return it;
}

void Cache_Mgr::free_item(Cache_Item* it) {
	assert(!expire_list_[it->expiration_id].is_linked(it));
	assert(it->ref_count == 0);

	uint32_t id = it->class_id;
	it->reset();
	if (free_cache_list_[id].size() < 1000) {
		free_cache_list_[id].push_front(it);
	} else {
		uint32_t item_size = max_size_[id];
#ifdef USING_BOOST_POOL
		pools_[id]->free(it);
#else
		::free(it);
#endif
		mem_used_ -= item_size;
	}
}

bool Cache_Mgr::item_size_ok(size_t key_length, uint32_t data_size) {
	return get_class_id(CALC_ITEM_SIZE(key_length, data_size)) != 0;
}

uint32_t Cache_Mgr::get_expiration_id(uint32_t curr_time, uint32_t expire_time) {
	if (expire_time == 0) {
		return 33;
	} else if (expire_time > curr_time) {
		uint32_t expiration = expire_time - curr_time;
		uint32_t i = 1;
		for (; i < 33; i++) {
			if (expiration < expiration_time_[i]) {
				break;
			}
		}
		return i - 1;
	} else {
		return 0;
	}
}

void Cache_Mgr::do_link(Cache_Item* it) {
	cache_hash_map_.insert(it, it->hash_value_);

	stats_.item_link(it->group_id, it->class_id, it->total_size());

	it->cache_id = get_cache_id();

	it->ref_count++;
	expire_list_[it->expiration_id].push_back(it);
}

void Cache_Mgr::do_unlink(Cache_Item* it) {
	assert(expire_list_[it->expiration_id].is_linked(it));

	stats_.item_unlink(it->group_id, it->class_id, it->total_size());

	Cache_Key ck(it->group_id, it->get_key(), it->key_length);
	cache_hash_map_.remove(&ck, it->hash_value_);

	expire_list_[it->expiration_id].remove(it);

	if (it->watch_item != NULL) {
		notify_watch(it);
		delete it->watch_item;
		it->watch_item = NULL;
	}
	do_release_reference(it);
}

void Cache_Mgr::do_unlink_flush(Cache_Item* it) {
	assert(expire_list_[it->expiration_id].is_linked(it));

	stats_.item_unlink(it->group_id, it->class_id, it->total_size());
	Cache_Key ck(it->group_id, it->get_key(), it->key_length);
	cache_hash_map_.remove(&ck, it->hash_value_);

	expire_list_[it->expiration_id].remove(it);
	if (it->watch_item != NULL) {
		notify_watch(it);
		delete it->watch_item;
		it->watch_item = NULL;
	}
	flush_cache_list_.push_back(it);
}

void Cache_Mgr::do_release_reference(Cache_Item* it) {
	assert(it->ref_count > 0);
	it->ref_count--;
	if (it->ref_count == 0) {
		free_item(it);
	}
}

void Cache_Mgr::do_replace(Cache_Item* it, Cache_Item* new_it) {
	do_unlink(it);
	do_link(new_it);
}

Cache_Item* Cache_Mgr::do_get(uint32_t group_id, const uint8_t* key, size_t key_length, uint32_t hash_value) {
	Cache_Key ck(group_id, key, key_length);
	Cache_Item* it = cache_hash_map_.find(&ck, hash_value);

	if (it != NULL) {
		LOG_TRACE("Cache_Mgr.do_get, found, key " << string((char*)key, key_length));
		it->ref_count++;
	} else {
		LOG_TRACE("Cache_Mgr.do_get, not found, key " << string((char*)key, key_length));
	}

	return it;
}

Cache_Item* Cache_Mgr::do_get(uint32_t group_id, const uint8_t* key, size_t key_length,
							  uint32_t hash_value, uint32_t&/*out*/ expiration) {
	Cache_Key ck(group_id, key, key_length);
	Cache_Item* it = cache_hash_map_.find(&ck, hash_value);

	if (it != NULL) {
		LOG_TRACE("Cache_Mgr.do_get, found, key " << string((char*)key, key_length));

		if (it->expire_time == 0) {
			it->ref_count++;
			expiration = 0;
		} else {
			uint32_t currtime = curr_time_.get_current_time();
			if (it->expire_time > currtime) {
				it->ref_count++;
				expiration = it->expire_time - currtime;
			} else {
				do_unlink(it);
				it = NULL;
			}
		}
	} else {
		LOG_TRACE("Cache_Mgr.do_get, not found, key " << string((char*)key, key_length));
	}

	return it;
}

Cache_Item* Cache_Mgr::do_get_touch(uint32_t group_id, const uint8_t* key, size_t key_length,
		uint32_t hash_value, uint32_t expiration) {
	Cache_Key ck(group_id, key, key_length);
	Cache_Item* it = cache_hash_map_.find(&ck, hash_value);

	if (it != NULL) {
		LOG_TRACE("Cache_Mgr.do_get_touch, found, key " << string((char*)key, key_length));

		it->ref_count++;
		uint32_t expire_time = curr_time_.realtime(expiration);
#ifdef NOTIFY_EXPIRATION_REDUCE
		if (expire_time < it->expire_time && expire_time != 0) {
			if (it->watch_item != NULL) {
				notify_watch(it);
				it->cache_id = get_cache_id();
			}
		}
#endif
		it->expire_time = expire_time;
	} else {
		LOG_TRACE("Cache_Mgr.do_get_touch, not found, key " << string((char*)key, key_length));
	}

	return it;
}

Cache_Item* Cache_Mgr::alloc_item(uint32_t group_id, size_t key_length, uint32_t flags,
								  uint32_t expiration, uint32_t data_size) {
	Cache_Item* it;
	cache_lock_.lock();
	it = do_alloc(group_id, key_length, flags, curr_time_.realtime(expiration), data_size);
	cache_lock_.unlock();
	return it;
}

Cache_Item*  Cache_Mgr::get(uint32_t group_id, const uint8_t* key, size_t key_length, uint32_t watch_id,
							uint32_t&/*out*/ expiration, bool&/*out*/ watch_error) {
	Cache_Item* item;
	uint32_t hash_value = hash32(key, key_length, group_id);
	watch_error = false;
	cache_lock_.lock();

	item = do_get(group_id, key, key_length, hash_value, expiration);
	if (item != NULL) {
		if (watch_id != 0) {
			if (is_valid_watch_id(watch_id)) {
				item->add_watch(watch_id);
				stats_.get_hit_watch(group_id, item->class_id);
			} else {
				watch_error = true;
				stats_.get_hit_watch_miss(group_id, item->class_id);
				do_release_reference(item);
				item = NULL;
			}
		} else {
			stats_.get_hit_no_watch(group_id, item->class_id);
		}
	} else {
		stats_.get_miss(group_id);
	}
	cache_lock_.unlock();
	return item;
}

Cache_Item*  Cache_Mgr::get_touch(uint32_t group_id, const uint8_t* key, size_t key_length, uint32_t watch_id,
								uint32_t expiration, bool&/*out*/ watch_error) {
	Cache_Item* item;
	uint32_t hash_value = hash32(key, key_length, group_id);
	watch_error = false;
	cache_lock_.lock();

	item = do_get_touch(group_id, key, key_length, hash_value, expiration);
	if (item != NULL) {
	  if (watch_id != 0) {
		  if (is_valid_watch_id(watch_id)) {
			  item->add_watch(watch_id);
			  stats_.get_touch_hit_watch(group_id, item->class_id);
		  } else {
			  watch_error = true;
			  stats_.get_touch_hit_watch_miss(group_id, item->class_id);
			  do_release_reference(item);
			  item = NULL;
		  }
	  } else {
		  stats_.get_touch_hit_no_watch(group_id, item->class_id);
	  }
	} else {
	  stats_.get_touch_miss(group_id);
	}
	cache_lock_.unlock();
	return item;
}

bool Cache_Mgr::get_base(uint32_t group_id, const uint8_t* key, size_t key_length, uint64_t&/*out*/ cache_id,
						 uint32_t&/*out*/ flags, uint32_t&/*out*/ expiration) {
	 Cache_Item* it;
	 bool ret;
	 uint32_t hash_value = hash32(key, key_length, group_id);
	 cache_lock_.lock();
	 it = do_get(group_id, key, key_length, hash_value, expiration);
	 if (it != NULL) {
		 cache_id = it->cache_id;
		 flags = it->flags;
		 stats_.get_base_hit(it->group_id, it->class_id);
		 do_release_reference(it);
		 ret = true;
	 } else {
		 stats_.get_base_miss(group_id);
		 ret = false;
	 }
	 cache_lock_.unlock();
	 return ret;
}

bool Cache_Mgr::update_base(uint32_t group_id, const uint8_t* key, size_t key_length, const XIXI_Update_Base_Req_Pdu* pdu, uint64_t&/*out*/ cache_id) {
	Cache_Item* it;
	bool ret = true;
	cache_id = 0;
	uint32_t hash_value = hash32(key, key_length, group_id);
	cache_lock_.lock();
	it = do_get(group_id, key, key_length, hash_value);
	if (it != NULL) {
		if (pdu->cache_id == 0 || pdu->cache_id == it->cache_id) {
			bool need_notify = false;
			if (pdu->is_update_expiration()) {
				uint32_t expire_time = curr_time_.realtime(pdu->expiration);
#ifdef NOTIFY_EXPIRATION_REDUCE
				if (expire_time < it->expire_time && expire_time != 0) {
					need_notify = true;
				}
#endif
				it->expire_time = expire_time;
			}
			if (pdu->is_update_flags()) {
				it->flags = pdu->flags;
				need_notify = true;
			}
			if (it->watch_item != NULL && need_notify) {
				notify_watch(it);
				it->cache_id = get_cache_id();
			}
			cache_id = it->cache_id;
			stats_.update_base_success(it->group_id, it->class_id);
			do_release_reference(it);
		} else {
			cache_id = -1;
			stats_.update_base_mismatch(it->group_id, it->class_id);
			ret = false;
		}
	} else {
		stats_.update_base_miss(group_id);
		ret = false;
	}
	cache_lock_.unlock();
	return ret;
}

void Cache_Mgr::release_reference(Cache_Item* item) {
	cache_lock_.lock();
	do_release_reference(item);
	cache_lock_.unlock();
}

xixi_reason Cache_Mgr::add(Cache_Item* item, uint32_t watch_id, uint64_t&/*out*/ cache_id) {
	xixi_reason reason = XIXI_REASON_SUCCESS;

	cache_lock_.lock();

	Cache_Item* old_it = do_get(item->group_id, item->get_key(), item->key_length, item->hash_value_);
	if (old_it == NULL) {
		if (watch_id != 0) {
			if (is_valid_watch_id(watch_id)) {
				item->add_watch(watch_id);
				stats_.add_success_watch(item->group_id, item->class_id);
			} else {
				stats_.add_watch_miss(item->group_id, item->class_id);
				reason = XIXI_REASON_WATCH_NOT_FOUND;
			}
		} else {
			stats_.add_success(item->group_id, item->class_id);
		}

		if (reason == XIXI_REASON_SUCCESS) {
			do_link(item);
			cache_id = item->cache_id;
		}
	} else {
		do_release_reference(old_it);
		stats_.add_fail(item->group_id, item->class_id);
		reason = XIXI_REASON_EXISTS;
	}

	cache_lock_.unlock();
	return reason;
}

xixi_reason Cache_Mgr::set(Cache_Item* item, uint32_t watch_id, uint64_t&/*out*/ cache_id) {
	xixi_reason reason = XIXI_REASON_SUCCESS;

	cache_lock_.lock();

	Cache_Item* old_it = do_get(item->group_id, item->get_key(), item->key_length, item->hash_value_);
	if (old_it != NULL) {
		//    LOG_ERROR("Cache_Mgr set, key " << string((char*)item->get_key(), item->key_length) << " hash_value=" << it->hash_value);
		if (item->cache_id == 0 || item->cache_id == old_it->cache_id) {
			if (watch_id != 0) {
				if (is_valid_watch_id(watch_id)) {
					stats_.set_success_watch(item->group_id, item->class_id);
				} else {
					stats_.set_watch_miss(item->group_id, item->class_id);
					reason = XIXI_REASON_WATCH_NOT_FOUND;
				}
			} else {
				stats_.set_success(item->group_id, item->class_id);
			}
			if (reason == XIXI_REASON_SUCCESS) {
				do_replace(old_it, item);
				// after notify last watch, then add new watch
				if (watch_id != 0) {
					item->add_watch(watch_id);
				}
				cache_id = item->cache_id;
			}
		} else {
			stats_.set_mismatch(item->group_id, item->class_id);
			reason = XIXI_REASON_MISMATCH;
		}
		do_release_reference(old_it);
	} else {
		if (watch_id != 0) {
			if (is_valid_watch_id(watch_id)) {
				item->add_watch(watch_id);
				stats_.set_success_watch(item->group_id, item->class_id);
			} else {
				stats_.set_watch_miss(item->group_id, item->class_id);
				reason = XIXI_REASON_WATCH_NOT_FOUND;
			}
		} else {
			stats_.set_success(item->group_id, item->class_id);
		}
		if (reason == XIXI_REASON_SUCCESS) {
			do_link(item);
			cache_id = item->cache_id;
		}
	}
	cache_lock_.unlock();
	return reason;
}

xixi_reason Cache_Mgr::replace(Cache_Item* it, uint32_t watch_id, uint64_t&/*out*/ cache_id) {
	xixi_reason reason = XIXI_REASON_SUCCESS;

	cache_lock_.lock();

	Cache_Item* old_it = do_get(it->group_id, it->get_key(), it->key_length, it->hash_value_);

	if (old_it == NULL) {
		reason = XIXI_REASON_NOT_FOUND;
		stats_.replace_miss(it->group_id);
	} else if (it->cache_id == 0 || it->cache_id == old_it->cache_id) {
		if (watch_id != 0) {
			if (is_valid_watch_id(watch_id)) {
				stats_.replace_success_watch(it->group_id, it->class_id);
			} else {
				stats_.replace_watch_miss(it->group_id, it->class_id);
				reason = XIXI_REASON_WATCH_NOT_FOUND;
			}
		} else {
			stats_.replace_success(old_it->group_id, old_it->class_id);
		}
		if (reason == XIXI_REASON_SUCCESS) {
			do_replace(old_it, it);
			// after notify last watch, then add new watch
			if (watch_id != 0) {
				it->add_watch(watch_id);
			}
			cache_id = it->cache_id;
		}
		do_release_reference(old_it);
	} else {
		reason = XIXI_REASON_MISMATCH;
		stats_.replace_mismatch(it->group_id, it->class_id);
		do_release_reference(old_it);
	}
	cache_lock_.unlock();
	return reason;
}

xixi_reason Cache_Mgr::append(Cache_Item* it, uint32_t watch_id, uint64_t&/*out*/ cache_id) {
	xixi_reason reason = XIXI_REASON_SUCCESS;

	cache_lock_.lock();

	Cache_Item* old_it = do_get(it->group_id, it->get_key(), it->key_length, it->hash_value_);
	if (old_it != NULL) {
		if (it->cache_id != 0 && it->cache_id != old_it->cache_id) {
			stats_.append_mismatch(it->group_id, it->class_id);
			reason = XIXI_REASON_MISMATCH;
		} else {
			Cache_Item* new_it = do_alloc(it->group_id, it->key_length, old_it->flags, old_it->expire_time, it->data_size + old_it->data_size);
			if (new_it != NULL) {
				new_it->set_key_with_hash(it->get_key(), it->hash_value_);
				memcpy(new_it->get_data(), old_it->get_data(), old_it->data_size);
				memcpy(new_it->get_data() + old_it->data_size, it->get_data(), it->data_size);
				if (watch_id != 0) {
					if (is_valid_watch_id(watch_id)) {
						stats_.append_success_watch(it->group_id, it->class_id);
					} else {
						stats_.append_watch_miss(it->group_id, it->class_id);
						reason = XIXI_REASON_WATCH_NOT_FOUND;
					}
				} else {
					stats_.append_success(old_it->group_id, old_it->class_id);
				}
				if (reason == XIXI_REASON_SUCCESS) {
					do_replace(old_it, new_it);
					// after notify last watch, then add new watch
					if (watch_id != 0) {
						it->add_watch(watch_id);
					}
					cache_id = new_it->cache_id;
				}
				do_release_reference(new_it);
			} else {
				stats_.append_out_of_memory(it->group_id, it->class_id);
				reason = XIXI_REASON_OUT_OF_MEMORY;
			}
			do_release_reference(old_it);
		}
	} else {
		stats_.append_miss(it->group_id);
		reason = XIXI_REASON_NOT_FOUND;
	}

	cache_lock_.unlock();
	return reason;
}

xixi_reason Cache_Mgr::prepend(Cache_Item* it, uint32_t watch_id, uint64_t&/*out*/ cache_id) {
	xixi_reason reason = XIXI_REASON_SUCCESS;

	cache_lock_.lock();

	Cache_Item* old_it = do_get(it->group_id, it->get_key(), it->key_length, it->hash_value_);
	if (old_it != NULL) {
		if (it->cache_id != 0 && it->cache_id != old_it->cache_id) {
			stats_.prepend_mismatch(it->group_id, it->class_id);
			reason = XIXI_REASON_MISMATCH;
		} else {
			Cache_Item* new_it = do_alloc(it->group_id, it->key_length, old_it->flags, old_it->expire_time, it->data_size + old_it->data_size);
			if (new_it != NULL) {
				new_it->set_key_with_hash(it->get_key(), it->hash_value_);
				memcpy(new_it->get_data(), it->get_data(), it->data_size);
				memcpy(new_it->get_data() + it->data_size, old_it->get_data(), old_it->data_size);
				if (watch_id != 0) {
					if (is_valid_watch_id(watch_id)) {
						stats_.prepend_success_watch(it->group_id, it->class_id);
					} else {
						stats_.prepend_watch_miss(it->group_id, it->class_id);
						reason = XIXI_REASON_WATCH_NOT_FOUND;
					}
				} else {
					stats_.prepend_success(old_it->group_id, old_it->class_id);
				}
				if (reason == XIXI_REASON_SUCCESS) {
					// after notify last watch, then add new watch
					if (watch_id != 0) {
						it->add_watch(watch_id);
					}
					do_replace(old_it, new_it);
					cache_id = new_it->cache_id;
				}
				do_release_reference(new_it);
			} else {
				stats_.prepend_out_of_memory(it->group_id, it->class_id);
				reason = XIXI_REASON_OUT_OF_MEMORY;
			}
			do_release_reference(old_it);
		}
	} else {
		stats_.prepend_miss(it->group_id);
		reason = XIXI_REASON_NOT_FOUND;
	}

	cache_lock_.unlock();
	return reason;
}

xixi_reason Cache_Mgr::remove(uint32_t group_id, const uint8_t* key, uint32_t key_length, uint64_t cache_id) {
	xixi_reason reason;
	uint32_t hash_value = hash32(key, key_length, group_id);

	cache_lock_.lock();

	Cache_Item* it = do_get(group_id, key, key_length, hash_value);
	if (it != NULL) {
		if (cache_id == 0 || cache_id == it->cache_id) {
			stats_.delete_success(group_id, it->class_id);
			do_unlink(it);
			reason = XIXI_REASON_SUCCESS;
		} else {
			stats_.delete_mismatch(group_id, it->class_id);
			reason = XIXI_REASON_MISMATCH;
		}
		do_release_reference(it);
	} else {
		stats_.delete_miss(group_id);
		reason = XIXI_REASON_NOT_FOUND;
	}

	cache_lock_.unlock();
	return reason;
}

#define INCR_MAX_STORAGE_LEN 24
xixi_reason Cache_Mgr::delta(uint32_t group_id, const uint8_t* key, uint32_t key_length, bool incr, int64_t delta, uint64_t&/*out*/ cache_id, int64_t&/*out*/ value) {
	xixi_reason reason;
	uint32_t hash_value = hash32(key, key_length, group_id);
	cache_lock_.lock();

	Cache_Item* it = do_get(group_id, key, key_length, hash_value);
	if (it == NULL) {
		cache_id = 0;
		value = 0;
		if (incr) {
			stats_.incr_miss(group_id);
		} else {
			stats_.decr_miss(group_id);
		}
		reason = XIXI_REASON_NOT_FOUND;
	} else if (cache_id == 0 || cache_id == it->cache_id) {
		if (!safe_toi64(it->get_data(), it->data_size, value)) {
			cache_id = 0;
			value = 0;

			reason = XIXI_REASON_INVALID_PARAMETER;
		} else {
			if (incr) {
				value += delta;
			} else {
				if (delta > value) {
					value = 0;
				} else {
					value -= delta;
				}
			}

			char buf[INCR_MAX_STORAGE_LEN];
			uint32_t data_size = _snprintf(buf, INCR_MAX_STORAGE_LEN, "%lld ", value);
			if (data_size > it->data_size) {
				Cache_Item* new_it = do_alloc(it->group_id, it->key_length, it->flags, it->expire_time, data_size);
				if (new_it == NULL) {
					reason = XIXI_REASON_OUT_OF_MEMORY;
				} else {
					new_it->set_key_with_hash(it->get_key(), it->hash_value_);
					memcpy(new_it->get_data(), buf, data_size);
					do_replace(it, new_it);
					cache_id = new_it->cache_id;
					do_release_reference(new_it);
					if (incr) {
						stats_.incr_success(group_id);
					} else {
						stats_.decr_success(group_id);
					}
					reason = XIXI_REASON_SUCCESS;
				}
			} else {
				it->cache_id = get_cache_id();

				memcpy(it->get_data(), buf, data_size);
				memset(it->get_data() + data_size, ' ', it->data_size - data_size);
				cache_id = it->cache_id;
				if (incr) {
					stats_.incr_success(group_id);
				} else {
					stats_.decr_success(group_id);
				}
				reason = XIXI_REASON_SUCCESS;
			}
		}
		do_release_reference(it);
	} else {
		cache_id = 0;
		value = 0;
		if (incr) {
			stats_.incr_mismatch(group_id);
		} else {
			stats_.decr_mismatch(group_id);
		}
		reason = XIXI_REASON_MISMATCH;
		do_release_reference(it);
	}

	cache_lock_.unlock();
	return reason;
}

void Cache_Mgr::flush(uint32_t group_id, uint32_t&/*out*/ flush_count, uint64_t&/*out*/ flush_size) {
	flush_count = 0;
	flush_size = 0;
	cache_lock_.lock();
	for (int i = 0; i < 34; i++) {
		Cache_Item* it = expire_list_[i].front();
		while (it != NULL) {
			Cache_Item* next = it->next();
			if (it->group_id == group_id) {
				flush_count++;
				flush_size += it->total_size();
				//  do_unlink_flush(it);
				do_unlink(it);
			}
			it = next;
		}
	}
	stats_.flush(group_id);
	cache_lock_.unlock();
}

uint32_t Cache_Mgr::get_watch_id() {
	for (int i = 0; i < 100; i++) {
		if (++last_watch_id_ == 0) {
			last_watch_id_ = 1;
		}
		if (watch_map_.find(last_watch_id_) == watch_map_.end()) {
			return last_watch_id_;
		}
	}
	return 0;
}

bool Cache_Mgr::is_valid_watch_id(uint32_t watch_id) {
	return watch_map_.find(watch_id) != watch_map_.end();
}

uint32_t Cache_Mgr::create_watch(uint32_t group_id, uint32_t max_next_check_interval) {
	cache_lock_.lock();
	uint32_t watch_id = get_watch_id();
	if (watch_id != 0) {
		shared_ptr<Cache_Watch> sp(new Cache_Watch(watch_id, curr_time_.realtime(max_next_check_interval)));
		watch_map_[watch_id] = sp;
		stats_.create_watch(group_id);
	}
	cache_lock_.unlock();
	return watch_id;
}

bool Cache_Mgr::check_watch_and_set_callback(uint32_t group_id, uint32_t watch_id, std::list<uint64_t>& updated_list, uint32_t& updated_count,
											 uint64_t ack_cache_id, boost::shared_ptr<Cache_Watch_Sink>& sp, uint32_t max_next_check_interval) {
	 bool ret = false;
	 cache_lock_.lock();
	 std::map<uint32_t, shared_ptr<Cache_Watch> >::iterator it = watch_map_.find(watch_id);
	 if (it != watch_map_.end()) {
		 it->second->check_and_set_callback(updated_list, updated_count, ack_cache_id, sp, curr_time_.realtime(max_next_check_interval));
		 ret = true;
		 stats_.check_watch(group_id);
	 } else {
		 stats_.check_watch_miss(group_id);
	 }
	 cache_lock_.unlock();
	 return ret;
}

bool Cache_Mgr::check_watch_and_clear_callback(uint32_t watch_id, std::list<uint64_t>& updated_list, uint32_t& updated_count) {
	bool ret = false;
	cache_lock_.lock();
	std::map<uint32_t, shared_ptr<Cache_Watch> >::iterator it = watch_map_.find(watch_id);
	if (it != watch_map_.end()) {
		it->second->check_and_clear_callback(updated_list, updated_count);
		ret = true;
	}
	cache_lock_.unlock();
	return ret;
}

void Cache_Mgr::notify_watch(Cache_Item* item) {
	std::set<uint32_t>::iterator it = item->watch_item->watch_map.begin();
	while (it != item->watch_item->watch_map.end()) {
		uint32_t watch_id = *it;
		std::map<uint32_t, shared_ptr<Cache_Watch> >::iterator it2 = watch_map_.find(watch_id);
		if (it2 != watch_map_.end()) {
			it2->second->notify_watch(item->cache_id);
			++it;
		} else {
			item->watch_item->watch_map.erase(it++);
		}
	}
}

void Cache_Mgr::expire_watchs(uint32_t curr_time) {
	//  LOG_INFO("Cache_Mgr::expire_watchs curr_time=" << curr_time);
	std::map<uint32_t, shared_ptr<Cache_Watch> >::iterator it = watch_map_.begin();
	while (it != watch_map_.end()) {
		if (it->second->is_expired(curr_time)) {
			watch_map_.erase(it++);
		} else {
			++it;
		}
	}
}

void Cache_Mgr::free_flushed_items() {
	Cache_Item* item = flush_cache_list_.front();
	while (item != NULL) {
		Cache_Item* next = item->next();
		flush_cache_list_.remove(item);
		do_release_reference(item);
		item = next;
	}
}


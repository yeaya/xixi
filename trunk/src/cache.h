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

#ifndef CACHE_H
#define CACHE_H

// #define USING_BOOST_POOL

#include "defines.h"
#include "util.h"
#include "peer_pdu.h"
#include "xixi_list.hpp"
#include "xixi_hash_map.hpp"
#include "hash.h"
#ifdef USING_BOOST_POOL
#include <boost/pool/pool.hpp>
#endif
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/tss.hpp>
#include <boost/smart_ptr/weak_ptr.hpp>

class Cache_Watch_Sink {
public:
	virtual void on_cache_watch_notify(uint32_t watch_id) = 0;
};

class Cache_Watch {
public:
	Cache_Watch(uint32_t watch_id, uint32_t expire_time_);
	void check_and_set_callback(std::list<uint64_t>& updated_list, uint32_t& updated_count, uint64_t ack_cache_id, boost::shared_ptr<Cache_Watch_Sink>& sp, uint32_t expire_time);
	void check_and_clear_callback(std::list<uint64_t>& updated_list, uint32_t& updated_count);
	void notify_watch(uint64_t cache_id);
	bool is_expired(uint32_t current_time) {
		return current_time >= expire_time_;
	}

private:
	uint32_t watch_id_;
	uint32_t max_check_interval_;
	uint32_t expire_time_;
	std::list<uint64_t> updated_list_;
	uint32_t updated_count_;
	std::list<uint64_t> wait_updated_list_;
	uint32_t wait_updated_count_;
	boost::weak_ptr<Cache_Watch_Sink> wp_;
};

class Cache_Watch_Item {
public:
	void add_watch(uint32_t watch_id) {
		watch_map.insert(watch_id);
	}
	std::set<uint32_t> watch_map;
};

struct Cache_Key {
	Cache_Key() : group_id(0), size(0), data(NULL) {}
	Cache_Key(uint32_t g, const void* d, uint32_t s) {
		group_id = g;
		data = d;
		size = s;
	}
	inline bool equal(const Cache_Key* sd) const {
		return (group_id == sd->group_id) && (size == sd->size) && (memcmp(data, sd->data, size) == 0);
	}
	inline uint32_t hash_value() const {
		return hash32(data, size, group_id);
	}
	uint32_t group_id;
	uint32_t size;
	const void* data;
};

class Cache_Item : public xixi::list_node_base<Cache_Item>, public xixi::hash_node_base<Cache_Key, Cache_Item> {
	friend class Cache_Mgr;
public:
	Cache_Item() {
		watch_item = NULL;
		expire_time = 0;
		cache_id = 0;
		flags = 0;
		group_id = 0;
		data_size = 0;
		ref_count = 0;
		class_id = 0;
		expiration_id = 0;
		key_length = 0;
	}
	~Cache_Item() {
		reset();
	}
	void reset() {
		if (watch_item != NULL) {
			delete watch_item;
			watch_item = NULL;
		}
		expire_time = 0;
		cache_id = 0;
		flags = 0;
		group_id = 0;
		data_size = 0;
		ref_count = 0;
		class_id = 0;
		expiration_id = 0;
		key_length = 0;
	}
	void set_key(uint8_t* key) {
		memcpy(get_key(), key, key_length);
		calc_hash_value();
	}
	void set_key_with_hash(uint8_t* key, uint32_t hash_value) {
		memcpy(get_key(), key, key_length);
		hash_value_ = hash_value;
	}
	inline bool is_key(const Cache_Key* p) const { return (group_id == p->group_id) && (key_length == p->size) && (memcmp((uint8_t*)body, p->data, key_length) == 0); }
	inline uint8_t* get_key() { return (uint8_t*)body; }
	inline uint32_t get_key_length() { return key_length; }

	inline uint8_t* get_data() { return ((uint8_t*)body) + key_length; }
	inline uint32_t total_size() { return sizeof(Cache_Item) + key_length + data_size; }

	inline void calc_hash_value() { hash_value_ = hash32((uint8_t*)body, key_length, group_id); }

protected:
	void add_watch(uint32_t watch_id) {
		if (watch_item == NULL) {
			watch_item = new Cache_Watch_Item();
		}
		watch_item->add_watch(watch_id);
	}

protected:
	Cache_Watch_Item* watch_item;
	uint32_t expire_time;
public:
	uint64_t cache_id;
	uint32_t group_id;
	uint32_t flags;
	uint32_t data_size;
protected:
	uint16_t ref_count;
public:
	uint16_t key_length;
protected:
	uint8_t class_id;
	uint8_t expiration_id;
	void*    body[1];
};

#define CLASSID_MIN 1
#define CLASSID_MAX  200

class XIXI_Update_Flags_Req_Pdu;
class XIXI_Update_Expiration_Req_Pdu;
class XIXI_Stats_Req_Pdu;

class Cache_Mgr {
public:
	Cache_Mgr();
	~Cache_Mgr();

	void init(uint64_t limit, uint32_t item_size_max, uint32_t item_size_min, double factor);
	Cache_Item* alloc_item(uint32_t group_id, uint32_t key_length, uint32_t flags, uint32_t expiration, uint32_t data_size);
	void flush(uint32_t group_id, uint32_t&/*out*/ flush_count, uint64_t&/*out*/ flush_size);
	Cache_Item* get(uint32_t group_id, const uint8_t* key, uint32_t key_length, uint32_t watch_id, uint32_t&/*out*/ expiration, bool&/*out*/ watch_error);
	Cache_Item* get_touch(uint32_t group_id, const uint8_t* key, uint32_t key_length, uint32_t watch_id, uint32_t expiration, bool&/*out*/ watch_error);
	void release_reference(Cache_Item* item);
	bool get_base(uint32_t group_id, const uint8_t* key, uint32_t key_length, uint64_t&/*out*/ cache_id, uint32_t&/*out*/ flags, uint32_t&/*out*/ expiration);
	bool update_flags(uint32_t group_id, const uint8_t* key, uint32_t key_length, const XIXI_Update_Flags_Req_Pdu* pdu, uint64_t&/*out*/ cache_id);
	bool update_expiration(uint32_t group_id, const uint8_t* key, uint32_t key_length, const XIXI_Update_Expiration_Req_Pdu* pdu, uint64_t&/*out*/ cache_id);

	xixi_reason add(Cache_Item* item, uint32_t watch_id, uint64_t&/*out*/ cache_id);
	xixi_reason set(Cache_Item* item, uint32_t watch_id, uint64_t&/*out*/ cache_id);
	xixi_reason replace(Cache_Item* item, uint32_t watch_id, uint64_t&/*out*/ cache_id);
	xixi_reason append(Cache_Item* item, uint32_t watch_id, uint64_t&/*out*/ cache_id);
	xixi_reason prepend(Cache_Item* item, uint32_t watch_id, uint64_t&/*out*/ cache_id);

	xixi_reason remove(uint32_t group_id, const uint8_t* key, uint32_t key_length, uint64_t cache_id);
	xixi_reason delta(uint32_t group_id, const uint8_t* key, uint32_t key_length, bool incr, int64_t delta, uint64_t&/*in and out*/ cache_id, int64_t&/*out*/ value);
	bool item_size_ok(uint32_t key_length, uint32_t data_size);

	uint32_t create_watch(uint32_t group_id, uint32_t max_next_check_interval);
	bool check_watch_and_set_callback(uint32_t group_id, uint32_t watch_id, std::list<uint64_t>&/*out*/ updated_list, uint32_t&/*out*/ updated_count,
		uint64_t ack_cache_id, boost::shared_ptr<Cache_Watch_Sink>& sp, uint32_t max_next_check_interval);
	bool check_watch_and_clear_callback(uint32_t watch_id, std::list<uint64_t>&/*out*/ updated_list, uint32_t&/*out*/ updated_count);

	void check_expired();
	void stats(const XIXI_Stats_Req_Pdu* pdu, std::string& result);
	void print_stats();

	uint64_t get_mem_limit() {
		return mem_limit_;
	}
	uint64_t get_mem_used() {
		return mem_used_;
	}
private:
	inline void free_item(Cache_Item* it);
	inline uint64_t get_cache_id();
	inline Cache_Item* do_alloc(uint32_t group_id, uint32_t key_length, uint32_t flags, uint32_t expire_time, uint32_t data_size);
	inline void do_link(Cache_Item* it);
	inline void do_unlink(Cache_Item* it);
	inline void do_unlink_flush(Cache_Item* it);
	inline void do_release_reference(Cache_Item* it);
	inline void do_replace(Cache_Item* it, Cache_Item* new_it);
	inline Cache_Item* do_get(uint32_t group_id, const uint8_t* key, uint32_t key_length, uint32_t hash_value);
	inline Cache_Item* do_get(uint32_t group_id, const uint8_t* key, uint32_t key_length, uint32_t hash_value, uint32_t&/*out*/ expiration);
	inline Cache_Item* do_get_touch(uint32_t group_id, const uint8_t* key, uint32_t key_length, uint32_t hash_value, uint32_t expiration);
	inline uint32_t get_class_id(uint32_t size);
	inline uint32_t get_watch_id();
	inline bool is_valid_watch_id(uint32_t watch_id);
	void notify_watch(Cache_Item* it);

	inline uint32_t get_expiration_id(uint32_t curr_time, uint32_t expire_time);

	void expire_items(uint32_t curr_time);
	void expire_watchs(uint32_t curr_time);
	void free_flushed_items();

private:
	mutex cache_lock_;
	boost::thread_specific_ptr<int> tls_int_;

	xixi::hash_map<Cache_Key, Cache_Item> cache_hash_map_;

	xixi::list<Cache_Item> expire_list_[34];
	uint32_t expire_check_time_[33];
	uint32_t expiration_time_[33];

	uint32_t max_size_[CLASSID_MAX];
	uint32_t class_id_max_;
	uint32_t last_class_id_;
#ifdef USING_BOOST_POOL
	boost::pool<>* pools_[CLASSID_MAX];
#endif
	xixi::list<Cache_Item> free_cache_list_[CLASSID_MAX];
	xixi::list<Cache_Item> flush_cache_list_;

	uint64_t last_cache_id_;

	uint64_t mem_limit_;
	uint64_t mem_used_;

	uint32_t last_print_stats_time_;

	uint32_t last_check_expired_time_;
	uint32_t last_watch_id_;
	std::map<uint32_t, shared_ptr<Cache_Watch> > watch_map_;
};

extern Cache_Mgr cache_mgr_;

#endif // CACHE_H

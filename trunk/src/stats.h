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

#ifndef STATS_H
#define STATS_H

#include "defines.h"
#include "util.h"
#include "log.h"
#include "xixibase.h"
#include <boost/detail/atomic_count.hpp>
#include <boost/thread/mutex.hpp>

class Cache_Stats_Item {
public:
	Cache_Stats_Item() {
		clear();
	}
	void merge(Cache_Stats_Item& cs) const {
		cs.get_hit_no_watch += get_hit_no_watch;
		cs.get_hit_watch += get_hit_watch;
		cs.get_hit_watch_miss += get_hit_watch_miss;

		cs.get_touch_hit_no_watch += get_touch_hit_no_watch;
		cs.get_touch_hit_watch += get_touch_hit_watch;
		cs.get_touch_hit_watch_miss += get_touch_hit_watch_miss;

		cs.get_base_hit += get_base_hit;
		cs.get_base_miss += get_base_miss;

		cs.update_base_success += update_base_success;
		cs.update_base_mismatch += update_base_mismatch;

		cs.add_success += add_success;
		cs.add_success_watch += add_success_watch;
		cs.add_watch_miss += add_watch_miss;
		cs.add_fail += add_fail;

		cs.set_success += set_success;
		cs.set_success_watch += set_success_watch;
		cs.set_watch_miss += set_watch_miss;
		cs.set_mismatch += set_mismatch;

		cs.replace_success += replace_success;
		cs.replace_success_watch += replace_success_watch;
		cs.replace_watch_miss += replace_watch_miss;
		cs.replace_mismatch += replace_mismatch;

		cs.append_success += append_success;
		cs.append_success_watch += append_success_watch;
		cs.append_watch_miss += append_watch_miss;
		cs.append_mismatch += append_mismatch;
		cs.append_out_of_memory += append_out_of_memory;

		cs.prepend_success += prepend_success;
		cs.prepend_success_watch += prepend_success_watch;
		cs.prepend_watch_miss += prepend_watch_miss;
		cs.prepend_mismatch += prepend_mismatch;
		cs.prepend_out_of_memory += prepend_out_of_memory;

		cs.delete_success += delete_success;
		cs.delete_mismatch += delete_mismatch;
	}

	void clear() {
		get_hit_no_watch = 0;
		get_hit_watch = 0;
		get_hit_watch_miss = 0;

		get_touch_hit_no_watch = 0;
		get_touch_hit_watch = 0;
		get_touch_hit_watch_miss = 0;

		get_base_hit = 0;
		get_base_miss = 0;

		update_base_success = 0;
		update_base_mismatch = 0;

		add_success = 0;
		add_success_watch = 0;
		add_watch_miss = 0;
		add_fail = 0;

		set_success = 0;
		set_success_watch = 0;
		set_watch_miss = 0;
		set_mismatch = 0;

		replace_success = 0;
		replace_success_watch = 0;
		replace_watch_miss = 0;
		replace_mismatch = 0;

		append_success = 0;
		append_success_watch = 0;
		append_watch_miss = 0;
		append_mismatch = 0;
		append_out_of_memory = 0;

		prepend_success = 0;
		prepend_success_watch = 0;
		prepend_watch_miss = 0;
		prepend_mismatch = 0;
		prepend_out_of_memory = 0;

		delete_success = 0;
		delete_mismatch = 0;
	}

	void static append(uint32_t class_id, const char* k, uint32_t v, std::string& out);
	void static append(uint32_t class_id, const char* k, uint64_t v, std::string& out);

	void to_string(uint8_t class_id, std::string& out) {
		append(class_id, "get_hit_no_watch", get_hit_no_watch, out);
		append(class_id, "get_hit_watch", get_hit_watch, out);
		append(class_id, "get_hit_watch_miss", get_hit_watch_miss, out);

		append(class_id, "get_touch_hit_no_watch", get_touch_hit_no_watch, out);
		append(class_id, "get_touch_hit_watch", get_touch_hit_watch, out);
		append(class_id, "get_touch_hit_watch_miss", get_touch_hit_watch_miss, out);

		append(class_id, "get_base_hit", get_base_hit, out);
		append(class_id, "get_base_miss", get_base_miss, out);

		append(class_id, "update_base_success", update_base_success, out);
		append(class_id, "update_base_mismatch", update_base_mismatch, out);

		append(class_id, "add_success", add_success, out);
		append(class_id, "add_success_watch", add_success_watch, out);
		append(class_id, "add_watch_miss", add_watch_miss, out);
		append(class_id, "add_fail", add_fail, out);

		append(class_id, "set_success", set_success, out);
		append(class_id, "set_success_watch", set_success_watch, out);
		append(class_id, "set_watch_miss", set_watch_miss, out);
		append(class_id, "set_mismatch", set_mismatch, out);

		append(class_id, "replace_success", replace_success, out);
		append(class_id, "replace_success_watch", replace_success_watch, out);
		append(class_id, "replace_watch_miss", replace_watch_miss, out);
		append(class_id, "replace_mismatch", replace_mismatch, out);

		append(class_id, "append_success", append_success, out);
		append(class_id, "append_success_watch", append_success_watch, out);
		append(class_id, "append_watch_miss", append_watch_miss, out);
		append(class_id, "append_mismatch", append_mismatch, out);
		append(class_id, "append_out_of_memory", append_out_of_memory, out);

		append(class_id, "prepend_success", prepend_success, out);
		append(class_id, "prepend_success_watch", prepend_success_watch, out);
		append(class_id, "prepend_watch_miss", prepend_watch_miss, out);
		append(class_id, "prepend_mismatch", prepend_mismatch, out);
		append(class_id, "prepend_out_of_memory", prepend_out_of_memory, out);

		append(class_id, "delete_success", delete_success, out);
		append(class_id, "delete_mismatch", delete_mismatch, out);
	}

	uint64_t get_hit_no_watch;
	uint64_t get_hit_watch;
	uint64_t get_hit_watch_miss;

	uint64_t get_touch_hit_no_watch;
	uint64_t get_touch_hit_watch;
	uint64_t get_touch_hit_watch_miss;

	uint64_t get_base_hit;
	uint64_t get_base_miss;

	uint64_t update_base_success;
	uint64_t update_base_mismatch;

	uint64_t add_success;
	uint64_t add_success_watch;
	uint64_t add_watch_miss;
	uint64_t add_fail;

	uint64_t set_success;
	uint64_t set_success_watch;
	uint64_t set_watch_miss;
	uint64_t set_mismatch;

	uint64_t replace_success;
	uint64_t replace_success_watch;
	uint64_t replace_watch_miss;
	uint64_t replace_mismatch;

	uint64_t append_success;
	uint64_t append_success_watch;
	uint64_t append_watch_miss;
	uint64_t append_mismatch;
	uint64_t append_out_of_memory;

	uint64_t prepend_success;
	uint64_t prepend_success_watch;
	uint64_t prepend_watch_miss;
	uint64_t prepend_mismatch;
	uint64_t prepend_out_of_memory;

	uint64_t delete_success;
	uint64_t delete_mismatch;
};

class Group_Stats_Item {
public:
	Group_Stats_Item() {
		clear();
	}

	void clear() {
		get_miss_ = 0;
		get_touch_miss_ = 0;
		get_base_miss_ = 0;
		update_base_miss_ = 0;
		replace_miss_ = 0;
		append_miss_ = 0;
		prepend_miss_ = 0;
		delete_miss_ = 0;
		incr_success_ = 0;
		incr_mismatch_ = 0;
		incr_miss_ = 0;
		decr_success_ = 0;
		decr_mismatch_ = 0;
		decr_miss_ = 0;
		create_watch_ = 0;
		check_watch_ = 0;
		check_watch_miss_ = 0;

		bytes_read_ = 0;
		bytes_written_ = 0;

		link_items_ = 0;
		unlink_items_ = 0;

		link_bytes_ = 0;
		unlink_bytes_ = 0;

		flush_ = 0;

		for (int i = 0; i < 200; i++) {
			cache_stats_[i].clear();
		}
	}

	void static append(const char* k, uint32_t v, std::string& out);

	void static append(const char* k, uint64_t v, std::string& out);

	void static append(const char* k, const char* v, std::string& out);

	void to_string(uint8_t class_id, uint32_t max_class_id, std::string& out) {
		append("get_miss", get_miss_, out);
		append("get_touch_miss", get_touch_miss_, out);
		append("get_base_miss_", get_base_miss_, out);
		append("update_base_miss", update_base_miss_, out);
		append("replace_miss", replace_miss_, out);
		append("append_miss", append_miss_, out);
		append("prepend_miss", prepend_miss_, out);
		append("delete_miss", delete_miss_, out);
		append("incr_success", incr_success_, out);
		append("incr_mismatch", incr_mismatch_, out);
		append("incr_miss", incr_miss_, out);
		append("decr_success", decr_success_, out);
		append("decr_mismatch", decr_mismatch_, out);
		append("decr_miss", decr_miss_, out);
		append("create_watch", create_watch_, out);
		append("check_watch", check_watch_, out);
		append("check_watch_miss", check_watch_miss_, out);

		append("bytes_read", bytes_read_, out);
		append("bytes_written", bytes_written_, out);

		append("link_caches", link_items_, out);
		append("unlink_caches", unlink_items_, out);

		append("link_bytes", link_bytes_, out);
		append("unlink_bytes", unlink_bytes_, out);

		append("flush", flush_, out);

		if (class_id > 0 && class_id < 200) {
			cache_stats_[class_id].to_string(class_id, out);
		} else {
			for (uint32_t i = 1; i < max_class_id; i++) {
				cache_stats_[i].to_string(i, out);
			}
		}
	}

	uint64_t get_miss_;
	uint64_t get_touch_miss_;
	uint64_t get_base_miss_;
	uint64_t update_base_miss_;
	uint64_t replace_miss_;
	uint64_t append_miss_;
	uint64_t prepend_miss_;
	uint64_t delete_miss_;
	uint64_t incr_success_;
	uint64_t incr_mismatch_;
	uint64_t incr_miss_;
	uint64_t decr_success_;
	uint64_t decr_mismatch_;
	uint64_t decr_miss_;
	uint64_t create_watch_;
	uint64_t check_watch_;
	uint64_t check_watch_miss_;

	uint64_t bytes_read_;
	uint64_t bytes_written_;

	uint64_t link_items_;
	uint64_t unlink_items_;

	uint64_t link_bytes_;
	uint64_t unlink_bytes_;

	uint32_t flush_;
	Cache_Stats_Item cache_stats_[200];
};

class Stats {
public:
	Stats();
	~Stats();

	void print();

	bool add_group(uint32_t group_id);
	bool remove_group(uint32_t group_id);

	void get_base_stats(std::string& out);
	bool get_stats(uint32_t group_id, uint8_t class_id, std::string& out);
	bool get_and_clear_stats(uint32_t group_id, uint8_t class_id,  std::string& out);
	bool get_stats(uint8_t class_id, std::string& out);
	bool get_and_clear_stats(uint8_t class_id, std::string& out);

	inline Group_Stats_Item* get_group_item(uint32_t group_id) {
		std::map<uint32_t, Group_Stats_Item*>::iterator it = group_map_.find(group_id);
		if (it != group_map_.end()) {
			return it->second;
		}
		return NULL;
	}

	inline void get_hit_no_watch(uint32_t group_id, uint32_t class_id) {
		group_sum_.cache_stats_[class_id].get_hit_no_watch++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].get_hit_no_watch++;
		}
	}
	inline void get_hit_watch(uint32_t group_id, uint32_t class_id) {
		group_sum_.cache_stats_[class_id].get_hit_watch++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].get_hit_watch++;
		}
	}
	inline void get_hit_watch_miss(uint32_t group_id, uint32_t class_id) {
		group_sum_.cache_stats_[class_id].get_hit_watch_miss++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].get_hit_watch_miss++;
		}
	}
	inline void get_miss(uint32_t group_id) {
		group_sum_.get_miss_++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->get_miss_++;
		}
	}

	inline void get_touch_hit_no_watch(uint32_t group_id, uint32_t class_id) {
		group_sum_.cache_stats_[class_id].get_touch_hit_no_watch++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].get_touch_hit_no_watch++;
		}
	}
	inline void get_touch_hit_watch(uint32_t group_id, uint32_t class_id) {
		group_sum_.cache_stats_[class_id].get_touch_hit_watch++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].get_touch_hit_watch++;
		}
	}
	inline void get_touch_hit_watch_miss(uint32_t group_id, uint32_t class_id) {
		group_sum_.cache_stats_[class_id].get_touch_hit_watch_miss++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].get_touch_hit_watch_miss++;
		}
	}
	inline void get_touch_miss(uint32_t group_id) {
		group_sum_.get_touch_miss_++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->get_touch_miss_++;
		}
	}

	inline void get_base_hit(uint32_t group_id, uint32_t class_id) {
		group_sum_.cache_stats_[class_id].get_base_hit++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].get_base_hit++;
		}
	}
	inline void get_base_miss(uint32_t group_id) {
		group_sum_.get_base_miss_++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->get_base_miss_++;
		}
	}

	inline void update_base_success(uint32_t group_id, uint32_t class_id) {
		group_sum_.cache_stats_[class_id].update_base_success++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].update_base_success++;
		}
	}
	inline void update_base_mismatch(uint32_t group_id, uint32_t class_id) {
		group_sum_.cache_stats_[class_id].update_base_mismatch++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].update_base_mismatch++;
		}
	}
	inline void update_base_miss(uint32_t group_id) {
		group_sum_.update_base_miss_++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->update_base_miss_++;
		}
	}

	inline void add_success(uint32_t group_id, uint32_t class_id) {
		group_sum_.cache_stats_[class_id].add_success++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].add_success++;
		}
	}
	inline void add_fail(uint32_t group_id, uint32_t class_id) {
		group_sum_.cache_stats_[class_id].add_fail++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].add_fail++;
		}
	}

	inline void set_success(uint32_t group_id, uint32_t class_id) {
		group_sum_.cache_stats_[class_id].set_success++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].set_success++;
		}
	}
	inline void set_success_watch(uint32_t group_id, uint32_t class_id) {
		group_sum_.cache_stats_[class_id].set_success++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].set_success_watch++;
		}
	}
	inline void set_watch_miss(uint32_t group_id, uint32_t class_id) {
		group_sum_.cache_stats_[class_id].set_success++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].set_watch_miss++;
		}
	}
	inline void set_mismatch(uint32_t group_id, uint32_t class_id) {
		group_sum_.cache_stats_[class_id].set_mismatch++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].set_mismatch++;
		}
	}

	inline void replace_success(uint32_t group_id,uint32_t class_id) {
		group_sum_.cache_stats_[class_id].replace_success++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].replace_success++;
		}
	}
	inline void replace_success_watch(uint32_t group_id,uint32_t class_id) {
		group_sum_.cache_stats_[class_id].replace_success++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].replace_success_watch++;
		}
	}
	inline void replace_watch_miss(uint32_t group_id,uint32_t class_id) {
		group_sum_.cache_stats_[class_id].replace_success++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].replace_watch_miss++;
		}
	}
	inline void replace_mismatch(uint32_t group_id,uint32_t class_id) {
		group_sum_.cache_stats_[class_id].replace_mismatch++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].replace_mismatch++;
		}
	}
	inline void replace_miss(uint32_t group_id) {
		group_sum_.replace_miss_++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->replace_miss_++;
		}
	}

	inline void append_success(uint32_t group_id,uint32_t class_id) {
		group_sum_.cache_stats_[class_id].append_success++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].append_success++;
		}
	}
	inline void append_success_watch(uint32_t group_id,uint32_t class_id) {
		group_sum_.cache_stats_[class_id].append_success++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].append_success_watch++;
		}
	}
	inline void append_watch_miss(uint32_t group_id,uint32_t class_id) {
		group_sum_.cache_stats_[class_id].append_success++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].append_watch_miss++;
		}
	}
	inline void append_mismatch(uint32_t group_id,uint32_t class_id) {
		group_sum_.cache_stats_[class_id].append_mismatch++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].append_mismatch++;
		}
	}
	inline void append_out_of_memory(uint32_t group_id, uint32_t class_id) {
		group_sum_.cache_stats_[class_id].append_out_of_memory++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].append_out_of_memory++;
		}
	}
	inline void append_miss(uint32_t group_id) {
		group_sum_.append_miss_++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->append_miss_++;
		}
	}

	inline void prepend_success(uint32_t group_id,uint32_t class_id) {
		group_sum_.cache_stats_[class_id].prepend_success++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].prepend_success++;
		}
	}
	inline void prepend_success_watch(uint32_t group_id,uint32_t class_id) {
		group_sum_.cache_stats_[class_id].prepend_success++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].prepend_success_watch++;
		}
	}
	inline void prepend_watch_miss(uint32_t group_id,uint32_t class_id) {
		group_sum_.cache_stats_[class_id].prepend_success++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].prepend_watch_miss++;
		}
	}
	inline void prepend_mismatch(uint32_t group_id,uint32_t class_id) {
		group_sum_.cache_stats_[class_id].prepend_mismatch++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].prepend_mismatch++;
		}
	}
	inline void prepend_out_of_memory(uint32_t group_id, uint32_t class_id) {
		group_sum_.cache_stats_[class_id].prepend_out_of_memory++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].prepend_out_of_memory++;
		}
	}
	inline void prepend_miss(uint32_t group_id) {
		group_sum_.prepend_miss_++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->prepend_miss_++;
		}
	}  

	inline void delete_success(uint32_t group_id, uint32_t class_id) {
		group_sum_.cache_stats_[class_id].delete_success++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].delete_success++;
		}
	}

	inline void delete_mismatch(uint32_t group_id, uint32_t class_id) {
		group_sum_.cache_stats_[class_id].delete_mismatch++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->cache_stats_[class_id].delete_mismatch++;
		}
	}
	inline void delete_miss(uint32_t group_id) {
		group_sum_.delete_miss_++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->delete_miss_++;
		}
	}

	inline void incr_success(uint32_t group_id) {
		group_sum_.incr_success_++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->incr_success_++;
		}
	}
	inline void incr_mismatch(uint32_t group_id) {
		group_sum_.incr_mismatch_++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->incr_mismatch_++;
		}
	}
	inline void incr_miss(uint32_t group_id) {
		group_sum_.incr_miss_++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->incr_miss_++;
		}
	}

	inline void decr_success(uint32_t group_id) {
		group_sum_.decr_success_++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->decr_success_++;
		}
	}
	inline void decr_mismatch(uint32_t group_id) {
		group_sum_.decr_mismatch_++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->decr_mismatch_++;
		}
	}
	inline void decr_miss(uint32_t group_id) {
		group_sum_.decr_miss_++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->decr_miss_++;
		}
	}

	inline void create_watch(uint32_t group_id) {
		group_sum_.create_watch_++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->create_watch_++;
		}
	}
	inline void check_watch(uint32_t group_id) {
		group_sum_.check_watch_++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->check_watch_++;
		}
	}
	inline void check_watch_miss(uint32_t group_id) {
		group_sum_.check_watch_miss_++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->check_watch_miss_++;
		}
	}

	inline void stat_bytes_read(uint32_t group_id, uint32_t bytes) {
		group_sum_.bytes_read_ += bytes;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->bytes_read_ += bytes;
		}
	}
	inline void stat_bytes_write(uint32_t group_id, uint32_t bytes) {
		group_sum_.bytes_written_ += bytes;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->bytes_written_ += bytes;
		}
	}

	inline void flush(uint32_t group_id) {
		group_sum_.flush_++;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->flush_++;
		}
	}

	inline void item_link(uint32_t group_id, uint32_t class_id, uint32_t size) {
		group_sum_.link_items_++;
		group_sum_.link_bytes_ += size;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->link_items_++;
			item->link_bytes_ += size;
		}
	}
	inline void item_unlink(uint32_t group_id, uint32_t class_id, uint32_t size) {
		group_sum_.unlink_items_++;
		group_sum_.unlink_bytes_ += size;

		Group_Stats_Item* item = get_group_item(group_id);
		if (item != NULL) {
			item->unlink_items_++;
			item->link_bytes_ += size;
		}
	}

	inline void new_conn() {
		lock_.lock();
		curr_conns_++;
		total_conns_++;
		lock_.unlock();
	}

	inline void close_conn() {
		lock_.lock();
		curr_conns_--;
		lock_.unlock();
	}

	void set_max_class_id(uint32_t max_class_id) {
		max_class_id_ = max_class_id;
	}

private:
	void merage(Cache_Stats_Item& cache_stat);

public:
	mutex lock_;

	uint32_t max_class_id_;
	uint32_t curr_conns_;
	uint64_t total_conns_;

	Group_Stats_Item group_sum_;
	std::map<uint32_t, Group_Stats_Item*> group_map_;
};

extern Stats stats_;

#endif // STATS_H

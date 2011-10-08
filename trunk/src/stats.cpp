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

#include <boost/lexical_cast.hpp>
#include "stats.h"
#include "settings.h"
#include "currtime.h"
#include "cache.h"

Stats stats_;

void Cache_Stats_Item::append(uint32_t class_id, const char* k, uint32_t v, std::string& out) {
  std::string c = boost::lexical_cast<std::string>(class_id);
  std::string t = boost::lexical_cast<std::string>(v);
  out += k + c + "=" + t + "\n";
}

void Cache_Stats_Item::append(uint32_t class_id, const char* k, uint64_t v, std::string& out) {
  std::string c = boost::lexical_cast<std::string>(class_id);
  std::string t = boost::lexical_cast<std::string>(v);
  out += k + c + "=" + t + "\n";
}

void Group_Stats_Item::append(const char* k, uint32_t v, std::string& out) {
  std::string t = boost::lexical_cast<std::string>(v);
  out += k + string("=") + t + "\n";
}

void Group_Stats_Item::append(const char* k, uint64_t v, std::string& out) {
  std::string t = boost::lexical_cast<std::string>(v);
  out += k + string("=") + t + "\n";
}

void Group_Stats_Item::append(const char* k, const char* v, std::string& out) {
  out += k + string("=") + v + "\n";
}

Stats::Stats() {
  max_class_id_ = 200;
  curr_conns_ = 0;
  total_conns_ = 0;
}

Stats::~Stats() {
  std::map<uint32_t, Group_Stats_Item*>::iterator it = group_map_.begin();
  while (it != group_map_.end()) {
    delete it->second;
    ++it;
  }
  group_map_.clear();
}

void Stats::merage(Cache_Stats_Item& cache_stat) {
  for (uint32_t i = 1; i < max_class_id_; i++) {
    group_sum_.cache_stats_[i].merge(cache_stat);
  }
}

void Stats::print() {
  Cache_Stats_Item cs;
  merage(cs);
  uint64_t mem_free = cache_mgr_.get_mem_limit() - cache_mgr_.get_mem_used();
  double mem_free_rate = ((double)mem_free) / (double)cache_mgr_.get_mem_limit();
  mem_free_rate = ((double)((uint64_t)(mem_free_rate * 10000))) / 10000;
  LOG_INFO("conn=" << curr_conns_ << "/" << total_conns_ << " memory=" << cache_mgr_.get_mem_used() << "/" << cache_mgr_.get_mem_limit()
    << ":" << mem_free << "/" << mem_free_rate);
  LOG_INFO("item=" << (group_sum_.link_items_ - group_sum_.unlink_items_) << "/" << (group_sum_.link_bytes_ - group_sum_.unlink_bytes_)
    << " link=" << group_sum_.link_items_ << "/" << group_sum_.link_bytes_
    << " unlink=" << group_sum_.unlink_items_ << "/" << group_sum_.unlink_bytes_);
  LOG_INFO("get hit/w=" << cs.get_hit_no_watch << "/" << cs.get_hit_watch << " miss=" << group_sum_.get_miss_ << " w_miss=" << cs.get_hit_watch_miss);
  LOG_INFO("get_touch hit/w=" << cs.get_touch_hit_no_watch << "/" << cs.get_touch_hit_watch << " miss=" << group_sum_.get_touch_miss_ << " w_miss=" << cs.get_touch_hit_watch_miss);
  LOG_INFO("set success=" << cs.set_success << " mismatch=" << cs.set_mismatch);
//  LOG_INFO("get_base hit=" << cs.get_base_hit << " miss=" << group_sum_.get_base_miss_);
//  LOG_INFO("update_base success=" << cs.update_base_success << " miss=" << group_sum_.update_base_miss_ << " mismatch=" << cs.update_base_mismatch);
}

bool Stats::add_group(uint32_t group_id) {
  if (group_map_.size() < settings_.max_stats_group){
    Group_Stats_Item* item = new Group_Stats_Item();
    if (item != NULL) {
      group_map_.insert(make_pair<uint32_t, Group_Stats_Item*>(group_id, item));
      return true;
    }
  }
  return false;
}

bool Stats::remove_group(uint32_t group_id) {
  std::map<uint32_t, Group_Stats_Item*>::iterator it = group_map_.find(group_id);
  if (it != group_map_.end()) {
    delete it->second;
    group_map_.erase(it);
    return true;
  }
  return false;
}

void Stats::get_base_stats(std::string& out) {
  Group_Stats_Item::append("version", VERSION, out);
  Group_Stats_Item::append("uptime", curr_time_.get_current_time(), out);
  Group_Stats_Item::append("maxbytes", settings_.maxbytes, out);
  Group_Stats_Item::append("threads", settings_.num_threads, out);
  Group_Stats_Item::append("max_stats_group", settings_.max_stats_group, out);
  Group_Stats_Item::append("curr_stats_group", group_map_.size(), out);
  Group_Stats_Item::append("memory_limit", cache_mgr_.get_mem_limit(), out);
  Group_Stats_Item::append("memory_used", cache_mgr_.get_mem_used(), out);
  Group_Stats_Item::append("curr_stats_group", group_map_.size(), out);
  lock_.lock();
  Group_Stats_Item::append("curr_conns", curr_conns_, out);
  Group_Stats_Item::append("total_conns", total_conns_, out);
  lock_.unlock();
}

bool Stats::get_stats(uint32_t group_id, uint8_t class_id, std::string& out) {
  get_base_stats(out);
  Group_Stats_Item* item = get_group_item(group_id);
  if (item != NULL) {
    item->to_string(class_id, max_class_id_, out);
    return true;
  }
  return false;
}

bool Stats::get_and_clear_stats(uint32_t group_id, uint8_t class_id,  std::string& out) {
  get_base_stats(out);
  Group_Stats_Item* item = get_group_item(group_id);
  if (item != NULL) {
    item->to_string(class_id, max_class_id_, out);
    item->clear();
    return true;
  }
  return false;
}

bool Stats::get_stats(uint8_t class_id, std::string& out) {
  get_base_stats(out);
  group_sum_.to_string(class_id, max_class_id_, out);
  return true;
}

bool Stats::get_and_clear_stats(uint8_t class_id, std::string& out) {
  get_base_stats(out);
  group_sum_.to_string(class_id, max_class_id_, out);
  group_sum_.clear();
  return true;
}

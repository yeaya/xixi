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

#ifndef CONCURRENT_H
#define CONCURRENT_H

#include "defines.h"
#include "util.h"
#include "peer.h"
#include "peer_cc_pdu.h"
#include <boost/pool/pool.hpp>
//#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/tss.hpp>
#include "xixi_hash_map.hpp"
#include "xixi_list.hpp"

////////////////////////////////////////////////////////////////////////////////
// ccc
//
class ccc : public xixi::list_node_base<ccc>, public xixi::hash_node_base<Const_Data, ccc> {
public:
  ccc(const XIXI_Mutex_Lock_Req_Pdu* pdu, uint32_t request_time, uint64_t sequence) {
    this->request_id = pdu->request_id;
    this->request_time = request_time;
    this->sequence = sequence;
    session_id.size = pdu->session_id.size;
    uint8_t* p = new uint8_t[session_id.size];
    if (p != NULL) {
      memcpy(p, pdu->session_id.data, session_id.size);
      session_id.data = p;
    } else {
      session_id.size = 0;
    }
  }
  ~ccc() {
    if (session_id.data != NULL) {
      delete[] session_id.data;
    }
  }
  inline bool is_key(const Const_Data* p) const {
    return p != NULL && session_id.size == p->size && memcmp(session_id.data, p->data, session_id.size) == 0;
  }

  void clear() {
    request_id = 0;
    request_time = 0;
    sequence = 0;
  }

  uint32_t request_id;
  uint32_t request_time;
  uint64_t sequence;
  Const_Data session_id;
};

enum mutex_status {
  MUTEX_STATUS_READY = 0,
  MUTEX_STATUS_SHARED_LOCKED,
  MUTEX_STATUS_LOCKED
};

class cc_item : public xixi::hash_node_base<Const_Data, cc_item> {
public:
  cc_item();
  ~cc_item();

  inline bool is_key(const Const_Data* p) const { return (key.size == p->size) && (memcmp(key.data, p->data, key.size) == 0); }

  xixi_reason mutex_create(const uint8_t* key, uint32_t key_length);
  void mutex_get_status(const XIXI_Mutex_Get_Status_Req_Pdu* pdu, XIXI_Mutex_Get_Status_Res_Pdu* res_pdu);
  xixi_reason mutex_lock(const XIXI_Mutex_Lock_Req_Pdu* pdu, uint64_t sequence);
  xixi_reason mutex_unlock(const XIXI_Mutex_Unlock_Req_Pdu* pdu);
  xixi_reason mutex_destroy();

  void notify_unlock();
  void dispatch_message(ccc* c);

  const char* status4string() {
    const char* status_str[] = {"READY", "SHARED_LOCKED", "LOCKED"};
    return status_str[status_];
  }
  Simple_Data key;
  mutex_status status_;
  xixi::list<ccc> lock_list_;
  xixi::hash_map<Const_Data, ccc> lock_map_;
  xixi::list<ccc> lock_shared_list_;
  xixi::hash_map<Const_Data, ccc> lock_shared_map_;
  xixi::hash_map<Const_Data, ccc> lock_shared_map_wait_;
};

class MutexMgr {
public:
  MutexMgr();
  ~MutexMgr();

  void init();

  xixi_reason mutex_create(const XIXI_Mutex_Create_Req_Pdu* pdu);
  void mutex_get_status(const XIXI_Mutex_Get_Status_Req_Pdu* pdu, XIXI_Mutex_Get_Status_Res_Pdu* res_pdu);
  xixi_reason mutex_lock(const XIXI_Mutex_Lock_Req_Pdu* pdu);
  xixi_reason mutex_unlock(const XIXI_Mutex_Unlock_Req_Pdu* pdu);
  xixi_reason mutex_destroy(const XIXI_Mutex_Destroy_Req_Pdu* pdu);

private:
  mutex lock_;
  boost::thread_specific_ptr<int> tls_int_;

  xixi::hash_map<Const_Data, cc_item> mutex_hash_map_;

  uint64_t sequence_;
};

extern MutexMgr mutex_mgr_;

#endif // CONCURRENT_H


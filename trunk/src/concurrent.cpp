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

#include "concurrent.h"
#include <assert.h>
#include "currtime.h"
#include "util.h"
#include "log.h"
#include "hash.h"
#include "session.h"

MutexMgr mutex_mgr_;
#define LOG_INFO2(x)  LOG_INFO(x)

cc_item::cc_item() {
  status_ = MUTEX_STATUS_READY;
}

cc_item::~cc_item() {
}

xixi_reason cc_item::mutex_create(const uint8_t* key, uint32_t key_length) {
  this->key.set(key, key_length);
  return XIXI_REASON_SUCCESS;
}

void cc_item::mutex_get_status(const XIXI_Mutex_Get_Status_Req_Pdu* pdu, XIXI_Mutex_Get_Status_Res_Pdu* res_pdu) {
  LOG_INFO2("mutex_get_status session=" << string((char*)pdu->session_id.data, pdu->session_id.size) << " status=" << status4string());

  uint32_t hash_value = pdu->session_id.hash_value();
  ccc* c = lock_map_.find(&pdu->session_id, pdu->session_id.hash_value());
  if (c != NULL) {
    if (c == lock_list_.front()) {
      res_pdu->own_lock = true;
    } else {
      res_pdu->wait_lock = true;
    }
  }

  c = lock_shared_map_.find(&pdu->session_id, hash_value);
  if (c != NULL) {
    res_pdu->own_shared_lock = true;
  } else {
    c = lock_shared_map_wait_.find(&pdu->session_id, hash_value);
    if (c != NULL) {
      res_pdu->wait_shared_lock = true;
    }
  }

  res_pdu->reason = XIXI_REASON_SUCCESS;

  LOG_INFO2("mutex_get_status session=" << string((char*)pdu->session_id.data, pdu->session_id.size)
    << " status=" << status4string());
}

xixi_reason cc_item::mutex_lock(const XIXI_Mutex_Lock_Req_Pdu* pdu, uint64_t sequence) {
  LOG_INFO2("mutex_lock session=" << string((char*)pdu->session_id.data, pdu->session_id.size) << " status=" << status4string());
  xixi_reason result;

  if (pdu->try_lock) {
    switch (status_) {
    case MUTEX_STATUS_READY:
      {
        status_ = MUTEX_STATUS_LOCKED;
        result = XIXI_REASON_SUCCESS;
      }
      break;
    case MUTEX_STATUS_SHARED_LOCKED:
    case MUTEX_STATUS_LOCKED:
      {
        result = XIXI_REASON_WAIT_FOR_ME;
      }
      break;
    }
  }

  if (!pdu->shared) {
    uint32_t hash_value = hash32(pdu->session_id.data, pdu->session_id.size, 0);
    ccc* c = lock_map_.find(&pdu->session_id, hash_value);
    if (c != NULL) {
      if (c == lock_list_.front() && status_ == MUTEX_STATUS_LOCKED) {
        result = XIXI_REASON_EXISTS;
      } else {
        if (pdu->try_lock) {
          result = XIXI_REASON_PLEASE_TRY_AGAIN;
        } else {
          result = XIXI_REASON_WAIT_FOR_ME;//XIXI_REASON_INVALID_OPERATION;
        }
      }
    } else {
      switch (status_) {
      case MUTEX_STATUS_READY: {
          status_ = MUTEX_STATUS_LOCKED;
          result = XIXI_REASON_SUCCESS;
        }
        break;
      case MUTEX_STATUS_SHARED_LOCKED:
      case MUTEX_STATUS_LOCKED: {
          if (pdu->try_lock) {
            result = XIXI_REASON_PLEASE_TRY_AGAIN;
          } else {
            result = XIXI_REASON_WAIT_FOR_ME;
          }
        }
        break;
      }
      if (result != XIXI_REASON_PLEASE_TRY_AGAIN) {
        c = new ccc(pdu, curr_time_.get_current_time(), sequence);
        lock_map_.insert(c, hash_value);
        lock_list_.push_back(c);
      }
    }
  } else {
    uint32_t hash_value = hash32(pdu->session_id.data, pdu->session_id.size, 0);
    ccc* c = lock_shared_map_.find(&pdu->session_id, hash_value);
    if (c != NULL) {
      if (status_ == MUTEX_STATUS_SHARED_LOCKED) {
        result = XIXI_REASON_EXISTS;
      } else {
        if (pdu->try_lock) {
          result = XIXI_REASON_PLEASE_TRY_AGAIN;
        } else {
          result = XIXI_REASON_WAIT_FOR_ME;//XIXI_REASON_INVALID_OPERATION;
        }
      }
    } else {
      c = lock_shared_map_wait_.find(&pdu->session_id, hash_value);
      if (c != NULL) {
        if (pdu->try_lock) {
          result = XIXI_REASON_PLEASE_TRY_AGAIN;
        } else {
          result = XIXI_REASON_WAIT_FOR_ME;//XIXI_REASON_INVALID_OPERATION;
        }
      }
    }
    if (c == NULL) {
      switch (status_) {
      case MUTEX_STATUS_READY: {
          status_ = MUTEX_STATUS_SHARED_LOCKED;
          result = XIXI_REASON_SUCCESS;
          c = new ccc(pdu, curr_time_.get_current_time(), sequence);
          lock_shared_list_.push_back(c);
          lock_shared_map_.insert(c, hash_value);
        }
        break;
      case MUTEX_STATUS_SHARED_LOCKED: {
          if (lock_list_.empty()) {
            result = XIXI_REASON_SUCCESS;
            c = new ccc(pdu, curr_time_.get_current_time(), sequence);
            lock_shared_list_.push_back(c);
            lock_shared_map_.insert(c, hash_value);
          } else {
            if (pdu->try_lock) {
              result = XIXI_REASON_PLEASE_TRY_AGAIN;
            } else {
              result = XIXI_REASON_WAIT_FOR_ME;
              c = new ccc(pdu, curr_time_.get_current_time(), sequence);
              lock_shared_list_.push_back(c);
              lock_shared_map_wait_.insert(c, hash_value);
            }
          }
        }
        break;
      case MUTEX_STATUS_LOCKED: {
          if (pdu->try_lock) {
            result = XIXI_REASON_PLEASE_TRY_AGAIN;
          } else {
            result = XIXI_REASON_WAIT_FOR_ME;
            c = new ccc(pdu, curr_time_.get_current_time(), sequence);
            lock_shared_list_.push_back(c);
            lock_shared_map_wait_.insert(c, hash_value);
          }
        }
        break;
      }
    }
  }
  LOG_INFO2("mutex_lock session=" << string((char*)pdu->session_id.data, pdu->session_id.size) << " shared=" << pdu->shared
    << " status=" << status4string() << " result=" << result);
  return result;
}

xixi_reason cc_item::mutex_unlock(const XIXI_Mutex_Unlock_Req_Pdu* pdu) {
  LOG_INFO2("mutex_unlock session=" << string((char*)pdu->session_id.data, pdu->session_id.size) << " shared=" << pdu->shared << " status=" << status4string());
  xixi_reason result;
  if (!pdu->shared) {
    uint32_t hash_value = hash32(pdu->session_id.data, pdu->session_id.size, 0);
    ccc* c = lock_map_.find(&pdu->session_id, hash_value);
    if (c != NULL) {
      switch (status_) {
      case MUTEX_STATUS_READY:
      case MUTEX_STATUS_SHARED_LOCKED: {
          result = XIXI_REASON_INVALID_OPERATION;
        }
        break;
      case MUTEX_STATUS_LOCKED: {
          if (!lock_list_.empty()) {
            if (c == lock_list_.front()) {
              result = XIXI_REASON_SUCCESS;
              lock_list_.pop_front();
              lock_map_.remove(c);
              delete c;
              c = NULL;
              status_ = MUTEX_STATUS_READY;
              notify_unlock();
            } else {
              result = XIXI_REASON_INVALID_OPERATION;
            }
          } else {
            result = XIXI_REASON_INVALID_OPERATION;
          }
        }
        break;
      }
    } else {
      result = XIXI_REASON_NOT_FOUND;
    }
  } else { // shared
    switch (status_) {
    case MUTEX_STATUS_READY:
    case MUTEX_STATUS_LOCKED: {
        result = XIXI_REASON_INVALID_OPERATION;
      }
      break;
    case MUTEX_STATUS_SHARED_LOCKED: {
        uint32_t hash_value = hash32(pdu->session_id.data, pdu->session_id.size, 0);
        ccc* c = lock_shared_map_.remove(&pdu->session_id, hash_value);
        if (c != NULL) {
          lock_shared_list_.remove(c);
          delete c;
          c = NULL;
          result = XIXI_REASON_SUCCESS;
          if (lock_shared_map_.empty()) {
            status_ = MUTEX_STATUS_READY;
            notify_unlock();
          }
        } else {
          result = XIXI_REASON_NOT_FOUND;
        }
      }
      break;
    }
  }

  LOG_INFO2("mutex_unlock session=" << string((char*)pdu->session_id.data, pdu->session_id.size) << " shared=" << pdu->shared
    << " status=" << status4string() << " result=" << result);
  return result;
}

xixi_reason cc_item::mutex_destroy() {
  return XIXI_REASON_SUCCESS;
}

void cc_item::dispatch_message(ccc* c) {
  LOG_INFO2("dispatch_message session_id=" << string((char*)c->session_id.data, c->session_id.size) << " status=" << status4string());
  XIXI_Mutex_Lock_Res_Pdu* pdu = new XIXI_Mutex_Lock_Res_Pdu();
  pdu->request_id = c->request_id;
  pdu->reason = XIXI_REASON_SUCCESS;

  session_mgr_.dispatch_message(&c->session_id, pdu);
}

void cc_item::notify_unlock() {
  LOG_INFO2("notify_unlock");

  ccc* c = lock_list_.front();
  ccc* cs = lock_shared_list_.front();
  if (c != NULL) {
    if (cs != NULL) {
      if (c->sequence < cs->sequence) {
        status_ = MUTEX_STATUS_LOCKED;
        dispatch_message(c);
      } else {
        status_ = MUTEX_STATUS_SHARED_LOCKED;
        while (cs != NULL && cs->sequence <= c->sequence) {
          lock_shared_map_wait_.remove(cs);
          lock_shared_map_.insert(cs, cs->hash_value_);

//          cs->sink->on_locked(cs->request_id, XIXI_REASON_SUCCESS);
          dispatch_message(cs);
          cs = lock_shared_list_.next(cs);
        }
      }
    } else {
      status_ = MUTEX_STATUS_LOCKED;
    //  c->sink->on_locked(c->request_id, XIXI_REASON_SUCCESS);
      dispatch_message(c);
    }
  } else if (cs != NULL) {
    status_ = MUTEX_STATUS_SHARED_LOCKED;
    while (cs != NULL) {
      lock_shared_map_wait_.remove(cs);
      lock_shared_map_.insert(cs, cs->hash_value_);

//      cs->sink->on_locked(cs->request_id, XIXI_REASON_SUCCESS);
      dispatch_message(cs);
      cs = lock_shared_list_.next(cs);
    }
  }
}

MutexMgr::MutexMgr() {
  sequence_ = 0;
}

MutexMgr::~MutexMgr() {

}

void MutexMgr::init() {

}

xixi_reason MutexMgr::mutex_create(const XIXI_Mutex_Create_Req_Pdu* pdu) {
  xixi_reason result;
  uint32_t hash_value = pdu->key.hash_value();
  lock_.lock();
  cc_item* it = mutex_hash_map_.find(&pdu->key, hash_value);
  if (it != NULL) {
    result = XIXI_REASON_EXISTS;
  } else {
    it = new cc_item();
    it->mutex_create(pdu->key.data, pdu->key.size);
    mutex_hash_map_.insert(it, hash_value);
    result = XIXI_REASON_SUCCESS;
  }
  lock_.unlock();
  return result;
}

void MutexMgr::mutex_get_status(const XIXI_Mutex_Get_Status_Req_Pdu* pdu, XIXI_Mutex_Get_Status_Res_Pdu* res_pdu) {
  uint32_t hash_value = pdu->key.hash_value();
  lock_.lock();
  cc_item* it = mutex_hash_map_.find(&pdu->key, hash_value);
  if (it != NULL) {
    it->mutex_get_status(pdu, res_pdu);
  } else {
    res_pdu->reason = XIXI_REASON_NOT_FOUND;
  }
  lock_.unlock();
}

xixi_reason MutexMgr::mutex_lock(const XIXI_Mutex_Lock_Req_Pdu* pdu) {
  xixi_reason result;
  uint32_t hash_value = hash32(pdu->key.data, pdu->key.size, 0);
  lock_.lock();
  cc_item* it = mutex_hash_map_.find(&pdu->key, hash_value);
  if (it == NULL && pdu->create) {
    it = new cc_item();
    it->mutex_create(pdu->key.data, pdu->key.size);
    mutex_hash_map_.insert(it, hash_value);
  }
  if (it != NULL) {
    result = it->mutex_lock(pdu, sequence_++);
  } else {
    result = XIXI_REASON_NOT_FOUND;
  }
  lock_.unlock();
  return result;
}

xixi_reason MutexMgr::mutex_unlock(const XIXI_Mutex_Unlock_Req_Pdu* pdu) {
  xixi_reason result;
  uint32_t hash_value = hash32(pdu->key.data, pdu->key.size, 0);
  lock_.lock();
  cc_item* it = mutex_hash_map_.find(&pdu->key, hash_value);
  if (it != NULL) {
    result = it->mutex_unlock(pdu);
  } else {
    result = XIXI_REASON_NOT_FOUND;
  }
  lock_.unlock();
  return result;
}

xixi_reason MutexMgr::mutex_destroy(const XIXI_Mutex_Destroy_Req_Pdu* pdu) {
  xixi_reason result;
  uint32_t hash_value = pdu->key.hash_value();
  lock_.lock();
  cc_item* it = mutex_hash_map_.find(&pdu->key, hash_value);
  if (it != NULL) {
    result = it->mutex_destroy();
    mutex_hash_map_.remove(it);
    delete it;
  } else {
    result = XIXI_REASON_NOT_FOUND;
  }
  lock_.unlock();
  return result;
}

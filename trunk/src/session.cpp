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

#include "session.h"
#include "settings.h"
#include "currtime.h"
#include <string.h>
#include "log.h"
#include "concurrent.h"

#define LOG_TRACE2(x)   LOG_TRACE("id=" << get_peer_id() << " " << x)
#define LOG_DEBUG2(x)   LOG_DEBUG("id=" << get_peer_id() << " " << x)
#define LOG_INFO2(x)    LOG_INFO("id=" << get_peer_id() << " " << x)
#define LOG_WARNING2(x) LOG_WARNING("id=" << get_peer_id() << " " << x)
#define LOG_ERROR2(x)   LOG_ERROR("id=" << get_peer_id() << " " << x)

Session_Mgr session_mgr_;

Session::Session() {
  LOG_INFO("Session::Session()");
}

Session::~Session() {
  LOG_INFO("Session::~Session()");
}

void Session::init(const Const_Data* session_id, boost::shared_ptr<Session_Sink>& sp) {
  LOG_INFO("Session::init()" << string((char*)session_id->data, session_id->size));
  wp_ = sp;
  session_id_.set(session_id);
}

Session_Sink* Session::get_peer() {
//  return peer_;
  return NULL;
}

void Session::update_peer(boost::shared_ptr<Session_Sink>& sp) {
  wp_ = sp;
}

void Session::dispatch_message(XIXI_Pdu* msg) {
//  LOG_INFO("Session::dispatch_message()");
  boost::shared_ptr<Session_Sink> sp = wp_.lock();
  if (sp != NULL) {
    sp->dispatch_message(msg);
  }
//  if (peer_ != NULL) {
//    peer_->dispatch_message(msg);
//  }
}

void Session_Mgr::attach_peer(const Const_Data* session_id, boost::shared_ptr<Session_Sink>& sp) {
//  LOG_INFO("Session_Mgr::attach_peer() " << string((char*)session_id->data, session_id->size));
  session_node* sn;
  lock_.lock();
  sn = sessions_.find(session_id, session_id->hash_value());
  if (sn == NULL) {
    sn = new session_node();
    sessions_.insert(sn, session_id->hash_value());
    sn->init(session_id, sp);
  } else {
    if (sn->get_peer() != sp.get()) {
      sn->update_peer(sp);
    }
  }
  std::map<Session_Sink*, set<session_node*> >::iterator it = peers_.find(sp.get());
  if (it != peers_.end()) {
    std::set<session_node*>& ss = it->second;
    std::set<session_node*>::iterator its = ss.find(sn);
    if (its == ss.end()) {
      ss.insert(sn);
    }
    ss.clear();
    peers_.erase(it);
  } else {
    std::set<session_node*> ss;
    ss.insert(sn);
    peers_.insert(std::pair<Session_Sink*, std::set<session_node*> >(sp.get(), ss));
  }
  lock_.unlock();
}

void Session_Mgr::detach_peer(Session_Sink* peer) {
  LOG_INFO("Session_Mgr::detach_peer()");
  lock_.lock();
  std::map<Session_Sink*, set<session_node*> >::iterator it = peers_.find(peer);
  if (it != peers_.end()) {
    std::set<session_node*>& ss = it->second;
    std::set<session_node*>::iterator its = ss.begin();
    while (its != ss.end()) {
      boost::shared_ptr<Session_Sink> sp;
      (*its)->update_peer(sp);
      ++its;
    }
    ss.clear();
    peers_.erase(it);
  }
  lock_.unlock();
}

void Session_Mgr::dispatch_message(const Const_Data* session_id, XIXI_Pdu* msg) {
//  LOG_INFO("Session_Mgr::dispatch_message()");
  lock_.lock();
  session_node* sn = sessions_.find(session_id, session_id->hash_value());
  if (sn != NULL) {
    sn->dispatch_message(msg);
  } else {
    delete msg;
  }
  lock_.unlock();
}

xixi_reason Session_Mgr::mutex_create(const XIXI_Mutex_Create_Req_Pdu* pdu) {
  return mutex_mgr_.mutex_create(pdu);
}

void Session_Mgr::mutex_get_status(const XIXI_Mutex_Get_Status_Req_Pdu* pdu, XIXI_Mutex_Get_Status_Res_Pdu* res_pdu) {
  mutex_mgr_.mutex_get_status(pdu, res_pdu);
}

xixi_reason Session_Mgr::mutex_lock(boost::shared_ptr<Session_Sink>& sp, const XIXI_Mutex_Lock_Req_Pdu* pdu) {
  attach_peer(&pdu->session_id, sp);
  return mutex_mgr_.mutex_lock(pdu);
}

xixi_reason Session_Mgr::mutex_unlock(const XIXI_Mutex_Unlock_Req_Pdu* pdu) {
  return mutex_mgr_.mutex_unlock(pdu);
}

xixi_reason Session_Mgr::mutex_destroy(const XIXI_Mutex_Destroy_Req_Pdu* pdu) {
  return mutex_mgr_.mutex_destroy(pdu);
}

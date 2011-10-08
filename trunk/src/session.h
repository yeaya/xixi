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

#ifndef SESSION_H
#define SESSION_H

#include "defines.h"
#include "util.h"
//#include "peer.h"
#include "xixi_hash_map.hpp"
#include "peer_cc_pdu.h"

#include <boost/thread/mutex.hpp>
#include <boost/smart_ptr/weak_ptr.hpp>

#ifndef USING_64_PEER_ID
typedef uint32_t PEER_ID;
#else
typedef uint64_t PEER_ID;
#endif

class Session_Mgr;

////////////////////////////////////////////////////////////////////////////////
// Session_Sink
//
class Session_Sink {
public:
  virtual void dispatch_message(XIXI_Pdu* msg) = 0;
};

////////////////////////////////////////////////////////////////////////////////
// Session
//
class Session {
  friend class Session_Mgr;
  friend struct Session_PK;
public:
  Session();
  ~Session();

  void init(const Const_Data* session_id, boost::shared_ptr<Session_Sink>& sp);

  Session_Sink* get_peer();

  void update_peer(boost::shared_ptr<Session_Sink>& sp);

  void dispatch_message(XIXI_Pdu* msg);

protected:
  boost::weak_ptr<Session_Sink> wp_;
//  Session_Sink* peer_;
  Simple_Data session_id_;
};

struct Session_PK {
  bool inline is_key(const Const_Data* k, const Session* t) const { return t->session_id_.equal(k); }
};

typedef xixi::hash_node_ext<Const_Data, Session> session_node;

////////////////////////////////////////////////////////////////////////////////
// Session_Mgr
//
class Session_Mgr {
public:
  Session_Mgr() {
  }

  void attach_peer(const Const_Data* session_id, boost::shared_ptr<Session_Sink>& sp);

  void detach_peer(Session_Sink* peer);

  void dispatch_message(const Const_Data* session_id, XIXI_Pdu* msg);

  void dispatch_message_2_backup(XIXI_Pdu* msg);

  // mutex
  xixi_reason mutex_create(const XIXI_Mutex_Create_Req_Pdu* pdu);
  void mutex_get_status(const XIXI_Mutex_Get_Status_Req_Pdu* pdu, XIXI_Mutex_Get_Status_Res_Pdu* res_pdu);
  xixi_reason mutex_lock(boost::shared_ptr<Session_Sink>& sp, const XIXI_Mutex_Lock_Req_Pdu* pdu);
  xixi_reason mutex_unlock(const XIXI_Mutex_Unlock_Req_Pdu* pdu);
  xixi_reason mutex_destroy(const XIXI_Mutex_Destroy_Req_Pdu* pdu);

private:
  mutex lock_;
  xixi::hash_map<Const_Data, session_node, Session_PK> sessions_;
  std::map<Session_Sink*, std::set<session_node*> > peers_;
};

extern Session_Mgr session_mgr_;

#endif // SESSION_H

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

#ifndef PEER_H
#define PEER_H

#include "defines.h"
#include "util.h"
#include <boost/unordered_map.hpp>
#include <boost/thread/mutex.hpp>
#include "peer_pdu.h"

using namespace std;

#ifndef USING_64_PEER_ID
typedef uint32_t PEER_ID;
#else
typedef uint64_t PEER_ID;
#endif

typedef uint8_t peer_state;
const peer_state PEER_STATE_NEW_CMD = 1;
const peer_state PEER_STATE_READ_HEADER = 2;
const peer_state PEER_STATE_READ_BODY_FIXED = 3;
const peer_state PEER_STATE_READ_BODY_EXTRAS = 4; // for big extras
const peer_state PEER_STATE_READ_BODY_EXTRAS2 = 5;// for small extras
const peer_state PEER_STATE_SWALLOW = 6;
const peer_state PEER_STATUS_ASYNC_WAIT = 7;
const peer_state PEER_STATUS_WRITE = 8;
const peer_state PEER_STATE_CLOSING = 9;
const peer_state PEER_STATE_CLOSED = 10;

class Peer_Mgr;

////////////////////////////////////////////////////////////////////////////////
// Peer
//
class Peer {
  friend class Peer_Mgr;
public:
  Peer();
  ~Peer();

  PEER_ID get_peer_id() { return peer_id_; };

protected:
  void set_peer_id(PEER_ID peer_id) { peer_id_ = peer_id; }

protected:
  PEER_ID peer_id_;
};

////////////////////////////////////////////////////////////////////////////////
// Peer_Mgr
//
class Peer_Mgr {
  typedef std::pair<PEER_ID,Peer*> MapPair;
  typedef unordered_map<PEER_ID,Peer*> Map;
  typedef unordered_map<PEER_ID, Peer*>::iterator Iterator; 
public:
  Peer_Mgr() {
    peer_id_ = 0;
  }

  static Peer* get_processor(uint8_t* data, uint32_t data_len);

  void add(Peer* peer) {
    MapPair v;
    v.second = peer;
    lock_.lock();
    ++peer_id_;
    v.first = peer_id_;
    std::pair<Iterator, bool> pair = peer_map_.insert(v);
    while (!pair.second) {
      peer_id_++;
      v.first = peer_id_;
      pair = peer_map_.insert(v);
    }
    peer->set_peer_id(peer_id_);
    lock_.unlock();
  }

  void remove(PEER_ID peer_id) {
    lock_.lock();
    peer_map_.erase(peer_id);
    lock_.unlock();
  }

  void dispatch_message(PEER_ID peer_id) {
    lock_.lock();
    Iterator it = peer_map_.find(peer_id);
//    it->second->dispatch_message();
    lock_.unlock();
  }

private:
  mutex lock_;
  Map peer_map_;
  PEER_ID peer_id_;
};

extern Peer_Mgr peer_mgr_;

#endif // PEER_H

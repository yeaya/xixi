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

#include "peer.h"
#include "settings.h"
#include "currtime.h"
#include <string.h>
#include "log.h"

#define LOG_TRACE2(x)    LOG_TRACE("id=" << get_peer_id() << " " << x)
#define LOG_DEBUG2(x)    LOG_DEBUG("id=" << get_peer_id() << " " << x)
#define LOG_INFO2(x)     LOG_INFO("id=" << get_peer_id() << " " << x)
#define LOG_WARNING2(x)  LOG_WARNING("id=" << get_peer_id() << " " << x)
#define LOG_ERROR2(x)    LOG_ERROR("id=" << get_peer_id() << " " << x)

Peer_Mgr peer_mgr_;

Peer::Peer() /*: connection_(NULL)*/ {
  peer_mgr_.add(this);
  LOG_TRACE2("Peer::Peer()");
}

Peer::~Peer() {
  peer_mgr_.remove(peer_id_);
  LOG_TRACE2("~Peer::Peer()\n");
}

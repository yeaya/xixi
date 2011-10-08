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

#ifndef PEER_HA_PDU_H
#define PEER_HA_PDU_H

#include "defines.h"
#include "util.h"
#include "peer_pdu.h"

const xixi_choice XIXI_CHOICE_HA_PRESENT_REQ = XIXI_CHOICE_HA_BASE + 1;
const xixi_choice XIXI_CHOICE_HA_PRESENT_RES = XIXI_CHOICE_HA_BASE + 2;

const xixi_choice XIXI_CHOICE_HA_CREATE_COMMITTEE_REQ = XIXI_CHOICE_HA_BASE + 3; // ELECTORAL_COMMITTEE
const xixi_choice XIXI_CHOICE_HA_CREATE_COMMITTEE_RES = XIXI_CHOICE_HA_BASE + 4;

const xixi_choice XIXI_CHOICE_HA_ANNOUNCE_LEADER_REQ = XIXI_CHOICE_HA_BASE + 5;
const xixi_choice XIXI_CHOICE_HA_ANNOUNCE_LEADER_RES = XIXI_CHOICE_HA_BASE + 6;

const xixi_choice XIXI_CHOICE_HA_KEEPALIVE_REQ = XIXI_CHOICE_HA_BASE + 7;
const xixi_choice XIXI_CHOICE_HA_KEEPALIVE_RES = XIXI_CHOICE_HA_BASE + 8;

const xixi_choice XIXI_CHOICE_HA_MUTEX_CREATE_REQ = XIXI_CHOICE_HA_BASE + 11;
const xixi_choice XIXI_CHOICE_HA_MUTEX_CREATE_RES = XIXI_CHOICE_HA_BASE + 12;

const xixi_choice XIXI_CHOICE_HA_MUTEX_GET_STATUS_REQ = XIXI_CHOICE_HA_BASE + 13;
const xixi_choice XIXI_CHOICE_HA_MUTEX_GET_STATUS_RES = XIXI_CHOICE_HA_BASE + 14;

const xixi_choice XIXI_CHOICE_HA_MUTEX_LOCK_REQ = XIXI_CHOICE_HA_BASE + 15;
const xixi_choice XIXI_CHOICE_HA_MUTEX_LOCK_RES = XIXI_CHOICE_HA_BASE + 16;

const xixi_choice XIXI_CHOICE_HA_MUTEX_UNLOCK_REQ = XIXI_CHOICE_HA_BASE + 17;
const xixi_choice XIXI_CHOICE_HA_MUTEX_UNLOCK_RES = XIXI_CHOICE_HA_BASE + 18;

const xixi_choice XIXI_CHOICE_HA_MUTEX_DESTROY_REQ = XIXI_CHOICE_HA_BASE + 19;
const xixi_choice XIXI_CHOICE_HA_MUTEX_DESTROY_RES = XIXI_CHOICE_HA_BASE + 20;

typedef uint8_t xixi_ha_status;
const xixi_ha_status XIXI_HA_STATUS_LOOKING = 0;
const xixi_ha_status XIXI_HA_STATUS_LEADING = 1;
const xixi_ha_status XIXI_HA_STATUS_FOLLOWING = 2;

const uint32_t XIXI_PDU_HA_PRESENT_REQ_BODY_LENGTH = 3;
const uint32_t XIXI_PDU_HA_PRESENT_REQ_LENGTH = XIXI_PDU_CHOICE_LENGTH + 3;
struct XIXI_HA_Present_Req_Pdu : public XIXI_Pdu {
  uint32_t calc_encode_size() {
    return XIXI_PDU_CHOICE_LENGTH + 1 + 2 + server_id.size();
  }
  uint32_t get_extras_size() {
    return server_id.size();
  }
  void encode(uint8_t* buf) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_HA_PRESENT_REQ); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT8(buf, status); buf += 1;
    ENCODE_UINT16(buf, server_id.size()); buf += 2;
    memcpy(buf, server_id.c_str(), server_id.size());
  }

  void decode_fixed(uint8_t* buf, uint32_t length) {
    status = DECODE_UINT8(buf); buf += 1;
    uint16_t size = DECODE_UINT16(buf);
    server_id.resize(size);
  }

  void decode_extras(uint8_t* buf, uint32_t length) {
    server_id = string((char*)buf, server_id.size());
  }

  xixi_ha_status status;
  string server_id;
};

const uint32_t XIXI_PDU_HA_PRESENT_RES_BODY_LENGTH = 3;
const uint32_t XIXI_PDU_HA_PRESENT_RES_LENGTH = XIXI_PDU_CHOICE_LENGTH + 3;
struct XIXI_HA_Present_Res_Pdu : public XIXI_Pdu {
  uint32_t calc_encode_size() {
    return XIXI_PDU_CHOICE_LENGTH + 1 + 2 + server_id.size();
  }
  uint32_t get_extras_size() {
    return server_id.size();
  }
  void encode(uint8_t* buf) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_HA_PRESENT_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT8(buf, status); buf += 1;
    ENCODE_UINT16(buf, server_id.size()); buf += 2;
    memcpy(buf, server_id.c_str(), server_id.size());
  }

  void decode_fixed(uint8_t* buf, uint32_t length) {
    status = DECODE_UINT8(buf); buf += 1;
    uint16_t size = DECODE_UINT16(buf);
    server_id.resize(size);
  }

  void decode_extras(uint8_t* buf, uint32_t length) {
    server_id = string((char*)buf, server_id.size());
  }

  xixi_ha_status status;
  string server_id;
};

const uint32_t XIXI_PDU_HA_CREATE_COMMITTEE_REQ_BODY_LENGTH = 3;
const uint32_t XIXI_PDU_HA_CREATE_COMMITTEE_REQ_LENGTH = XIXI_PDU_CHOICE_LENGTH + 3;
struct XIXI_HA_Create_Committee_Req_Pdu : public XIXI_Pdu {
  uint32_t calc_encode_size() {
    return XIXI_PDU_CHOICE_LENGTH + 1 + 2 + server_id.size();
  }
  uint32_t get_extras_size() {
    return server_id.size();
  }
  void encode(uint8_t* buf) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_HA_CREATE_COMMITTEE_REQ); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT8(buf, status); buf += 1;
    ENCODE_UINT16(buf, server_id.size()); buf += 2;
    memcpy(buf, server_id.c_str(), server_id.size());
  }

  void decode_fixed(uint8_t* buf, uint32_t length) {
    status = DECODE_UINT8(buf); buf += 1;
    uint16_t size = DECODE_UINT16(buf);
    server_id.resize(size);
  }

  void decode_extras(uint8_t* buf, uint32_t length) {
    server_id = string((char*)buf, server_id.size());
  }

  xixi_ha_status status;
  string server_id;
};

const uint32_t XIXI_PDU_HA_CREATE_COMMITTEE_RES_BODY_LENGTH = 9;
const uint32_t XIXI_PDU_HA_CREATE_COMMITTEE_RES_LENGTH = XIXI_PDU_CHOICE_LENGTH + 9;
struct XIXI_HA_Create_Committee_Res_Pdu : public XIXI_Pdu {
  uint32_t calc_encode_size() {
    uint32_t size = XIXI_PDU_CHOICE_LENGTH + 1 + 2 + 2 + 4 + server_id.size();
    for (std::vector<string>::size_type i = 0; i < votes.size(); i++) {
      string& vote = votes[i];
      size += 2 + vote.size();
    }
    return size;
  }
  uint32_t get_fixed_size() {
    return 9;
  }
  uint32_t get_extras_size() {
    return extras_size;
  }
  void encode(uint8_t* buf) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_HA_CREATE_COMMITTEE_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT8(buf, status); buf += 1;
    ENCODE_UINT16(buf, server_id.size()); buf += 2;
    ENCODE_UINT16(buf, votes.size()); buf += 2;
    extras_size = server_id.size();
    for (size_t i = 0; i < votes.size(); i++) {
      string& vote = votes[i];
      extras_size += 2 + vote.size();
    }
    ENCODE_UINT32(buf, extras_size); buf += 4;
    memcpy(buf, server_id.c_str(), server_id.size()); buf += server_id.size();
    for (size_t i = 0; i < votes.size(); i++) {
      string& vote = votes[i];
      ENCODE_UINT16(buf, vote.size()); buf += 2;
      memcpy(buf, vote.c_str(), vote.size()); buf += vote.size();
    }
  }

  void decode_fixed(uint8_t* buf, uint32_t length) {
    status = DECODE_UINT8(buf); buf += 1;
    uint16_t size = DECODE_UINT16(buf); buf += 2;
    server_id.resize(size);
    size = DECODE_UINT16(buf); buf += 2;
    votes.resize(size);
    extras_size = DECODE_UINT32(buf);
  }

  void decode_extras(uint8_t* buf, uint32_t length) {
    uint16_t size = server_id.size();
    server_id = string((char*)buf, size); buf += size;
    for (size_t i = 0; i < votes.size(); i++) {
      size = DECODE_UINT16(buf); buf += 2;
      votes[i] = string((char*)buf, size); buf += size;
    }
  }

  xixi_ha_status status;
  string server_id;
  uint32_t extras_size;
  std::vector<string> votes;
};

const uint32_t XIXI_PDU_HA_ANNOUNCE_LEADER_REQ_BODY_LENGTH = 3;
const uint32_t XIXI_PDU_HA_ANNOUNCE_LEADER_REQ_LENGTH = XIXI_PDU_CHOICE_LENGTH + 3;
struct XIXI_HA_Announce_Leader_Req_Pdu : public XIXI_Pdu {
  uint32_t calc_encode_size() {
    return XIXI_PDU_CHOICE_LENGTH + 1 + 2 + server_id.size();
  }
  uint32_t get_extras_size() {
    return server_id.size();
  }
  void encode(uint8_t* buf) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_HA_ANNOUNCE_LEADER_REQ); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT8(buf, status); buf += 1;
    ENCODE_UINT16(buf, server_id.size()); buf += 2;
    memcpy(buf, server_id.c_str(), server_id.size());
  }

  void decode_fixed(uint8_t* buf, uint32_t length) {
    status = DECODE_UINT8(buf); buf += 1;
    uint16_t size = DECODE_UINT16(buf);
    server_id.resize(size);
  }

  void decode_extras(uint8_t* buf, uint32_t length) {
    server_id = string((char*)buf, server_id.size());
  }

  xixi_ha_status status;
  string server_id;
};

const uint32_t XIXI_PDU_HA_ANNOUNCE_LEADER_RES_BODY_LENGTH = 3;
const uint32_t XIXI_PDU_HA_ANNOUNCE_LEADER_RES_LENGTH = XIXI_PDU_CHOICE_LENGTH + 3;
struct XIXI_HA_Announce_Leader_Res_Pdu : public XIXI_Pdu {
  uint32_t calc_encode_size() {
    return XIXI_PDU_CHOICE_LENGTH + 1 + 2 + server_id.size();
  }
  uint32_t get_extras_size() {
    return server_id.size();
  }
  void encode(uint8_t* buf) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_HA_ANNOUNCE_LEADER_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT8(buf, status); buf += 1;
    ENCODE_UINT16(buf, server_id.size()); buf += 2;
    memcpy(buf, server_id.c_str(), server_id.size());
  }

  void decode_fixed(uint8_t* buf, uint32_t length) {
    status = DECODE_UINT8(buf); buf += 1;
    uint16_t size = DECODE_UINT16(buf);
    server_id.resize(size);
  }

  void decode_extras(uint8_t* buf, uint32_t length) {
    server_id = string((char*)buf, server_id.size());
  }

  xixi_ha_status status;
  string server_id;
};

const uint32_t XIXI_PDU_HA_KEEPALIVE_REQ_BODY_LENGTH = 3;
const uint32_t XIXI_PDU_HA_KEEPALIVE_REQ_LENGTH = XIXI_PDU_CHOICE_LENGTH + 3;
struct XIXI_HA_Keepalive_Req_Pdu : public XIXI_Pdu {
  uint32_t calc_encode_size() {
    return XIXI_PDU_CHOICE_LENGTH + 1 + 2 + server_id.size();
  }
  uint32_t get_extras_size() {
    return server_id.size();
  }
  void encode(uint8_t* buf) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_HA_KEEPALIVE_REQ); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT8(buf, status); buf += 1;
    ENCODE_UINT16(buf, server_id.size()); buf += 2;
    memcpy(buf, server_id.c_str(), server_id.size());
  }

  void decode_fixed(uint8_t* buf, uint32_t length) {
    status = DECODE_UINT8(buf); buf += 1;
    uint16_t size = DECODE_UINT16(buf);
    server_id.resize(size);
  }

  void decode_extras(uint8_t* buf, uint32_t length) {
    server_id = string((char*)buf, server_id.size());
  }

  xixi_ha_status status;
  string server_id;
};

const uint32_t XIXI_PDU_HA_KEEPALIVE_RES_BODY_LENGTH = 3;
const uint32_t XIXI_PDU_HA_KEEPALIVE_RES_LENGTH = XIXI_PDU_CHOICE_LENGTH + 3;
struct XIXI_HA_Keepalive_Res_Pdu : public XIXI_Pdu {
  uint32_t calc_encode_size() {
    return XIXI_PDU_CHOICE_LENGTH + 1 + 2 + server_id.size();
  }
  uint32_t get_extras_size() {
    return server_id.size();
  }
  void encode(uint8_t* buf) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_HA_KEEPALIVE_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT8(buf, status); buf += 1;
    ENCODE_UINT16(buf, server_id.size()); buf += 2;
    memcpy(buf, server_id.c_str(), server_id.size());
  }

  void decode_fixed(uint8_t* buf, uint32_t length) {
    status = DECODE_UINT8(buf); buf += 1;
    uint16_t size = DECODE_UINT16(buf);
    server_id.resize(size);
  }

  void decode_extras(uint8_t* buf, uint32_t length) {
    server_id = string((char*)buf, server_id.size());
  }

  xixi_ha_status status;
  string server_id;
};

/*
const uint32_t XIXI_PDU_HA_ANNOUNCE_LEADER_REQ_BODY_LENGTH = 15;
const uint32_t XIXI_PDU_HA_ANNOUNCE_LEADER_REQ_LENGTH = XIXI_PDU_CHOICE_LENGTH + 15;
struct XIXI_HA_Announce_Leader_Req_Pdu : public XIXI_Pdu {
  void decode_fixed(uint8_t* buf, uint32_t length) {
    request_id = DECODE_UINT32(buf); buf += 4;
    status = DECODE_UINT8(buf); buf += 1;
    update_count = DECODE_UINT64(buf); buf += 8;
    id.size = DECODE_UINT16(buf);
  }

  uint32_t request_id;
  uint8_t status;
  uint64_t update_count;
  Const_Data id;
};

const uint32_t XIXI_PDU_HA_ANNOUNCE_TEMP_LEADER_RES_BODY_LENGTH = 6;
const uint32_t XIXI_PDU_HA_ANNOUNCE_TEMP_LEADER_RES_LENGTH = XIXI_PDU_CHOICE_LENGTH + 6;
struct XIXI_HA_Announce_Temp_Leader_Res_Pdu : public XIXI_Pdu {
  static void encode(uint8_t* buf, uint32_t request_id, xixi_reason reason) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_HA_MUTEX_CREATE_RES) buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT32(buf, request_id); buf += 4;
    ENCODE_REASON(buf, reason);
  }

  uint32_t request_id;
  xixi_reason reason;
  uint8_t status;
  uint64_t update_count;
  Const_Data id;
};
*/
const uint32_t XIXI_PDU_HA_MUTEX_CREATE_REQ_BODY_LENGTH = 8;
const uint32_t XIXI_PDU_HA_MUTEX_CREATE_REQ_LENGTH = XIXI_PDU_CHOICE_LENGTH + 8;
struct XIXI_HA_Mutex_Create_Req_Pdu : public XIXI_Pdu {
  void decode_fixed(uint8_t* buf, uint32_t length) {
    request_id = DECODE_UINT32(buf); buf += 4;
    key.size = DECODE_UINT16(buf); buf += 2;
    session_id.size = DECODE_UINT16(buf);
  }

  uint32_t request_id;
  Const_Data key;
  Const_Data session_id;
};

const uint32_t XIXI_PDU_HA_MUTEX_CREATE_RES_BODY_LENGTH = 6;
const uint32_t XIXI_PDU_HA_MUTEX_CREATE_RES_LENGTH = XIXI_PDU_CHOICE_LENGTH + 6;
struct XIXI_HA_Mutex_Create_Res_Pdu : public XIXI_Pdu {
  static void encode(uint8_t* buf, uint32_t request_id, xixi_reason reason) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_HA_MUTEX_CREATE_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT32(buf, request_id); buf += 4;
    ENCODE_REASON(buf, reason);
  }

  uint32_t request_id;
  xixi_reason reason;
};

const uint32_t XIXI_PDU_HA_MUTEX_LOCK_REQ_BODY_LENGTH = 9;
const uint32_t XIXI_PDU_HA_MUTEX_LOCK_REQ_LENGTH = XIXI_PDU_CHOICE_LENGTH + 9;
struct XIXI_HA_Mutex_Lock_Req_Pdu : public XIXI_Pdu {
  void decode_fixed(uint8_t* buf, uint32_t length) {
    request_id = DECODE_UINT32(buf); buf += 4;
    uint8_t flag = DECODE_UINT8(buf); buf += 1;
    try_lock = ((flag & 0x01) == 0x01);
    create = ((flag & 0x02) == 0x02);
    shared = ((flag & 0x04) == 0x04);

    key.size = DECODE_UINT16(buf); buf += 2;
    session_id.size = DECODE_UINT16(buf);
  }

  uint32_t request_id;
  bool try_lock;
  bool create;
  bool shared;
  Const_Data key;
  Const_Data session_id;
};

const uint32_t XIXI_PDU_HA_MUTEX_LOCK_RES_BODY_LENGTH = 6;
const uint32_t XIXI_PDU_HA_MUTEX_LOCK_RES_LENGTH = XIXI_PDU_HEAD_LENGTH + 6;
struct XIXI_HA_Mutex_Lock_Res_Pdu : public XIXI_Pdu {
  XIXI_HA_Mutex_Lock_Res_Pdu() {
  //  choice = XIXI_CHOICE_HA_MUTEX_LOCK_RES;
  }
  static void encode(uint8_t* buf, uint32_t request_id, xixi_reason reason) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_HA_MUTEX_LOCK_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT32(buf, request_id); buf += 4;
    ENCODE_REASON(buf, reason);
  }

  uint32_t request_id;
  xixi_reason reason;
};

const uint32_t XIXI_PDU_HA_MUTEX_UNLOCK_REQ_BODY_LENGTH = 9;
const uint32_t XIXI_PDU_HA_MUTEX_UNLOCK_REQ_LENGTH = XIXI_PDU_HEAD_LENGTH + 9;
struct XIXI_HA_Mutex_Unlock_Req_Pdu : public XIXI_Pdu {
  void decode_fixed(uint8_t* buf, uint32_t length) {
    request_id = DECODE_UINT32(buf); buf += 4;
    uint8_t flag = DECODE_UINT8(buf); buf += 1;
    shared = ((flag & 0x01) == 0x01);
  //  reply = ((flag & 0x02) == 0x02);
    key.size = DECODE_UINT16(buf); buf += 2;
    session_id.size = DECODE_UINT16(buf);
  }

  uint32_t request_id;
  bool shared;
//  bool reply;
//  uint32_t lock_time;
//  uint32_t timeout;
  Const_Data key;
  Const_Data session_id;
};

const uint32_t XIXI_PDU_HA_MUTEX_UNLOCK_RES_BODY_LENGTH = 6;
const uint32_t XIXI_PDU_HA_MUTEX_UNLOCK_RES_LENGTH = XIXI_PDU_CHOICE_LENGTH + 6;
struct XIXI_HA_Mutex_Unlock_Res_Pdu : public XIXI_Pdu {
  static void encode(uint8_t* buf, uint32_t request_id, xixi_reason reason) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_HA_MUTEX_UNLOCK_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT32(buf, request_id); buf += 4;
    ENCODE_REASON(buf, reason);
  }

  uint32_t request_id;
  xixi_reason reason;
};

const uint32_t XIXI_PDU_HA_MUTEX_DESTROY_REQ_BODY_LENGTH = 8;
const uint32_t XIXI_PDU_HA_MUTEX_DESTROY_REQ_LENGTH = XIXI_PDU_CHOICE_LENGTH + 8;
struct XIXI_HA_Mutex_Destroy_Req_Pdu : public XIXI_Pdu {
  void decode_fixed(uint8_t* buf, uint32_t length) {
    request_id = DECODE_UINT32(buf); buf += 4;
    key.size = DECODE_UINT16(buf); buf += 2;
    session_id.size = DECODE_UINT16(buf);
  }
  uint32_t get_extras_size() {
    return key.size + session_id.size;
  }

  uint32_t request_id;
  Const_Data key;
  Const_Data session_id;
};

const uint32_t XIXI_PDU_HA_MUTEX_DESTROY_RES_BODY_LENGTH = 6;
const uint32_t XIXI_PDU_HA_MUTEX_DESTROY_RES_LENGTH = XIXI_PDU_CHOICE_LENGTH + 6;
struct XIXI_HA_Mutex_Destroy_Res_Pdu : public XIXI_Pdu {
  static void encode(uint8_t* buf, uint32_t request_id, xixi_reason reason) {
//    XIXI_Pdu_Header header;
//    header.choice = XIXI_CHOICE_HA_MUTEX_DESTROY_RES;
//    header.encode(buf); buf += XIXI_PDU_HEAD_LENGTH;
    ENCODE_CHOICE(buf, XIXI_CHOICE_HA_MUTEX_DESTROY_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT32(buf, request_id); buf += 4;
    ENCODE_REASON(buf, reason);
  }

  uint32_t request_id;
  xixi_reason reason;
};

#endif // PEER_HA_PDU_H

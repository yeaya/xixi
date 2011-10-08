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

#ifndef PEER_CC_PDU_H
#define PEER_CC_PDU_H

#include "defines.h"
#include "util.h"
#include "peer_pdu.h"

const xixi_choice XIXI_CHOICE_MUTEX_CREATE_REQ = XIXI_CHOICE_CC_BASE + 1;
const xixi_choice XIXI_CHOICE_MUTEX_CREATE_RES = XIXI_CHOICE_CC_BASE + 2;

const xixi_choice XIXI_CHOICE_MUTEX_GET_STATUS_REQ = XIXI_CHOICE_CC_BASE + 3;
const xixi_choice XIXI_CHOICE_MUTEX_GET_STATUS_RES = XIXI_CHOICE_CC_BASE + 4;

const xixi_choice XIXI_CHOICE_MUTEX_LOCK_REQ = XIXI_CHOICE_CC_BASE + 5;
const xixi_choice XIXI_CHOICE_MUTEX_LOCK_RES = XIXI_CHOICE_CC_BASE + 6;

const xixi_choice XIXI_CHOICE_MUTEX_UNLOCK_REQ = XIXI_CHOICE_CC_BASE + 7;
const xixi_choice XIXI_CHOICE_MUTEX_UNLOCK_RES = XIXI_CHOICE_CC_BASE + 8;

const xixi_choice XIXI_CHOICE_MUTEX_DESTROY_REQ = XIXI_CHOICE_CC_BASE + 9;
const xixi_choice XIXI_CHOICE_MUTEX_DESTROY_RES = XIXI_CHOICE_CC_BASE + 10;


const uint32_t XIXI_PDU_MUTEX_CREATE_REQ_BODY_LENGTH = 8;
struct XIXI_Mutex_Create_Req_Pdu : public XIXI_Pdu {
  void decode_fixed(uint8_t* buf, uint32_t length) {
    request_id = DECODE_UINT32(buf); buf += 4;
    key.size = DECODE_UINT16(buf); buf += 2;
    session_id.size = DECODE_UINT16(buf);
  }

  uint32_t request_id;
  Const_Data key;
  Const_Data session_id;
};

const uint32_t XIXI_PDU_MUTEX_CREATE_RES_LENGTH = XIXI_PDU_CHOICE_LENGTH + 6;
struct XIXI_Mutex_Create_Res_Pdu : public XIXI_Pdu {
  static void encode(uint8_t* buf, uint32_t request_id, xixi_reason reason) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_MUTEX_CREATE_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT32(buf, request_id); buf += 4;
    ENCODE_UINT16(buf, reason);
  }

  uint32_t request_id;
  xixi_reason reason;
};

const uint32_t XIXI_PDU_MUTEX_GET_STATUS_REQ_BODY_LENGTH = 8;
struct XIXI_Mutex_Get_Status_Req_Pdu : public XIXI_Pdu {
  void decode_fixed(uint8_t* buf, uint32_t length) {
    request_id = DECODE_UINT32(buf); buf += 4;
    key.size = DECODE_UINT16(buf); buf += 2;
    session_id.size = DECODE_UINT16(buf);
  }

  uint32_t request_id;
  Const_Data key;
  Const_Data session_id;
};

const uint32_t XIXI_PDU_MUTEX_GET_STATUS_RES_LENGTH = XIXI_PDU_HEAD_LENGTH + 7;
struct XIXI_Mutex_Get_Status_Res_Pdu : public XIXI_Pdu {
  XIXI_Mutex_Get_Status_Res_Pdu() {
    request_id = 0;
    reason = XIXI_REASON_SUCCESS;
    own_lock = false;
    own_shared_lock = false;
    wait_lock = false;
    wait_shared_lock = false;
  }
  void encode(uint8_t* buf) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_MUTEX_GET_STATUS_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT32(buf, request_id); buf += 4;
    ENCODE_REASON(buf, reason);
    uint8_t flag = 0;
    if (own_lock) {
      flag += 1;
    }
    if (own_shared_lock) {
      flag += 2;
    }
    if (wait_lock) {
      flag += 4;
    }
    if (wait_shared_lock) {
      flag += 8;
    }
  }

  uint32_t request_id;
  xixi_reason reason;
  bool own_lock;
  bool own_shared_lock;
  bool wait_lock;
  bool wait_shared_lock;
};

const uint32_t XIXI_PDU_MUTEX_LOCK_REQ_BODY_LENGTH = 9;
struct XIXI_Mutex_Lock_Req_Pdu : public XIXI_Pdu {
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

const uint32_t XIXI_PDU_MUTEX_LOCK_RES_LENGTH = XIXI_PDU_CHOICE_LENGTH + 6;
struct XIXI_Mutex_Lock_Res_Pdu : public XIXI_Pdu {
  XIXI_Mutex_Lock_Res_Pdu() {
  //  choice = XIXI_CHOICE_MUTEX_LOCK_RES;
  }
  static void encode(uint8_t* buf, uint32_t request_id, xixi_reason reason) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_MUTEX_LOCK_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT32(buf, request_id); buf += 4;
    ENCODE_REASON(buf, reason);
  }

  uint32_t request_id;
  xixi_reason reason;
};

const uint32_t XIXI_PDU_MUTEX_UNLOCK_REQ_BODY_LENGTH = 9;
struct XIXI_Mutex_Unlock_Req_Pdu : public XIXI_Pdu {
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

const uint32_t XIXI_PDU_MUTEX_UNLOCK_RES_LENGTH = XIXI_PDU_CHOICE_LENGTH + 6;
struct XIXI_Mutex_Unlock_Res_Pdu : public XIXI_Pdu {
  static void encode(uint8_t* buf, uint32_t request_id, xixi_reason reason) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_MUTEX_UNLOCK_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT32(buf, request_id); buf += 4;
    ENCODE_REASON(buf, reason);
  }

  uint32_t request_id;
  xixi_reason reason;
};

const uint32_t XIXI_PDU_MUTEX_DESTROY_REQ_BODY_LENGTH = 8;
struct XIXI_Mutex_Destroy_Req_Pdu : public XIXI_Pdu {
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

const uint32_t XIXI_PDU_MUTEX_DESTROY_RES_LENGTH = XIXI_PDU_CHOICE_LENGTH + 6;
struct XIXI_Mutex_Destroy_Res_Pdu : public XIXI_Pdu {
  static void encode(uint8_t* buf, uint32_t request_id, xixi_reason reason) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_MUTEX_UNLOCK_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT32(buf, request_id); buf += 4;
    ENCODE_REASON(buf, reason);
  }

  uint32_t request_id;
  xixi_reason reason;
};

#endif // PEER_CC_PDU_H

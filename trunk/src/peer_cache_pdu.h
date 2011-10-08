#ifndef PEER_CACHE_PDU_H
#define PEER_CACHE_PDU_H

#include "defines.h"
#include "util.h"
#include "peer_pdu.h"

const xixi_choice XIXI_CHOICE_GET_REQ = XIXI_CHOICE_CACHE_BASE + 1;
const xixi_choice XIXI_CHOICE_GET_TOUCH_REQ = XIXI_CHOICE_CACHE_BASE + 2;
const xixi_choice XIXI_CHOICE_GET_RES = XIXI_CHOICE_CACHE_BASE + 3;

const xixi_choice XIXI_CHOICE_GET_BASE_REQ = XIXI_CHOICE_CACHE_BASE + 4;
const xixi_choice XIXI_CHOICE_GET_BASE_RES = XIXI_CHOICE_CACHE_BASE + 5;

const xixi_choice XIXI_CHOICE_UPDATE_REQ = XIXI_CHOICE_CACHE_BASE + 6;
const xixi_choice XIXI_CHOICE_UPDATE_RES = XIXI_CHOICE_CACHE_BASE + 7;

const xixi_choice XIXI_CHOICE_UPDATE_BASE_REQ = XIXI_CHOICE_CACHE_BASE + 8;
const xixi_choice XIXI_CHOICE_UPDATE_BASE_RES = XIXI_CHOICE_CACHE_BASE + 9;

const xixi_choice XIXI_CHOICE_DELETE_REQ = XIXI_CHOICE_CACHE_BASE + 10;
const xixi_choice XIXI_CHOICE_DELETE_RES = XIXI_CHOICE_CACHE_BASE + 11;

const xixi_choice XIXI_CHOICE_DELTA_REQ = XIXI_CHOICE_CACHE_BASE + 12;
const xixi_choice XIXI_CHOICE_DELTA_RES = XIXI_CHOICE_CACHE_BASE + 13;

const xixi_choice XIXI_CHOICE_FLUSH_REQ = XIXI_CHOICE_CACHE_BASE + 14;
const xixi_choice XIXI_CHOICE_FLUSH_RES = XIXI_CHOICE_CACHE_BASE + 15;

const xixi_choice XIXI_CHOICE_AUTH_REQ = XIXI_CHOICE_CACHE_BASE + 16;
const xixi_choice XIXI_CHOICE_AUTH_RES = XIXI_CHOICE_CACHE_BASE + 17;

const xixi_choice XIXI_CHOICE_STATS_REQ = XIXI_CHOICE_CACHE_BASE + 18;
const xixi_choice XIXI_CHOICE_STATS_RES = XIXI_CHOICE_CACHE_BASE + 19;

const xixi_choice XIXI_CHOICE_CREATE_WATCH_REQ = XIXI_CHOICE_CACHE_BASE + 20;
const xixi_choice XIXI_CHOICE_CREATE_WATCH_RES = XIXI_CHOICE_CACHE_BASE + 21;

const xixi_choice XIXI_CHOICE_CHECK_WATCH_REQ = XIXI_CHOICE_CACHE_BASE + 22;
const xixi_choice XIXI_CHOICE_CHECK_WATCH_RES = XIXI_CHOICE_CACHE_BASE + 23;

class XIXI_Get_Req_Pdu : public XIXI_Pdu {
public:
  static uint32_t get_fixed_body_size() {
    return 10;
  }

  void decode_fixed(uint8_t* buf, uint32_t length) {
    group_id = DECODE_UINT32(buf);
    watch_id = DECODE_UINT32(buf + 4);
    key_length = DECODE_UINT16(buf + 8);
  }

  uint32_t group_id;
  uint32_t watch_id;
  uint16_t key_length;
//  uint8_t* key;
};

class XIXI_Get_Touch_Req_Pdu : public XIXI_Pdu {
public:
  static uint32_t get_fixed_body_size() {
    return 14;
  }

  void decode_fixed(uint8_t* buf, uint32_t length) {
    group_id = DECODE_UINT32(buf);
    watch_id = DECODE_UINT32(buf + 4);
    expiration = DECODE_UINT32(buf + 8);
    key_length = DECODE_UINT16(buf + 12);
  }

  uint32_t group_id;
  uint32_t watch_id;
  uint32_t expiration;
  uint16_t key_length;
//  uint8_t* key;
};

class XIXI_Get_Res_Pdu : public XIXI_Pdu {
public:
  static uint32_t calc_encode_size() {
    return XIXI_PDU_CHOICE_LENGTH + 20;
  }
  void encode(uint8_t* buf) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_GET_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT64(buf, cache_id); buf += 8;
    ENCODE_UINT32(buf, flags); buf += 4;
    ENCODE_UINT32(buf, expiration); buf += 4;
    ENCODE_UINT32(buf, data_length);
  }

  uint64_t cache_id;
  uint32_t flags;
  uint32_t expiration;
  uint32_t data_length;
//  uint8_t* data;
};

class XIXI_Get_Base_Req_Pdu : public XIXI_Pdu {
public:
  static uint32_t get_fixed_body_size() {
    return 6; // sizeof(key_length)
  }

  void decode_fixed(uint8_t* buf, uint32_t length) {
    group_id = DECODE_UINT32(buf);
    key_length = DECODE_UINT16(buf + 4);
  }

  uint32_t group_id;
  uint16_t key_length;
//  uint8_t* key;
};

class XIXI_Get_Base_Res_Pdu : public XIXI_Pdu {
public:
  static uint32_t calc_encode_size() {
    return XIXI_PDU_CHOICE_LENGTH + 16;
  }
  void encode(uint8_t* buf) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_GET_BASE_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT64(buf, cache_id); buf += 8;
    ENCODE_UINT32(buf, flags); buf += 4;
    ENCODE_UINT32(buf, expiration);
  }

  uint64_t cache_id;
  uint32_t flags;
  uint32_t expiration;
};

const uint8_t XIXI_UPDATE_SUB_OP_MASK = 7;
const uint8_t XIXI_UPDATE_SUB_OP_SET = 0;
const uint8_t XIXI_UPDATE_SUB_OP_ADD = 1;
const uint8_t XIXI_UPDATE_SUB_OP_REPLACE = 2;
const uint8_t XIXI_UPDATE_SUB_OP_APPEND = 3;
const uint8_t XIXI_UPDATE_SUB_OP_PREPEND = 4;
const uint8_t XIXI_UPDATE_REPLY = 128;
class XIXI_Update_Req_Pdu : public XIXI_Pdu {
public:
  static uint32_t get_fixed_body_size() {
    return 31;
  }
  void decode_fixed(uint8_t* buf, uint32_t length) {
    op_flag = DECODE_UINT8(buf); buf += 1;
    cache_id = DECODE_UINT64(buf); buf += 8;
    group_id = DECODE_UINT32(buf); buf += 4;
    flags = DECODE_UINT32(buf); buf += 4;
    expiration = DECODE_UINT32(buf); buf += 4;
    watch_id = DECODE_UINT32(buf); buf += 4;
    key_length = DECODE_UINT16(buf); buf += 2;
    data_length = DECODE_UINT32(buf);
  }

  uint8_t sub_op() const {
    return op_flag & XIXI_UPDATE_SUB_OP_MASK;
  }

  bool reply() const {
    return true;//(op_flag & XIXI_UPDATE_REPLY) == XIXI_UPDATE_REPLY;
  }

  uint8_t op_flag;
  uint64_t cache_id;
  uint32_t group_id;
  uint32_t flags;
  uint32_t expiration;
  uint32_t watch_id;
  uint16_t key_length;
  uint32_t data_length;
};

class XIXI_Update_Res_Pdu : public XIXI_Pdu {
public:
  static uint32_t calc_encode_size() {
    return XIXI_PDU_CHOICE_LENGTH + 8; // sizeof(cache_id)
  }
  static void encode(uint8_t* buf, uint64_t cache_id) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_UPDATE_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT64(buf, cache_id);
  }

  uint64_t cache_id;
};

const uint8_t XIXI_UPDATE_BASE_SUB_OP_FLAGS = 1;
const uint8_t XIXI_UPDATE_BASE_SUB_OP_EXPIRATION = 2;
const uint8_t XIXI_UPDATE_BASE_REPLY = 128;
class XIXI_Update_Base_Req_Pdu : public XIXI_Pdu {
public:
  static uint32_t get_fixed_body_size() {
    return 23;
  }
  void decode_fixed(uint8_t* buf, uint32_t length) {
    op_flag = DECODE_UINT8(buf); buf += 1;
    cache_id = DECODE_UINT64(buf); buf += 8;
    group_id = DECODE_UINT32(buf); buf += 4;
    flags = DECODE_UINT32(buf); buf += 4;
    expiration = DECODE_UINT32(buf); buf += 4;
    key_length = DECODE_UINT16(buf);
  }

  bool is_update_flags() const {
    return (op_flag & XIXI_UPDATE_BASE_SUB_OP_FLAGS) == XIXI_UPDATE_BASE_SUB_OP_FLAGS;
  }

  bool is_update_expiration() const {
    return (op_flag & XIXI_UPDATE_BASE_SUB_OP_EXPIRATION) == XIXI_UPDATE_BASE_SUB_OP_EXPIRATION;
  }

  bool reply() const {
    return true;//(op_flag & XIXI_UPDATE_BASE_REPLY) == XIXI_UPDATE_REPLY;
  }

  uint8_t op_flag;
  uint64_t cache_id;
  uint32_t group_id;
  uint32_t flags;
  uint32_t expiration;
  uint16_t key_length;
};

class XIXI_Update_Base_Res_Pdu : public XIXI_Pdu {
public:
  static uint32_t calc_encode_size() {
    return XIXI_PDU_CHOICE_LENGTH + 8; // sizeof(cache_id)
  }
  static void encode(uint8_t* buf, uint64_t cache_id) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_UPDATE_BASE_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT64(buf, cache_id);
  }

  uint64_t cache_id;
};

const uint8_t XIXI_DELETE_SUB_OP_MASK = 1;
const uint8_t XIXI_DELETE_SUB_OP = 0;
const uint8_t XIXI_DELETE_REPLY = 128;
class XIXI_Delete_Req_Pdu : public XIXI_Pdu {
public:
  static uint32_t get_fixed_body_size() {
    return 15;
  }
  void decode_fixed(uint8_t* buf, uint32_t length) {
    op_flag = DECODE_UINT8(buf); buf += 1;
    cache_id = DECODE_UINT64(buf); buf += 8;
    group_id = DECODE_UINT32(buf); buf += 4;
    key_length = DECODE_UINT16(buf);
  }
  uint8_t sub_op() const {
    return op_flag & XIXI_DELETE_SUB_OP_MASK;
  }

  bool reply() const {
    return true;//(op_flag & XIXI_DELETE_REPLY) == XIXI_DELETE_REPLY;
  }

  uint8_t op_flag;
  uint64_t cache_id;
  uint32_t group_id;
  uint16_t key_length;
//  uint8_t* key;
};

const uint8_t XIXI_DELTA_SUB_OP_MASK = 1;
const uint8_t XIXI_DELTA_SUB_OP_INCR = 0;
const uint8_t XIXI_DELTA_SUB_OP_DECR = 1;
const uint8_t XIXI_DELTA_REPLY = 128;
class XIXI_Delta_Req_Pdu : public XIXI_Pdu {
public:
  static uint32_t get_fixed_body_size() {
    return 23;
  }
  void decode_fixed(uint8_t* buf, uint32_t length) {
    op_flag = DECODE_UINT8(buf); buf += 1;
    cache_id = DECODE_UINT64(buf); buf += 8;
    group_id = DECODE_UINT32(buf); buf += 4;
    delta = DECODE_UINT64(buf); buf += 8;
    key_length = DECODE_UINT16(buf);
  }

  uint8_t sub_op() const {
    return op_flag & XIXI_DELTA_SUB_OP_MASK;
  }

  bool reply() const {
    return true;//(op_flag & XIXI_UPDATE_REPLY) == XIXI_DELTA_REPLY;
  }

  uint8_t op_flag;
  uint64_t cache_id;
  uint32_t group_id;
  int64_t delta;
  uint16_t key_length;
};

class XIXI_Delta_Res_Pdu : public XIXI_Pdu {
public:
  static uint32_t calc_encode_size() {
    return XIXI_PDU_CHOICE_LENGTH + 16;
  }
  static void encode(uint8_t* buf, int64_t value, uint64_t cache_id) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_DELTA_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT64(buf, cache_id); buf += 8;
    ENCODE_UINT64(buf, value);
  }

  uint64_t cache_id;
  int64_t value;
};

class XIXI_Flush_Req_Pdu : public XIXI_Pdu {
public:
  static uint32_t get_fixed_body_size() {
    return 4;
  }
  void decode_fixed(uint8_t* buf, uint32_t length) {
    group_id = DECODE_UINT32(buf);
  }

    uint32_t group_id;
};

class XIXI_Flush_Res_Pdu : public XIXI_Pdu {
public:
  static uint32_t calc_encode_size() {
    return XIXI_PDU_CHOICE_LENGTH + 4 + 8;
  }
  void encode(uint8_t* buf) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_FLUSH_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT32(buf, flush_count); buf += 4;
    ENCODE_UINT64(buf, flush_size);
  }

  uint32_t flush_count;
  uint64_t flush_size;
};

// ANONYMOUS
// PLAIN user password
class XIXI_Auth_Req_Pdu : public XIXI_Pdu {
public:
  static uint32_t get_fixed_body_size() {
    return 4;
  }
  void decode_fixed(uint8_t* buf, uint32_t length) {
    base64_length = DECODE_UINT32(buf);
  }

    uint32_t base64_length;
  uint8_t* base64;
};

class XIXI_Auth_Res_Pdu : public XIXI_Pdu {
public:
  static uint32_t calc_encode_size() {
    return XIXI_PDU_CHOICE_LENGTH + 4;
  }
  void encode(uint8_t* buf) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_AUTH_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT32(buf, base64_length);
  }

  uint32_t base64_length;
  uint8_t* base64;
};

const uint8_t XIXI_STATS_SUB_OP_MASK = 7;
const uint8_t XIXI_STATS_SUB_OP_ADD_GROUP = 0;
const uint8_t XIXI_STATS_SUB_OP_REMOVE_GROUP = 1;
const uint8_t XIXI_STATS_SUB_OP_GET_STATS_GROUP_ONLY = 2;
const uint8_t XIXI_STATS_SUB_OP_GET_AND_CLEAR_STATS_GROUP_ONLY = 3;
const uint8_t XIXI_STATS_SUB_OP_GET_STATS_SUM_ONLY = 4;
const uint8_t XIXI_STATS_SUB_OP_GET_AND_CLEAR_STATS_SUM_ONLY = 5;
class XIXI_Stats_Req_Pdu : public XIXI_Pdu {
public:
  static uint32_t get_fixed_body_size() {
    return 6;
  }
  void decode_fixed(uint8_t* buf, uint32_t length) {
    op_flag = DECODE_UINT8(buf);
    class_id = DECODE_UINT8(buf + 1);
    group_id = DECODE_UINT32(buf + 2);
  }
  uint8_t sub_op() const {
    return op_flag & XIXI_STATS_SUB_OP_MASK;
  }
  uint8_t op_flag;
  uint8_t class_id;
  uint32_t group_id;
};

class XIXI_Stats_Res_Pdu : public XIXI_Pdu {
public:
  static uint32_t calc_encode_size(uint32_t result_length) {
    return XIXI_PDU_CHOICE_LENGTH + 4 + result_length;
  }
  static void encode(uint8_t* buf, uint8_t* result, uint32_t result_length) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_STATS_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT32(buf, result_length); buf += 4;
    memcpy(buf, result, result_length);
  }

  uint32_t result_length;
};

class XIXI_Create_Watch_Req_Pdu : public XIXI_Pdu {
public:
  static uint32_t get_fixed_body_size() {
    return 8;
  }
  void decode_fixed(uint8_t* buf, uint32_t length) {
    group_id = DECODE_UINT32(buf);
    max_next_check_interval = DECODE_UINT32(buf + 4);
  }

  uint32_t group_id;
  uint32_t max_next_check_interval;
};

class XIXI_Create_Watch_Res_Pdu : public XIXI_Pdu {
public:
  static uint32_t get_body_size() {
    return XIXI_PDU_CHOICE_LENGTH + 4;
  }
  static void encode(uint8_t* buf, uint32_t watch_id) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_CREATE_WATCH_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT32(buf, watch_id);
  }

  uint32_t watch_id;
};

class XIXI_Check_Watch_Req_Pdu : public XIXI_Pdu {
public:
  static uint32_t get_fixed_body_size() {
    return 24;
  }
  void decode_fixed(uint8_t* buf, uint32_t length) {
    group_id = DECODE_UINT32(buf);
    watch_id = DECODE_UINT32(buf + 4);
    max_next_check_interval = DECODE_UINT32(buf + 8);
    check_timeout = DECODE_UINT32(buf + 12);
    ack_cache_id = DECODE_UINT64(buf + 16);
  }

  uint32_t group_id;
  uint32_t watch_id;
  uint32_t max_next_check_interval;
  uint32_t check_timeout;
  uint64_t ack_cache_id;
};

class XIXI_Check_Watch_Res_Pdu : public XIXI_Pdu {
public:
  static uint32_t calc_encode_size(uint32_t updated_count) {
    return XIXI_PDU_CHOICE_LENGTH + 4 + updated_count * 8;
  }
  static void encode(uint8_t* buf, uint32_t update_count, std::list<uint64_t>& update_list) {
    ENCODE_CHOICE(buf, XIXI_CHOICE_CHECK_WATCH_RES); buf += XIXI_PDU_CHOICE_LENGTH;
    ENCODE_UINT32(buf, update_count); buf += 4;
    std::list<uint64_t>::iterator it = update_list.begin();
    while (it != update_list.end()) {
      ENCODE_UINT64(buf, *it); buf += 8;
      ++it;
    }
  }

  uint32_t update_count;
};

#endif // PEER_CACHE_PDU_H

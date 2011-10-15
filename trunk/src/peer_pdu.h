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

#ifndef XIXI_PDU_H
#define XIXI_PDU_H

#include "xixibase.h"
#include "util.h"

const uint32_t MAX_PDU_FIXED_BODY_LENGTH = 64;
const uint32_t MAX_PDU_FIXED_LENGTH = 66;

typedef uint8_t xixi_category;
typedef uint8_t xixi_command;
typedef uint16_t xixi_choice;
// choice
// ----------------------
// | category | command |
// ----------------------

const xixi_category XIXI_CATEGORY_COMMON = 0x00;
const xixi_category XIXI_CATEGORY_CC = 0x01;
const xixi_category XIXI_CATEGORY_CACHE = 0x02;
const xixi_category XIXI_CATEGORY_HA = 0x03;

const xixi_choice XIXI_CHOICE_COMMON_BASE = (XIXI_CATEGORY_COMMON << 8);
const xixi_choice XIXI_CHOICE_CC_BASE = (XIXI_CATEGORY_CC << 8);
const xixi_choice XIXI_CHOICE_CACHE_BASE = (XIXI_CATEGORY_CACHE << 8);
const xixi_choice XIXI_CHOICE_HA_BASE = (XIXI_CATEGORY_HA << 8);

const xixi_choice XIXI_CHOICE_ERROR = XIXI_CHOICE_COMMON_BASE + 0;
const xixi_choice XIXI_CHOICE_HELLO_REQ = XIXI_CHOICE_COMMON_BASE + 1;
const xixi_choice XIXI_CHOICE_HELLO_RES = XIXI_CHOICE_COMMON_BASE + 2;
const xixi_choice XIXI_CHOICE_BYE_REQ = XIXI_CHOICE_COMMON_BASE + 3;
const xixi_choice XIXI_CHOICE_BYE_RES = XIXI_CHOICE_COMMON_BASE + 4;
const xixi_choice XIXI_CHOICE_BYE_IND = XIXI_CHOICE_COMMON_BASE + 5;
const xixi_choice XIXI_CHOICE_SESSION_CREATE_REQ = XIXI_CHOICE_COMMON_BASE + 6;
const xixi_choice XIXI_CHOICE_SESSION_CREATE_RES = XIXI_CHOICE_COMMON_BASE + 7;

#define XIXI_PDU_CHOICE_LENGTH 2
#define XIXI_PDU_HEAD_LENGTH XIXI_PDU_CHOICE_LENGTH

// fixed_size = choice_length + fixed_body_size

#define ENCODE_CHOICE(buf, choice) ENCODE_UINT16(buf, choice)
#define DECODE_CHOICE(buf) DECODE_UINT16(buf)

#define XIXI_PDU_REASON_LENGTH 2
#define ENCODE_REASON(buf, reason) ENCODE_UINT16(buf, reason)
#define DECODE_REASON(buf) DECODE_UINT16(buf)

struct XIXI_Pdu_Header {
	inline void encode(uint8_t* buf) {
		ENCODE_UINT16(buf, choice);
	}
	inline void decode(uint8_t* buf) {
		choice = DECODE_UINT16(buf);
	}
	inline static void encode_choice(uint8_t* buf, xixi_choice choice) {
		ENCODE_UINT16(buf, choice);
	}
	inline xixi_category category() {
		return (xixi_category)(choice >> 8);
	}
	inline xixi_command command() {
		return (xixi_command)(choice);
	}

	xixi_choice choice; // build with(uint8_t category, uint8_t type)
};

class XIXI_Pdu {
public:
	static XIXI_Pdu* new_fixed_pdu(XIXI_Pdu_Header& header, uint8_t* buf, uint32_t length, xixi_reason& error_code);
	static void delete_pdu(XIXI_Pdu* pdu);
	static bool decode_pdu(uint8_t* pdu_buffer, XIXI_Pdu_Header& header, uint8_t* buf, uint32_t length);
	static XIXI_Pdu* decode_pdu_4_ha(XIXI_Pdu_Header& header, uint8_t* buf, uint32_t length);

	xixi_choice choice; // build with(uint8_t category, uint8_t type)
};

const uint32_t XIXI_PDU_HELLO_REQ_BODY_LENGTH = 2;
typedef XIXI_Pdu XIXI_Hello_Req_Pdu;

const uint32_t XIXI_PDU_SIMPLE_RES_LENGTH = XIXI_PDU_HEAD_LENGTH;
struct XIXI_Simple_Res_Pdu : public XIXI_Pdu {
	static void encode(uint8_t* buf, xixi_choice choice) {
		ENCODE_CHOICE(buf, choice);
	}
};

const uint32_t XIXI_PDU_SIMPLE_RES_WITH_REQID_LENGTH = XIXI_PDU_CHOICE_LENGTH + 4;
struct XIXI_Simple_Res_With_ReqID_Pdu : public XIXI_Pdu {
	static void encode(uint8_t* buf, uint32_t request_id, xixi_choice choice) {
		ENCODE_CHOICE(buf, choice); buf += XIXI_PDU_CHOICE_LENGTH;
		ENCODE_UINT32(buf, request_id); buf += 4;
	}
	uint32_t request_id;
};

const uint32_t XIXI_PDU_ERROR_RES_LENGTH = XIXI_PDU_CHOICE_LENGTH + XIXI_PDU_REASON_LENGTH;
struct XIXI_Error_Res_Pdu : public XIXI_Pdu {
	static void encode(uint8_t* buf, xixi_reason reason) {
		ENCODE_CHOICE(buf, XIXI_CHOICE_ERROR); buf += XIXI_PDU_CHOICE_LENGTH;
		ENCODE_UINT16(buf, reason);
	}
	xixi_reason error_code;
};

const uint32_t XIXI_PDU_ERROR_RES_WITH_REQID_LENGTH = XIXI_PDU_CHOICE_LENGTH + 6;
struct XIXI_Error_Res_With_ReqID_Pdu : public XIXI_Pdu {
	static void encode(uint8_t* buf, uint32_t request_id, xixi_reason reason) {
		ENCODE_CHOICE(buf, XIXI_CHOICE_ERROR); buf += XIXI_PDU_CHOICE_LENGTH;
		ENCODE_UINT32(buf, request_id); buf += 4;
		ENCODE_REASON(buf, reason);
	}
	uint32_t request_id;
	xixi_reason error_code;
};

const uint32_t XIXI_PDU_BYE_REQ_BODY_LENGTH = 2;
typedef XIXI_Pdu XIXI_Bye_Req_Pdu;

#endif // XIXI_PDU_H

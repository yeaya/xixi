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

#include "peer_cache_pdu.h"

bool XIXI_Pdu::decode_pdu(uint8_t* pdu_buffer, XIXI_Pdu_Header& header, uint8_t* buf, uint32_t length) {
	switch (header.choice) {
	case XIXI_CHOICE_GET_REQ:
		((XIXI_Get_Req_Pdu*)pdu_buffer)->decode_fixed(buf, length);
		break;
	case XIXI_CHOICE_GET_TOUCH_REQ:
		((XIXI_Get_Touch_Req_Pdu*)pdu_buffer)->decode_fixed(buf, length);
		break;
	case XIXI_CHOICE_GET_BASE_REQ:
		((XIXI_Get_Base_Req_Pdu*)pdu_buffer)->decode_fixed(buf, length);
		break;
	case XIXI_CHOICE_UPDATE_REQ:
		((XIXI_Update_Req_Pdu*)pdu_buffer)->decode_fixed(buf, length);
		break;
	case XIXI_CHOICE_UPDATE_FLAGS_REQ:
		((XIXI_Update_Flags_Req_Pdu*)pdu_buffer)->decode_fixed(buf, length);
		break;
	case XIXI_CHOICE_UPDATE_EXPIRATION_REQ:
		((XIXI_Update_Expiration_Req_Pdu*)pdu_buffer)->decode_fixed(buf, length);
		break;
	case XIXI_CHOICE_DELETE_REQ:
		((XIXI_Delete_Req_Pdu*)pdu_buffer)->decode_fixed(buf, length);
		break;
	case XIXI_CHOICE_AUTH_REQ:
		((XIXI_Auth_Req_Pdu*)pdu_buffer)->decode_fixed(buf, length);
		break;
	case XIXI_CHOICE_DELTA_REQ:
		((XIXI_Delta_Req_Pdu*)pdu_buffer)->decode_fixed(buf, length);
		break;
	case XIXI_CHOICE_FLUSH_REQ:
		((XIXI_Flush_Req_Pdu*)pdu_buffer)->decode_fixed(buf, length);
		break;
	case XIXI_CHOICE_STATS_REQ:
		((XIXI_Stats_Req_Pdu*)pdu_buffer)->decode_fixed(buf, length);
		break;
	case XIXI_CHOICE_HELLO_REQ:
		break;
	case XIXI_CHOICE_CREATE_WATCH_REQ:
		((XIXI_Create_Watch_Req_Pdu*)pdu_buffer)->decode_fixed(buf, length);
		break;
	case XIXI_CHOICE_CHECK_WATCH_REQ:
		((XIXI_Check_Watch_Req_Pdu*)pdu_buffer)->decode_fixed(buf, length);
		break;
	default:
		return false;
	}

	return true;
}

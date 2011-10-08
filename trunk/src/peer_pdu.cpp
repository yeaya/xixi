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

#include "peer_cc_pdu.h"
#include "peer_cache_pdu.h"
#include "peer_ha_pdu.h"

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
    case XIXI_CHOICE_UPDATE_BASE_REQ:
      ((XIXI_Update_Base_Req_Pdu*)pdu_buffer)->decode_fixed(buf, length);
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
    case XIXI_CHOICE_MUTEX_CREATE_REQ:
      ((XIXI_Mutex_Create_Req_Pdu*)pdu_buffer)->decode_fixed(buf, length);
      break;
    case XIXI_CHOICE_MUTEX_GET_STATUS_REQ:
      ((XIXI_Mutex_Get_Status_Req_Pdu*)pdu_buffer)->decode_fixed(buf, length);
    case XIXI_CHOICE_MUTEX_LOCK_REQ:
      ((XIXI_Mutex_Lock_Req_Pdu*)pdu_buffer)->decode_fixed(buf, length);
      break;
    case XIXI_CHOICE_MUTEX_UNLOCK_REQ:
      ((XIXI_Mutex_Unlock_Req_Pdu*)pdu_buffer)->decode_fixed(buf, length);
      break;
    case XIXI_CHOICE_MUTEX_DESTROY_REQ:
      ((XIXI_Mutex_Destroy_Req_Pdu*)pdu_buffer)->decode_fixed(buf, length);
      break;
    default:
      return false;
  }

  return true;
}

#define DECODE_PDU_4_HA(x) { \
  x* pdu = new x; \
  if (pdu != NULL) { \
    pdu->decode_fixed(buf, length); \
    pdu->choice = header.choice; \
  } \
  return pdu; \
}

XIXI_Pdu* XIXI_Pdu::decode_pdu_4_ha(XIXI_Pdu_Header& header, uint8_t* buf, uint32_t length) {
  switch (header.choice) {
    case XIXI_CHOICE_HA_PRESENT_REQ:
      DECODE_PDU_4_HA(XIXI_HA_Present_Req_Pdu);

    case XIXI_CHOICE_HA_PRESENT_RES:
      DECODE_PDU_4_HA(XIXI_HA_Present_Res_Pdu);

    case XIXI_CHOICE_HA_CREATE_COMMITTEE_REQ:
      DECODE_PDU_4_HA(XIXI_HA_Create_Committee_Req_Pdu);

    case XIXI_CHOICE_HA_CREATE_COMMITTEE_RES:
      DECODE_PDU_4_HA(XIXI_HA_Create_Committee_Res_Pdu);

    case XIXI_CHOICE_HA_ANNOUNCE_LEADER_REQ:
      DECODE_PDU_4_HA(XIXI_HA_Announce_Leader_Req_Pdu);

    case XIXI_CHOICE_HA_ANNOUNCE_LEADER_RES:
      DECODE_PDU_4_HA(XIXI_HA_Announce_Leader_Res_Pdu);

    case XIXI_CHOICE_HA_KEEPALIVE_REQ:
      DECODE_PDU_4_HA(XIXI_HA_Keepalive_Req_Pdu);

    case XIXI_CHOICE_HA_KEEPALIVE_RES:
      DECODE_PDU_4_HA(XIXI_HA_Keepalive_Res_Pdu);

    case XIXI_CHOICE_HA_MUTEX_CREATE_REQ:
      DECODE_PDU_4_HA(XIXI_HA_Mutex_Create_Req_Pdu);

    case XIXI_CHOICE_HA_MUTEX_LOCK_REQ:
      DECODE_PDU_4_HA(XIXI_HA_Mutex_Lock_Req_Pdu);

    case XIXI_CHOICE_HA_MUTEX_UNLOCK_REQ:
      DECODE_PDU_4_HA(XIXI_HA_Mutex_Unlock_Req_Pdu);

    case XIXI_CHOICE_HA_MUTEX_DESTROY_REQ:
      DECODE_PDU_4_HA(XIXI_HA_Mutex_Destroy_Req_Pdu);
    default:
      return NULL;
  }

  return NULL;
}

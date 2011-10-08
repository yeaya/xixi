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

#include "peer_xixi_ha.h"
#include "settings.h"
#include "currtime.h"
#include "stats.h"
#include <string.h>
#include "log.h"
#include "concurrent.h"
#include "server.h"

#define READ_BUFFER_HIGHWAT 8192
#define DATA_BUFFER_SIZE 2048
#define MAX_WRITE_IOV_COUNT 128
#define MAX_WRITE_SIZE 14000

#define OUT_STRING(x) out_string2(x"\r\n", sizeof(x"\r"))
//#define OUT_STRING(x) out_string2(x"\r\n", sizeof(x"\r\n") - 1)

#define LOG_TRACE2(x)   LOG_TRACE("Peer_XIXI_HA id=" << get_peer_id() << " " << x)
#define LOG_DEBUG2(x)   LOG_DEBUG("Peer_XIXI_HA id=" << get_peer_id() << " " << x)
#define LOG_INFO2(x)    LOG_INFO("Peer_XIXI_HA id=" << get_peer_id() << " " << x)
#define LOG_WARNING2(x) LOG_WARNING("Peer_XIXI_HA id=" << get_peer_id() << " " << x)
#define LOG_ERROR2(x)   LOG_ERROR("Peer_XIXI_HA id=" << get_peer_id() << " " << x)

#define PROCESS_READ_BODY_EXTRAS_SMALL(length) { next_data_len_ = length; set_state(PEER_STATE_READ_BODY_EXTRAS2); }

Peer_XIXI_HA::Peer_XIXI_HA(boost::asio::ip::tcp::socket* socket) :
    read_pdu_(NULL),
    state_(PEER_STATE_NEW_CMD),
    next_state_(PEER_STATE_NEW_CMD),
//    cache_item_(NULL),
    write_buf_total_(0),
    read_item_buf_(NULL),
    swallow_size_(0),
    next_data_len_(XIXI_PDU_HEAD_LENGTH) {

  LOG_INFO2("Peer_XIXI_HA::Peer_XIXI_HA()");
  socket_ = socket;
  rop_count_ = 0;
  wop_count_ = 0;
}

Peer_XIXI_HA::~Peer_XIXI_HA() {
  cleanup();
//  session_mgr_.detach_peer(this);

  LOG_INFO2("~Peer_XIXI_HA::Peer_XIXI_HA()");
}

void Peer_XIXI_HA::cleanup() {
//  if (cache_item_ != NULL) {
//    cache_mgr_.release_reference(cache_item_);
//    cache_item_ = NULL;
//  }
  if (socket_ != NULL) {
    delete socket_;
    socket_ = NULL;
  }
}

void Peer_XIXI_HA::write_error(xixi_reason error_code, uint32_t swallow, bool reply) {
  if (reply) {
    XIXI_Error_Res_Pdu::encode(write_pdu_fixed_buffer, error_code);
    Const_Data sd;
    sd.data = write_pdu_fixed_buffer;
    sd.size = XIXI_PDU_ERROR_RES_LENGTH;

    write_bufs_.push_back(sd);
    write_buf_total_ += sd.size;
    set_state(PEER_STATUS_WRITE);
    if (swallow > 0) {
      swallow_size_ = swallow;
      next_state_ = PEER_STATE_SWALLOW;
    } else {
      next_state_ = PEER_STATE_NEW_CMD;
    }
  } else {
    if (swallow > 0) {
      swallow_size_ = swallow;
      set_state(PEER_STATE_SWALLOW);
      next_state_ = PEER_STATE_NEW_CMD;
    } else {
      set_state(PEER_STATE_NEW_CMD);
      next_state_ = PEER_STATE_NEW_CMD;
    }
  }
}

void Peer_XIXI_HA::write_error(uint32_t request_id, xixi_reason error_code, uint32_t swallow, bool reply) {
  if (reply) {
    XIXI_Error_Res_With_ReqID_Pdu::encode(write_pdu_fixed_buffer, request_id, error_code);
    Const_Data sd;
    sd.data = write_pdu_fixed_buffer;
    sd.size = XIXI_PDU_ERROR_RES_WITH_REQID_LENGTH;

    write_bufs_.push_back(sd);
    write_buf_total_ += sd.size;
    set_state(PEER_STATUS_WRITE);
    if (swallow > 0) {
      swallow_size_ = swallow;
      next_state_ = PEER_STATE_SWALLOW;
    } else {
      next_state_ = PEER_STATE_NEW_CMD;
    }
  } else {
    if (swallow > 0) {
      swallow_size_ = swallow;
      set_state(PEER_STATE_SWALLOW);
      next_state_ = PEER_STATE_NEW_CMD;
    } else {
      set_state(PEER_STATE_NEW_CMD);
      next_state_ = PEER_STATE_NEW_CMD;
    }
  }
}

void Peer_XIXI_HA::write_one(uint32_t length) {
  Const_Data sd;
  sd.data = write_pdu_fixed_buffer;
  sd.size = length;

  write_bufs_.push_back(sd);

  write_buf_total_ += length;

  set_state(PEER_STATUS_WRITE);
  next_state_ = PEER_STATE_NEW_CMD;
}

void Peer_XIXI_HA::write_one(uint8_t* buf, uint32_t length) {
  Const_Data sd;
  sd.data = buf;
  sd.size = length;

  write_bufs_.push_back(sd);

  write_buf_total_ += length;

  set_state(PEER_STATUS_WRITE);
  next_state_ = PEER_STATE_NEW_CMD;
}

uint32_t Peer_XIXI_HA::process() {
  LOG_TRACE2("process length=" << read_buffer_.read_data_size_);
  uint32_t offset = 0;

  uint32_t tmp = 0;
  bool run = true;

  while (run) {

    LOG_TRACE2("process while state_" << (uint32_t)state_ << " offset=" << offset);

    switch (state_) {

    case PEER_STATE_NEW_CMD:
      reset_for_new_cmd();
      break;

    case PEER_STATE_READ_HEADER:
      tmp = process_header(read_buffer_.read_curr_, read_buffer_.read_data_size_);
      if (tmp > 0) {
        offset += tmp;
        read_buffer_.read_curr_ += tmp;
        read_buffer_.read_data_size_ -= tmp;
      } else {
        run = false;
      }
      break;

    case PEER_STATE_READ_BODY_FIXED:
      if (read_buffer_.read_data_size_ >= next_data_len_) {
        read_pdu_ = XIXI_Pdu::decode_pdu_4_ha(read_pdu_header_, read_buffer_.read_curr_, read_buffer_.read_data_size_);
        read_buffer_.read_curr_ += next_data_len_;
        read_buffer_.read_data_size_ -= next_data_len_;
        offset += next_data_len_;
        if (read_pdu_ != NULL) {
        //  read_pdu_ = (XIXI_Pdu*)read_pdu_fixed_body_buffer_;
          process_pdu_fixed(read_pdu_);
        } else {
          write_error(XIXI_REASON_UNKNOWN_COMMAND, 0, true);
          next_state_ = PEER_STATE_CLOSING;
        }
      } else {
        run = false;
      }
      break;

    case PEER_STATE_READ_BODY_EXTRAS:
      if (read_buffer_.read_data_size_ > 0) {
        tmp = read_buffer_.read_data_size_ > next_data_len_ ? next_data_len_ : read_buffer_.read_data_size_;
        if (read_item_buf_ != read_buffer_.read_curr_) {
          memmove(read_item_buf_, read_buffer_.read_curr_, tmp);
        }
        read_item_buf_ += tmp;
        next_data_len_ -= tmp;
        offset += tmp;
        read_buffer_.read_curr_ += tmp;
        read_buffer_.read_data_size_ -= tmp;
        if (next_data_len_ == 0) {
          process_pdu_extras(read_pdu_);
        }
      } else {
        run = false;
      }
      break;

    case PEER_STATE_READ_BODY_EXTRAS2:
      tmp = process_pdu_extras2(read_pdu_, read_buffer_.read_curr_, read_buffer_.read_data_size_);
      if (tmp > 0) {
        offset += tmp;
        read_buffer_.read_curr_ += tmp;
        read_buffer_.read_data_size_ -= tmp;
      } else {
        run = false;
      }
      break;

    case PEER_STATE_SWALLOW:
      if (swallow_size_ == 0) {
        set_state(PEER_STATE_NEW_CMD);
      } else if (read_buffer_.read_data_size_ > 0) {
        tmp = read_buffer_.read_data_size_ > swallow_size_ ? swallow_size_ : read_buffer_.read_data_size_;
        swallow_size_ -= tmp;
        offset += tmp;
        read_buffer_.read_curr_ += tmp;
        read_buffer_.read_data_size_ -= tmp;
      } else {
        run = false;
      }
      break;

    case PEER_STATUS_WRITE:
      set_state(next_state_);
      next_state_ = PEER_STATE_NEW_CMD;
      run = false;
      break;

    case PEER_STATE_CLOSING:
      cleanup();
      set_state(PEER_STATE_CLOSED);
      run = false;
      break;

    case PEER_STATE_CLOSED:
      break;

    default:
      assert(false);
      break;
    }
  }

  return offset;
}

void Peer_XIXI_HA::set_state(peer_state state) {
  LOG_TRACE("state change, from " << (uint32_t)state_ << " to " << (uint32_t)state);
  state_ = state;
}

uint32_t Peer_XIXI_HA::get_tcp_write_buf(vector<boost::asio::const_buffer>& bufs, uint32_t max_count, uint32_t max_size) {

//  lock_.lock();
  if (write_bufs_.empty() && !pdu_list_.empty()) {
    XIXI_Pdu* p = pdu_list_.front();
    XIXI_Mutex_Lock_Res_Pdu* pdu = (XIXI_Mutex_Lock_Res_Pdu*)p;
    XIXI_Mutex_Lock_Res_Pdu::encode(write_pdu_fixed_buffer, pdu->request_id, pdu->reason);
    pdu_list_.pop_front();
    delete pdu;

    Const_Data sd;
    sd.data = write_pdu_fixed_buffer;
    sd.size = XIXI_PDU_MUTEX_LOCK_RES_LENGTH;

    write_bufs_.push_back(sd);
    write_buf_total_ += XIXI_PDU_MUTEX_LOCK_RES_LENGTH;
  }
//  lock_.unlock();

  uint32_t size = 0;

  while (!write_bufs_.empty() && max_count > 0) {
    Const_Data& sd = write_bufs_.front();
    if (max_size > sd.size) {
      bufs.push_back(boost::asio::const_buffer(sd.data, sd.size));
      size += sd.size;
      max_size -= sd.size;
      write_bufs_.pop_front();
    } else if (max_size < sd.size) {
      bufs.push_back(boost::asio::const_buffer(sd.data, max_size));
      Const_Data sd1;
      sd1.data = (const uint8_t*)sd.data + max_size;
      sd1.size = sd.size - max_size;
      write_bufs_.front() = sd1;
      size += max_size;
      max_size = 0;
      break;
    } else {
      bufs.push_back(boost::asio::const_buffer(sd.data, sd.size));
      size += sd.size;
      max_size = 0;
      write_bufs_.pop_front();
      break;
    }
    max_count--;
  }
  return size;
}

void Peer_XIXI_HA::dispatch_message(XIXI_Pdu* msg) {
  lock_.lock();
  pdu_list_.push_back(msg);
  lock_.unlock();

//  if (connection_ != NULL) {
//    connection_->send_data();
//  }
}

uint32_t Peer_XIXI_HA::process_header(uint8_t* data, uint32_t data_len) {
  LOG_TRACE2("process_header choice=" << read_pdu_header_.choice);
  if (data_len >= XIXI_PDU_HEAD_LENGTH) {
    read_pdu_header_.decode(data);

    switch (read_pdu_header_.choice) {
      case XIXI_CHOICE_HA_PRESENT_REQ:
        next_data_len_ = XIXI_PDU_HA_PRESENT_REQ_BODY_LENGTH;
        set_state(PEER_STATE_READ_BODY_FIXED);
        break;

      case XIXI_CHOICE_HA_CREATE_COMMITTEE_REQ:
        next_data_len_ = XIXI_PDU_HA_CREATE_COMMITTEE_REQ_BODY_LENGTH;
        set_state(PEER_STATE_READ_BODY_FIXED);
        break;

      case XIXI_CHOICE_HA_ANNOUNCE_LEADER_REQ:
        next_data_len_ = XIXI_PDU_HA_ANNOUNCE_LEADER_REQ_BODY_LENGTH;
        set_state(PEER_STATE_READ_BODY_FIXED);
        break;

      case XIXI_CHOICE_HA_KEEPALIVE_REQ:
        next_data_len_ = XIXI_PDU_HA_KEEPALIVE_REQ_BODY_LENGTH;
        set_state(PEER_STATE_READ_BODY_FIXED);
        break;

      case XIXI_CHOICE_HA_MUTEX_CREATE_REQ:
        next_data_len_ = XIXI_PDU_HA_MUTEX_CREATE_REQ_BODY_LENGTH;
        set_state(PEER_STATE_READ_BODY_FIXED);
        break;

      case XIXI_CHOICE_HA_MUTEX_LOCK_REQ:
        next_data_len_ = XIXI_PDU_HA_MUTEX_LOCK_REQ_BODY_LENGTH;
        set_state(PEER_STATE_READ_BODY_FIXED);
        break;

      case XIXI_CHOICE_HA_MUTEX_UNLOCK_REQ:
        next_data_len_ = XIXI_PDU_HA_MUTEX_UNLOCK_REQ_BODY_LENGTH;
        set_state(PEER_STATE_READ_BODY_FIXED);
        break;
    
      case XIXI_CHOICE_HA_MUTEX_DESTROY_REQ:
        next_data_len_ = XIXI_PDU_HA_MUTEX_DESTROY_REQ_BODY_LENGTH;
        set_state(PEER_STATE_READ_BODY_FIXED);
        break;
      
      default:
        LOG_WARNING2("process_header unknown choice=" << read_pdu_header_.choice);
        write_error(XIXI_REASON_UNKNOWN_COMMAND, 0, true);
        next_state_ = PEER_STATE_CLOSING;
        break;
    }
    return XIXI_PDU_HEAD_LENGTH;
  } else {
    return 0;
  }  
}

void Peer_XIXI_HA::process_pdu_fixed(XIXI_Pdu* pdu) {
  LOG_TRACE2("process_pdu_fixed choice=" << read_pdu_header_.choice);
  switch (read_pdu_header_.choice) {
    case XIXI_CHOICE_HA_PRESENT_REQ:
      PROCESS_READ_BODY_EXTRAS_SMALL(((XIXI_HA_Present_Req_Pdu*)pdu)->get_extras_size());
      break;

    case XIXI_CHOICE_HA_CREATE_COMMITTEE_REQ:
      PROCESS_READ_BODY_EXTRAS_SMALL(((XIXI_HA_Create_Committee_Req_Pdu*)pdu)->get_extras_size());
      break;

    case XIXI_CHOICE_HA_ANNOUNCE_LEADER_REQ:
      PROCESS_READ_BODY_EXTRAS_SMALL(((XIXI_HA_Announce_Leader_Req_Pdu*)pdu)->get_extras_size());
      break;

    case XIXI_CHOICE_HA_KEEPALIVE_REQ:
      PROCESS_READ_BODY_EXTRAS_SMALL(((XIXI_HA_Keepalive_Req_Pdu*)pdu)->get_extras_size());
      break;

    case XIXI_CHOICE_HA_MUTEX_CREATE_REQ:
      PROCESS_READ_BODY_EXTRAS_SMALL(((XIXI_HA_Mutex_Create_Req_Pdu*)pdu)->key.size + ((XIXI_HA_Mutex_Create_Req_Pdu*)pdu)->session_id.size);
      break;

    case XIXI_CHOICE_HA_MUTEX_LOCK_REQ:
      PROCESS_READ_BODY_EXTRAS_SMALL(((XIXI_HA_Mutex_Lock_Req_Pdu*)pdu)->key.size + ((XIXI_HA_Mutex_Lock_Req_Pdu*)pdu)->session_id.size);
      break;

    case XIXI_CHOICE_HA_MUTEX_UNLOCK_REQ:
      PROCESS_READ_BODY_EXTRAS_SMALL(((XIXI_HA_Mutex_Unlock_Req_Pdu*)pdu)->key.size + ((XIXI_HA_Mutex_Unlock_Req_Pdu*)pdu)->session_id.size);
      break;

    case XIXI_CHOICE_HA_MUTEX_DESTROY_REQ:
      PROCESS_READ_BODY_EXTRAS_SMALL(((XIXI_HA_Mutex_Destroy_Req_Pdu*)pdu)->key.size + ((XIXI_HA_Mutex_Destroy_Req_Pdu*)pdu)->session_id.size);
      break;

    default:
      LOG_ERROR2("process_pdu_fixed unknown choice=" << read_pdu_header_.choice);
      set_state(PEER_STATE_CLOSING);
      break;
  }
}

void Peer_XIXI_HA::process_pdu_extras(XIXI_Pdu* pdu) {
  LOG_TRACE2("process_pdu_extras choice=" << read_pdu_header_.choice);
//  switch (read_pdu_header_.choice) {
//  default:
    LOG_ERROR2("process_pdu_extras unknown choice=" << read_pdu_header_.choice);
    set_state(PEER_STATE_CLOSING);
//    break;
//  }
}

uint32_t Peer_XIXI_HA::process_pdu_extras2(XIXI_Pdu* pdu, uint8_t* data, uint32_t data_length) {
  LOG_TRACE2("process_pdu_extras2 choice=" << read_pdu_header_.choice << " date_length=" << data_length);
  switch (read_pdu_header_.choice) {
    case XIXI_CHOICE_HA_PRESENT_REQ:
      return process_ha_present_req_pdu_extras((XIXI_HA_Present_Req_Pdu*)pdu, data, data_length);

    case XIXI_CHOICE_HA_CREATE_COMMITTEE_REQ:
      return process_ha_create_committee_req_pdu_extras((XIXI_HA_Create_Committee_Req_Pdu*)pdu, data, data_length);

    case XIXI_CHOICE_HA_ANNOUNCE_LEADER_REQ:
      return process_ha_announce_leader_req_pdu_extras((XIXI_HA_Announce_Leader_Req_Pdu*)pdu, data, data_length);

    case XIXI_CHOICE_HA_KEEPALIVE_REQ:
      return process_ha_keepalive_req_pdu_extras((XIXI_HA_Keepalive_Req_Pdu*)pdu, data, data_length);

    case XIXI_CHOICE_HA_MUTEX_CREATE_REQ:
      return process_ha_mutex_create_req_pdu_extras((XIXI_HA_Mutex_Create_Req_Pdu*)pdu, data, data_length);

    case XIXI_CHOICE_HA_MUTEX_LOCK_REQ:
      return process_ha_mutex_lock_req_pdu_extras((XIXI_HA_Mutex_Lock_Req_Pdu*)pdu, data, data_length);

    case XIXI_CHOICE_HA_MUTEX_UNLOCK_REQ:
      return process_ha_mutex_unlock_req_pdu_extras((XIXI_HA_Mutex_Unlock_Req_Pdu*)pdu, data, data_length);

    case XIXI_CHOICE_HA_MUTEX_DESTROY_REQ:
      return process_ha_mutex_destroy_req_pdu_extras((XIXI_HA_Mutex_Destroy_Req_Pdu*)pdu, data, data_length);
  }
  LOG_ERROR2("process_pdu_extras2 unknown choice=" << read_pdu_header_.choice);
  set_state(PEER_STATE_CLOSING);
  return 0;
}

uint32_t Peer_XIXI_HA::process_ha_present_req_pdu_extras(XIXI_HA_Present_Req_Pdu* pdu, uint8_t* data, uint32_t data_length) {
  LOG_INFO2("process_ha_present_req_pdu_extras");
  if (data_length < pdu->get_extras_size()) {
    return 0;
  }
  pdu->decode_extras(data, data_length);

  LOG_DEBUG2("Present req, id=" << pdu->server_id);

//  boost::asio::streambuf sb;

  XIXI_HA_Present_Res_Pdu res_pdu;
  svr_->process_ha_present_req_pdu(this, pdu, &res_pdu);

//  res_pdu.status = svr_->get_status();
//  res_pdu.server_id = svr_->get_server_id();

  uint32_t size = res_pdu.calc_encode_size();
  uint8_t* buf = cache_buf_.prepare(size);
  res_pdu.encode(buf);
//  res_pdu.server_id.set(server_id);
  write_one(buf, size);

//  xixi_reason ret = 0;//session_mgr_.mutex_create(this, pdu);

//  XIXI_Mutex_Create_Res_Pdu::encode(write_pdu_fixed_buffer, pdu->request_id, ret);
//  write_one(XIXI_PDU_MUTEX_CREATE_RES_LENGTH);

  return pdu->get_extras_size();
}

uint32_t Peer_XIXI_HA::process_ha_create_committee_req_pdu_extras(XIXI_HA_Create_Committee_Req_Pdu* pdu, uint8_t* data, uint32_t data_length) {
  LOG_INFO2("process_ha_create_committee_req_pdu_extras");
  if (data_length < pdu->get_extras_size()) {
    return 0;
  }
  pdu->decode_extras(data, data_length);

  LOG_DEBUG2("Create committee req, id=" << pdu->server_id);

  XIXI_HA_Create_Committee_Res_Pdu res_pdu;
  svr_->process_ha_create_committee_req_pdu(this, pdu, &res_pdu);

  uint32_t size = res_pdu.calc_encode_size();
  uint8_t* buf = cache_buf_.prepare(size);
  res_pdu.encode(buf);
  write_one(buf, size);

  return pdu->get_extras_size();
}

uint32_t Peer_XIXI_HA::process_ha_announce_leader_req_pdu_extras(XIXI_HA_Announce_Leader_Req_Pdu* pdu, uint8_t* data, uint32_t data_length) {
  LOG_INFO2("process_ha_announce_leader_req_pdu_extras");
  if (data_length < pdu->get_extras_size()) {
    return 0;
  }
  pdu->decode_extras(data, data_length);

  LOG_DEBUG2("Announce leader req, id=" << pdu->server_id);

  XIXI_HA_Announce_Leader_Res_Pdu res_pdu;
  svr_->process_ha_announce_leader_req_pdu(this, pdu, &res_pdu);

  uint32_t size = res_pdu.calc_encode_size();
  uint8_t* buf = cache_buf_.prepare(size);
  res_pdu.encode(buf);
  write_one(buf, size);

  return pdu->get_extras_size();
}

uint32_t Peer_XIXI_HA::process_ha_keepalive_req_pdu_extras(XIXI_HA_Keepalive_Req_Pdu* pdu, uint8_t* data, uint32_t data_length) {
  LOG_INFO2("process_ha_keepalive_req_pdu_extras");
  if (data_length < pdu->get_extras_size()) {
    return 0;
  }
  pdu->decode_extras(data, data_length);

  LOG_DEBUG2("Keepalive req, id=" << pdu->server_id);

  XIXI_HA_Keepalive_Res_Pdu res_pdu;
  svr_->process_ha_keepalive_req_pdu(this, pdu, &res_pdu);

  uint32_t size = res_pdu.calc_encode_size();
  uint8_t* buf = cache_buf_.prepare(size);
  res_pdu.encode(buf);
  write_one(buf, size);

  return pdu->get_extras_size();
}

uint32_t Peer_XIXI_HA::process_ha_mutex_create_req_pdu_extras(XIXI_HA_Mutex_Create_Req_Pdu* pdu, uint8_t* data, uint32_t data_length) {
  LOG_INFO2("process_ha_mutex_create_req_pdu_extras");
  if (data_length < pdu->key.size + pdu->session_id.size) {
    return 0;
  }
  pdu->key.data = data;
  pdu->session_id.data = data + pdu->key.size;

  LOG_DEBUG2("Create ha mutex " << string((char*)data, pdu->key.size));

  xixi_reason ret = 0;//session_mgr_.mutex_create(this, pdu);

  XIXI_Mutex_Create_Res_Pdu::encode(write_pdu_fixed_buffer, pdu->request_id, ret);
  write_one(XIXI_PDU_MUTEX_CREATE_RES_LENGTH);

  return pdu->key.size + pdu->session_id.size;
}

uint32_t Peer_XIXI_HA::process_ha_mutex_lock_req_pdu_extras(XIXI_HA_Mutex_Lock_Req_Pdu* pdu, uint8_t* data, uint32_t data_length) {
  LOG_INFO2("process_ha_mutex_lock_req_pdu_extras");

  if (data_length < pdu->key.size + pdu->session_id.size) {
    return 0;
  }
  pdu->key.data = data;
  pdu->session_id.data = data + pdu->key.size;

  LOG_DEBUG2("Lock ha mutex " << string((char*)data, pdu->key.size));

  xixi_reason ret = 0;//session_mgr_.mutex_lock(this, pdu);

  if (ret != XIXI_REASON_WAIT_FOR_ME) {
    XIXI_Mutex_Lock_Res_Pdu::encode(write_pdu_fixed_buffer, pdu->request_id, ret);
    write_one(XIXI_PDU_MUTEX_LOCK_RES_LENGTH);
  } else {
    set_state(PEER_STATE_NEW_CMD);
    next_state_ = PEER_STATE_NEW_CMD;
  }

  return pdu->key.size + pdu->session_id.size;
}

uint32_t Peer_XIXI_HA::process_ha_mutex_unlock_req_pdu_extras(XIXI_HA_Mutex_Unlock_Req_Pdu* pdu, uint8_t* data, uint32_t data_length) {
  LOG_INFO2("process_mutex_unlock_req_pdu_extras");

  if (data_length < pdu->key.size + pdu->session_id.size) {
    return 0;
  }
  pdu->key.data = data;
  pdu->session_id.data = data + pdu->key.size;

  LOG_DEBUG2("Unlock ha mutex " << string((char*)data, pdu->key.size));

  xixi_reason ret = 0;//session_mgr_.mutex_unlock(this, pdu);

  XIXI_Mutex_Unlock_Res_Pdu::encode(write_pdu_fixed_buffer, pdu->request_id, ret);
  write_one(XIXI_PDU_MUTEX_UNLOCK_RES_LENGTH);

  return pdu->key.size + pdu->session_id.size;
}

uint32_t Peer_XIXI_HA::process_ha_mutex_destroy_req_pdu_extras(XIXI_HA_Mutex_Destroy_Req_Pdu* pdu, uint8_t* data, uint32_t data_length) {
  LOG_INFO2("process_ha_mutex_destroy_req_pdu_extras");
  if (data_length < pdu->key.size + pdu->session_id.size) {
    return 0;
  }
  pdu->key.data = data;
  pdu->session_id.data = data + pdu->key.size;

  LOG_DEBUG2("Destroy ha mutex " << string((char*)pdu->key.data, pdu->key.size));

  xixi_reason ret = 0;//session_mgr_.mutex_destroy(this, pdu);

  XIXI_Mutex_Destroy_Res_Pdu::encode(write_pdu_fixed_buffer, pdu->request_id, ret);
  write_one(XIXI_PDU_MUTEX_DESTROY_RES_LENGTH);

  return pdu->key.size + pdu->session_id.size;
}

void Peer_XIXI_HA::reset_for_new_cmd() {
  if (read_pdu_ != NULL) {
    delete read_pdu_;
    read_pdu_ = NULL;
  }
//  if (cache_item_ != NULL) {
//    cache_mgr_.release_reference(cache_item_);
//    cache_item_ = NULL;
//  }
//  cache_buf_.reset();
  write_buf_total_ = 0;
  read_item_buf_ = NULL;
  next_data_len_ = XIXI_PDU_HEAD_LENGTH;
  set_state(PEER_STATE_READ_HEADER);
}

void Peer_XIXI_HA::start(uint8_t* data, uint32_t data_length) {
  if (read_buffer_.get_read_buf_size() >= data_length) {
    memcpy(read_buffer_.get_read_buf(), data, data_length);
    read_buffer_.read_data_size_ += data_length;
//    peer_ = peer;
//    peer_->set_connection(this);
    uint32_t len = process();
    read_buffer_.handle_processed();

    if (!is_closed()) {
      uint32_t size = try_write();
      if (size == 0) {
        try_read();
      }
    } else {
      LOG_INFO2("Peer_XIXI_HA::start closed");
  //    on_connection_closed(this);
    //  peer_ = NULL;
      socket_->get_io_service().post(boost::bind(&Peer_XIXI_HA::destroy, this));
    }
  } else {
    LOG_INFO2("Peer_XIXI_HA::start invalid parameter:data_length=" << data_length);
//    if ( != NULL) {
//      on_connection_closed(this);
    //  peer_ = NULL;
//    }
    socket_->get_io_service().post(boost::bind(&Peer_XIXI_HA::destroy, this));
  }
}

void Peer_XIXI_HA::handle_read(const boost::system::error_code& err, size_t length) {
  LOG_TRACE2("handle_read length=" << length << " err=" << err.message() << " err_value=" << err.value());

  lock_.lock();
  --rop_count_;
  if (!err) {
    read_buffer_.read_data_size_ += length;
    uint32_t len = process();
    read_buffer_.handle_processed();

    if (!is_closed()) {
      uint32_t size = try_write();
      if (size == 0) {
        try_read();
      }
    } else {
      LOG_INFO2("handle_read peer is closed, rop_count_=" << rop_count_ << " wop_count_=" << wop_count_);
    }
  } else {
    LOG_INFO2("handle_read error rop_count_=" << rop_count_ << " wop_count_=" << wop_count_ << " err=" << err);
  }

  if (rop_count_ + wop_count_ == 0) {
    LOG_INFO2("handle_read destroy rop_count_=" << rop_count_ << " wop_count_=" << wop_count_);
//    this->on_connection_closed(NULL);
  //  peer_ = NULL;
    socket_->get_io_service().post(boost::bind(&Peer_XIXI_HA::destroy, this));
  } else {
    LOG_TRACE2("handle_read end rop_count_=" << rop_count_ << " wop_count_=" << wop_count_);
  }
  lock_.unlock();
}

void Peer_XIXI_HA::handle_write(const boost::system::error_code& err) {
  LOG_TRACE2("handle_write err=" << err.message() << " err_value=" << err.value());

  lock_.lock();
  --wop_count_;
  if (!err) {
    uint32_t size = try_write();
    if (size == 0) {
      uint32_t len = process();
      read_buffer_.handle_processed();
      if (!is_closed()) {
        uint32_t size = try_write();
        if (size == 0) {
          try_read();
        }
      } else {
        LOG_INFO2("handle_write peer is closed, rop_count_=" << rop_count_ << " wop_count_=" << wop_count_);
      }
    }
  }

  if (rop_count_ + wop_count_ == 0) {
    LOG_INFO2("handle_write destroy rop_count_=" << rop_count_ << " wop_count_=" << wop_count_);
//    on_connection_closed(NULL);
  //  peer_ = NULL;
    socket_->get_io_service().post(boost::bind(&Peer_XIXI_HA::destroy, this));
  } else {
    LOG_TRACE2("handle_write end rop_count_=" << rop_count_ << " wop_count_=" << wop_count_);
  }
  lock_.unlock();
}

void Peer_XIXI_HA::try_read() {
  if (rop_count_ == 0) {
    ++rop_count_;
    socket_->async_read_some(boost::asio::buffer(read_buffer_.get_read_buf(), read_buffer_.get_read_buf_size()),
      make_custom_alloc_handler(read_allocator_,
        boost::bind(&Peer_XIXI_HA::handle_read, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred)));
    LOG_TRACE2("try_read async_read_some get_read_buf_size=" << read_buffer_.get_read_buf_size());
  }
}

uint32_t Peer_XIXI_HA::try_write() {
  if (wop_count_ == 0) {
    write_buf_.clear();
    uint32_t size = get_tcp_write_buf(write_buf_, MAX_WRITE_IOV_COUNT, MAX_WRITE_SIZE);
    if (size > 0) {
      ++wop_count_;
      async_write(*socket_, write_buf_,
        make_custom_alloc_handler(write_allocator_,
            boost::bind(&Peer_XIXI_HA::handle_write, this,
              boost::asio::placeholders::error)));
      LOG_TRACE2("try_write async_write write_buf.count=" << write_buf_.size() << " size=" << size);
    }
    return size;
  }
  return 0;
}

void Peer_XIXI_HA::send_data() {

  LOG_TRACE2("send_data");

  socket_->get_io_service().post(boost::bind(&Peer_XIXI_HA::do_write, this));
//  lock_.lock();
//  try_write();
//  lock_.unlock();
}

void Peer_XIXI_HA::do_write() {
  LOG_TRACE2("do_write");
  lock_.lock();
  try_write();
  lock_.unlock();
}
/*
void Peer_XIXI_HA::close() {
  socket_->cancel();
    return;
}
*/
void Peer_XIXI_HA::destroy(Peer_XIXI_HA* peer) {
  delete peer;
}


/**
 * \file ProtocolHandler.cpp
 * \brief ProtocolHandler class source file.
 *
 * Copyright (c) 2013, Ford Motor Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the Ford Motor Company nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <memory.h>

#include "protocol_handler/session_observer.h"
#include "protocol_handler/protocol_handler_impl.h"
#include "protocol_handler/message_from_mobile_app_handler.h"
#include "protocol_handler/messages_to_mobile_app_handler.h"
#include "utils/macro.h"

namespace protocol_handler {

log4cxx::LoggerPtr ProtocolHandlerImpl::logger_ =
  log4cxx::LoggerPtr(log4cxx::Logger::getLogger("ProtocolHandler"));

ProtocolHandlerImpl::ProtocolHandlerImpl(
  transport_manager::TransportManager* transport_manager)
  : protocol_observers_(),
    session_observer_(0),
    transport_manager_(transport_manager),
    handle_messages_from_mobile_app_(NULL),
    handle_messages_to_mobile_app_(NULL),
    kPeriodForNaviAck(5) {
  LOG4CXX_TRACE_ENTER(logger_);

  handle_messages_from_mobile_app_ = new threads::Thread(
    "MessagesFromMobileAppHandler", new MessagesFromMobileAppHandler(this));
  handle_messages_from_mobile_app_->startWithOptions(
    threads::ThreadOptions(threads::Thread::kMinStackSize));

  handle_messages_to_mobile_app_ = new threads::Thread(
    "MessagesToMobileAppHandler", new MessagesToMobileAppHandler(this));
  handle_messages_to_mobile_app_->startWithOptions(
    threads::ThreadOptions(threads::Thread::kMinStackSize));

  LOG4CXX_TRACE_EXIT(logger_);
}

ProtocolHandlerImpl::~ProtocolHandlerImpl() {
  handle_messages_from_mobile_app_->stop();
  delete handle_messages_from_mobile_app_;
  handle_messages_from_mobile_app_ = NULL;

  handle_messages_to_mobile_app_->stop();
  delete handle_messages_to_mobile_app_;
  handle_messages_to_mobile_app_ = NULL;

  protocol_observers_.clear();
  session_observer_ = 0;
  transport_manager_ = 0;
}

void ProtocolHandlerImpl::AddProtocolObserver(ProtocolObserver* observer) {
  if (!observer) {
    LOG4CXX_ERROR(logger_, "Invalid (NULL) pointer to IProtocolObserver.");
    return;
  }

  protocol_observers_.insert(observer);
}

void ProtocolHandlerImpl::RemoveProtocolObserver(ProtocolObserver* observer) {
  if (!observer) {
    LOG4CXX_ERROR(logger_, "Invalid (NULL) pointer to IProtocolObserver.");
    return;
  }

  protocol_observers_.erase(observer);
}

void ProtocolHandlerImpl::set_session_observer(SessionObserver* observer) {
  if (!observer) {
    LOG4CXX_ERROR(logger_, "Invalid (NULL) pointer to ISessionObserver.");
    return;
  }

  session_observer_ = observer;
}

void ProtocolHandlerImpl::SendEndSessionNAck(
  ConnectionID connection_id ,
  unsigned int session_id,
  unsigned char service_type) {
  LOG4CXX_TRACE_ENTER(logger_);

  ProtocolPacket packet(PROTOCOL_VERSION_2, COMPRESS_OFF, FRAME_TYPE_CONTROL,
                        0x0, FRAME_DATA_END_SESSION_NACK, session_id, 0, 0);

  if (RESULT_OK == SendFrame(connection_id, packet)) {
    LOG4CXX_INFO(logger_, "sendStartSessionAck() - BT write OK");
  } else {
    LOG4CXX_ERROR(logger_, "sendStartSessionAck() - BT write FAIL");
  }

  LOG4CXX_TRACE_EXIT(logger_);
}

void ProtocolHandlerImpl::SendStartSessionAck(
  ConnectionID connection_id ,
  unsigned char session_id,
  unsigned char protocol_version,
  unsigned int hash_code,
  unsigned char service_type) {
  LOG4CXX_TRACE_ENTER(logger_);

  ProtocolPacket packet(PROTOCOL_VERSION_2, COMPRESS_OFF, FRAME_TYPE_CONTROL,
                        service_type, FRAME_DATA_START_SESSION_ACK,
                        session_id, 0, hash_code);

  if (RESULT_OK == SendFrame(connection_id, packet)) {
    LOG4CXX_INFO(
      logger_,
      "sendStartSessionAck() for connection " <<
      connection_id
      << " for service_type " << service_type <<
      " session_id " << session_id);
  } else {
    LOG4CXX_ERROR(logger_, "sendStartSessionAck() - BT write FAIL");
  }

  LOG4CXX_TRACE_EXIT(logger_);
}

void ProtocolHandlerImpl::SendStartSessionNAck(
  ConnectionID connection_id ,
  unsigned char service_type) {
  LOG4CXX_TRACE_ENTER(logger_);

  unsigned char versionFlag = PROTOCOL_VERSION_1;

  ProtocolPacket packet(versionFlag, COMPRESS_OFF, FRAME_TYPE_CONTROL,
                        service_type, FRAME_DATA_START_SESSION_NACK, 0x0, 0, 0);

  if (RESULT_OK == SendFrame(connection_id, packet)) {
    LOG4CXX_INFO(logger_, "sendStartSessionAck() - BT write OK");
  } else {
    LOG4CXX_ERROR(logger_, "sendStartSessionAck() - BT write FAIL");
  }

  LOG4CXX_TRACE_EXIT(logger_);
}

void ProtocolHandlerImpl::SendMessageToMobileApp(
  const RawMessagePtr& message) {
  LOG4CXX_TRACE_ENTER(logger_);
  if (!message) {
    LOG4CXX_ERROR(logger_,
                  "Invalid message for sending to mobile app is received.");
    LOG4CXX_TRACE_EXIT(logger_);
    return;
  }
  messages_to_mobile_app_.push(message);
  LOG4CXX_TRACE_EXIT(logger_);
}

void ProtocolHandlerImpl::OnTMMessageReceived(
  const RawMessagePtr message) {
  LOG4CXX_TRACE_ENTER(logger_);

  if (message.valid()) {
    LOG4CXX_INFO_EXT(logger_,
                     "Received from TM " << message->data()
                     << " with connection id " << message->connection_key());
    messages_from_mobile_app_.push(message);
  } else {
    LOG4CXX_ERROR(
      logger_,
      "Invalid incoming message received in"
      << " ProtocolHandler from Transport Manager.");
  }

  LOG4CXX_TRACE_EXIT(logger_);
}

void ProtocolHandlerImpl::OnTMMessageReceiveFailed(
  const transport_manager::DataReceiveError& error) {
  // TODO(PV): implement
  LOG4CXX_ERROR(logger_, "Received error on attemping to recieve message.")
}

void ProtocolHandlerImpl::NotifySubscribers(const RawMessagePtr& message) {
  for (ProtocolObservers::iterator it = protocol_observers_.begin();
       protocol_observers_.end() != it;
       ++it) {
    (*it)->OnMessageReceived(message);
  }
}

void ProtocolHandlerImpl::OnTMMessageSend(const RawMessagePtr message) {
  // TODO(PV): implement if needed.
  LOG4CXX_INFO(logger_, "Sending message finished successfully.");

  // TODO(PK): Check if it RegisterAppInterfaceResponse with error was sent
  if (false) {
    for (ProtocolObservers::iterator it = protocol_observers_.begin();
         protocol_observers_.end() != it;
         ++it) {
      // TODO(PK): Paste valid app_id taken from message
      (*it)->OnMobileMessageSent(NULL /*Valid message*/);
    }
  }
}

void ProtocolHandlerImpl::OnTMMessageSendFailed(
  const transport_manager::DataSendError& error,
  const RawMessagePtr& message)  {
  // TODO(PV): implement
  LOG4CXX_ERROR(logger_, "Sending message " <<
                message-> data() << " failed.");
}

RESULT_CODE ProtocolHandlerImpl::SendFrame(
  ConnectionID connection_id ,
  const ProtocolPacket& packet) {
  LOG4CXX_TRACE_ENTER(logger_);
  if (!packet.packet()) {
    LOG4CXX_ERROR(logger_, "Failed to create packet.");

    LOG4CXX_TRACE_EXIT(logger_);
    return RESULT_FAIL;
  }

  LOG4CXX_INFO_EXT(
    logger_,
    "Packet to be sent: " << packet.packet()
    << " of size: " << packet.packet_size());

  if (!session_observer_) {
    LOG4CXX_WARN(logger_, "No session_observer_ set.");
    return RESULT_FAIL;
  }

  transport_manager::ConnectionUID connection_uid = 0;
  unsigned char session_id = 0;
  session_observer_->PairFromKey(connection_id,
                                 &connection_uid,
                                 &session_id);

  RawMessagePtr message_to_send(new RawMessage(
                                  connection_uid,
                                  PROTOCOL_VERSION_2,
                                  packet.packet(),
                                  packet.packet_size()));

  LOG4CXX_INFO(logger_, "Message to send with connection id "
               << connection_uid);

  if (transport_manager_) {
    transport_manager_->SendMessageToDevice(message_to_send);
  } else {
    LOG4CXX_WARN(logger_, "No Transport Manager found.");
    LOG4CXX_TRACE_EXIT(logger_);
    return RESULT_FAIL;
  }

  LOG4CXX_TRACE_EXIT(logger_);
  return RESULT_OK;
}

RESULT_CODE ProtocolHandlerImpl::SendSingleFrameMessage(
  ConnectionID connection_id ,
  const unsigned char session_id, unsigned int protocol_version,
  const unsigned char service_type, const unsigned int data_size,
  const unsigned char* data, const bool compress) {
  LOG4CXX_TRACE_ENTER(logger_);

  unsigned char versionF = PROTOCOL_VERSION_1;
  if (2 == protocol_version) {
    versionF = PROTOCOL_VERSION_2;
  }

  ProtocolPacket packet(versionF, compress, FRAME_TYPE_SINGLE, service_type, 0,
                        session_id, data_size, message_counters_[session_id]++,
                        data);

  LOG4CXX_TRACE_EXIT(logger_);
  return SendFrame(connection_id, packet);
}

RESULT_CODE ProtocolHandlerImpl::SendMultiFrameMessage(
  ConnectionID connection_id ,
  const unsigned char session_id, unsigned int protocol_version,
  const unsigned char service_type, const unsigned int data_size,
  const unsigned char* data, const bool compress,
  const unsigned int maxdata_size) {
  LOG4CXX_TRACE_ENTER(logger_);
  RESULT_CODE retVal = RESULT_OK;

  LOG4CXX_INFO_EXT(
    logger_, " data size " << data_size << " maxdata_size " << maxdata_size);

  unsigned char versionF = PROTOCOL_VERSION_1;
  if (2 == protocol_version) {
    versionF = PROTOCOL_VERSION_2;
  }

  int numOfFrames = 0;
  int lastdata_size = 0;

  if (data_size % maxdata_size) {
    numOfFrames = (data_size / maxdata_size) + 1;
    lastdata_size = data_size % maxdata_size;
  } else {
    numOfFrames = data_size / maxdata_size;
    lastdata_size = maxdata_size;
  }

  LOG4CXX_INFO_EXT(
    logger_,
    "Data size " << data_size << " of " << numOfFrames
    << " frames with last frame " << lastdata_size);

  unsigned char* outDataFirstFrame = new unsigned char[FIRST_FRAME_DATA_SIZE];
  outDataFirstFrame[0] = data_size >> 24;
  outDataFirstFrame[1] = data_size >> 16;
  outDataFirstFrame[2] = data_size >> 8;
  outDataFirstFrame[3] = data_size;

  outDataFirstFrame[4] = numOfFrames >> 24;
  outDataFirstFrame[5] = numOfFrames >> 16;
  outDataFirstFrame[6] = numOfFrames >> 8;
  outDataFirstFrame[7] = numOfFrames;

  ProtocolPacket firstPacket(versionF, compress, FRAME_TYPE_FIRST,
                             service_type, 0,
                             session_id, FIRST_FRAME_DATA_SIZE,
                             ++message_counters_[session_id],
                             outDataFirstFrame);

  retVal = SendFrame(connection_id, firstPacket);

  LOG4CXX_INFO_EXT(logger_, "First frame is sent.");

  delete[] outDataFirstFrame;

  unsigned char* outDataFrame = new unsigned char[maxdata_size];

  for (unsigned int i = 0; i < numOfFrames; i++) {
    if (i != (numOfFrames - 1)) {
      memcpy(outDataFrame, data + (maxdata_size * i), maxdata_size);

      ProtocolPacket packet(versionF, compress, FRAME_TYPE_CONSECUTIVE,
                            service_type, ((i % FRAME_DATA_MAX_VALUE) + 1),
                            session_id, maxdata_size,
                            message_counters_[session_id], outDataFrame);

      retVal = SendFrame(connection_id, packet);
      if (RESULT_FAIL == retVal) {
        break;
      }
    } else {
      memcpy(outDataFrame, data + (maxdata_size * i), lastdata_size);

      ProtocolPacket packet(versionF, compress, FRAME_TYPE_CONSECUTIVE,
                            service_type, 0x0, session_id, lastdata_size,
                            message_counters_[session_id], outDataFrame);

      retVal = SendFrame(connection_id, packet);
    }
  }

  delete[] outDataFrame;

  LOG4CXX_TRACE_EXIT(logger_);
  return retVal;
}

RESULT_CODE ProtocolHandlerImpl::HandleMessage(
  ConnectionID connection_id ,
  ProtocolPacket* packet) {
  LOG4CXX_TRACE_ENTER(logger_);

  switch (packet->frame_type()) {
    case FRAME_TYPE_CONTROL: {
      LOG4CXX_INFO(logger_, "handleMessage() - case FRAME_TYPE_CONTROL");

      LOG4CXX_TRACE_EXIT(logger_);
      return HandleControlMessage(connection_id, packet);
    }
    case FRAME_TYPE_SINGLE: {
      LOG4CXX_INFO(
        logger_,
        "FRAME_TYPE_SINGLE: of size " << packet->data_size()
        << ";message " << packet -> data());

      if (!session_observer_) {
        LOG4CXX_ERROR(
          logger_,
          "Cannot handle message from Transport"
          << " Manager: ISessionObserver doesn't exist.");

        LOG4CXX_TRACE_EXIT(logger_);
        return RESULT_FAIL;
      }

      int connection_key = session_observer_->KeyFromPair(
                             connection_id,
                             packet->session_id());

      RawMessagePtr raw_message(new RawMessage(
                                  connection_key,
                                  packet->version(),
                                  packet->data(),
                                  packet->data_size(),
                                  packet->service_type()));

      NotifySubscribers(raw_message);
      break;
    }
    case FRAME_TYPE_FIRST:
    case FRAME_TYPE_CONSECUTIVE: {
      LOG4CXX_INFO(logger_, "handleMessage() - case FRAME_TYPE_CONSECUTIVE");

      LOG4CXX_TRACE_EXIT(logger_);
      return HandleMultiFrameMessage(connection_id, packet);
    }
    default: {
      LOG4CXX_WARN(logger_, "handleMessage() - case default!!!");
    }
  }

  LOG4CXX_TRACE_EXIT(logger_);
  return RESULT_OK;
}

RESULT_CODE ProtocolHandlerImpl::HandleMultiFrameMessage(
  ConnectionID connection_id ,
  ProtocolPacket* packet) {
  LOG4CXX_TRACE_ENTER(logger_);

  if (!session_observer_) {
    LOG4CXX_ERROR(logger_, "No ISessionObserver set.");

    LOG4CXX_TRACE_EXIT(logger_);
    return RESULT_FAIL;
  }

  LOG4CXX_INFO_EXT(
    logger_, "Packet " << packet << "; session_id " << packet -> session_id());

  int key = session_observer_->KeyFromPair(connection_id,
            packet->session_id());

  if (packet->frame_type() == FRAME_TYPE_FIRST) {
    LOG4CXX_INFO(logger_, "handleMultiFrameMessage() - FRAME_TYPE_FIRST");

    // const unsigned char* data = packet -> getData();
    unsigned int total_data_bytes = packet->data()[0] << 24;
    total_data_bytes |= packet->data()[1] << 16;
    total_data_bytes |= packet->data()[2] << 8;
    total_data_bytes |= packet->data()[3];

    packet->set_total_data_bytes(total_data_bytes);

    incomplete_multi_frame_messages_[key] = packet;
  } else {
    LOG4CXX_INFO(logger_, "handleMultiFrameMessage() - Consecutive frame");

    std::map<int, ProtocolPacket*>::iterator it =
      incomplete_multi_frame_messages_.find(key);

    if (it == incomplete_multi_frame_messages_.end()) {
      LOG4CXX_ERROR(
        logger_, "Frame of multiframe message for non-existing session id");

      LOG4CXX_TRACE_EXIT(logger_);
      return RESULT_FAIL;
    }

    // LOG4CXX_INFO(mLogger, "Found message " << it->second->)

    if (it->second->appendData(packet->data(), packet->data_size())
        != RESULT_OK) {
      LOG4CXX_ERROR(logger_,
                    "Failed to append frame for multiframe message.");

      LOG4CXX_TRACE_EXIT(logger_);
      return RESULT_FAIL;
    }

    if (packet->frame_data() == FRAME_DATA_LAST_FRAME) {
      LOG4CXX_INFO(logger_, "Last frame of multiframe message size "
                   << packet->data_size() << "; connection key " << key);
      if (protocol_observers_.empty()) {
        LOG4CXX_ERROR(
          logger_,
          "Cannot handle multiframe message: no IProtocolObserver is set.");

        LOG4CXX_TRACE_EXIT(logger_);
        return RESULT_FAIL;
      }

      ProtocolPacket* completePacket = it->second;
      RawMessage* rawMessage = new RawMessage(
        key, completePacket->version(), completePacket->data(),
        completePacket->total_data_bytes());

      NotifySubscribers(rawMessage);

      incomplete_multi_frame_messages_.erase(it);
    }
  }

  LOG4CXX_TRACE_EXIT(logger_);
  return RESULT_OK;
}

RESULT_CODE ProtocolHandlerImpl::HandleControlMessage(
  ConnectionID connection_id,
  const ProtocolPacket* packet) {
  LOG4CXX_TRACE_ENTER(logger_);

  if (!session_observer_) {
    LOG4CXX_ERROR(logger_, "ISessionObserver is not set.");

    LOG4CXX_TRACE_EXIT(logger_);
    return RESULT_FAIL;
  }

  if (packet->frame_data() == FRAME_DATA_END_SESSION) {
    LOG4CXX_INFO(logger_, "handleControlMessage() - FRAME_DATA_END_SESSION");

    unsigned char currentsession_id = packet->session_id();

    unsigned int hash_code = 0;
    if (packet->version() == 2) {
      hash_code = packet->message_id();
    }

    bool success = true;
    int sessionhash_code = session_observer_->OnSessionEndedCallback(
                             connection_id,
                             currentsession_id,
                             hash_code,
                             packet->service_type());

    if (-1 != sessionhash_code) {
      if (2 == packet->version()) {
        if (packet->message_id() != sessionhash_code) {
          success = false;
        }
      }
    } else {
      success = false;
    }

    if (success) {
      message_counters_.erase(currentsession_id);
    } else {
      LOG4CXX_INFO_EXT(
        logger_,
        "Refused to end session " << packet -> service_type() << " type.");
      SendEndSessionNAck(connection_id, currentsession_id,
                         packet->service_type());
    }
  }

  if (packet->frame_data() == FRAME_DATA_START_SESSION) {
    LOG4CXX_INFO(logger_,
                 "handleControlMessage() - FRAME_DATA_START_SESSION");
    LOG4CXX_INFO_EXT(
      logger_, "Version 2 " << (packet -> version() == PROTOCOL_VERSION_2));

    int session_id = session_observer_->OnSessionStartedCallback(
                       connection_id,
                       packet->service_type());
    if (-1 != session_id) {
      SendStartSessionAck(
        connection_id, session_id, packet->version(),
        session_observer_->KeyFromPair(
          connection_id, session_id),
        packet->service_type());
    } else {
      LOG4CXX_INFO_EXT(
        logger_,
        "Refused to create session " << packet -> service_type() << " type.");
      SendStartSessionNAck(connection_id,
                           packet->service_type());
    }
  }

  LOG4CXX_TRACE_EXIT(logger_);
  return RESULT_OK;
}

void ProtocolHandlerImpl::SendFramesNumber(int connection_key,
    int number_of_frames) {
  LOG4CXX_INFO(logger_, "SendFramesNumber MobileNaviAck for session "
               << connection_key);

  ProtocolPacket packet(PROTOCOL_VERSION_2,
                        COMPRESS_OFF,
                        FRAME_TYPE_CONTROL,
                        SERVICE_TYPE_NAVI,
                        FRAME_DATA_MOBILE_NAVE_ACK,
                        connection_key,
                        0,
                        number_of_frames);

  RESULT_CODE send_result = SendFrame(connection_key, packet);
  if (RESULT_OK == send_result) {
    LOG4CXX_INFO(logger_, "MobileNaviAck sent successfully.");
  } else {
    LOG4CXX_ERROR(logger_, "MobileNaviAck failed to be sent.");
  }
}

}  // namespace protocol_handler

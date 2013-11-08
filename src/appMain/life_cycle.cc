/**
* \file signals.cc
* \brief Signal (i.e. SIGINT) handling.
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

#include "./life_cycle.h"
#include "utils/signals.h"
#include "config_profile/profile.h"

namespace main_namespace {
log4cxx::LoggerPtr LifeCycle::logger_ = log4cxx::LoggerPtr(
    log4cxx::Logger::getLogger("appMain"));

LifeCycle::LifeCycle()
  : transport_manager_(NULL)
  , protocol_handler_(NULL)
  , mmh_(NULL)
  , connection_handler_(NULL)
  , app_manager_(NULL)
  , hmi_handler_(NULL)
  , media_manager_(NULL)
#ifdef QT_HMI
  , dbus_adapter_(NULL)
  , dbus_adapter_thread_(NULL) {
#endif  // QT_HMI
#ifdef WEB_HMI
  , mb_adapter_(NULL)
  , message_broker_(NULL)
  , message_broker_server_(NULL)
  , mb_thread_(NULL)
  , mb_server_thread_(NULL)
  , mb_adapter_thread_(NULL) {
#endif  // WEB_HMI
}

LifeCycle* LifeCycle::instance() {
  static LifeCycle instance;
  return &instance;
}

bool LifeCycle::StartComponents() {
  LOG4CXX_INFO(logger_, "LifeCycle::StartComponents()");
  transport_manager_ =
    transport_manager::TransportManagerDefault::Instance();
  DCHECK(transport_manager_);

  protocol_handler_ =
    new protocol_handler::ProtocolHandlerImpl(transport_manager_);
  DCHECK(protocol_handler_);

  mmh_ =
    mobile_message_handler::MobileMessageHandlerImpl::instance();
  DCHECK(mmh_);

  connection_handler_ =
    connection_handler::ConnectionHandlerImpl::instance();
  DCHECK(connection_handler_);

  app_manager_ =
    application_manager::ApplicationManagerImpl::instance();
  DCHECK(app_manager_);

  hmi_handler_ =
    hmi_message_handler::HMIMessageHandlerImpl::instance();
  DCHECK(hmi_handler_)

  transport_manager_->SetProtocolHandler(protocol_handler_);
  transport_manager_->AddEventListener(protocol_handler_);
  transport_manager_->AddEventListener(connection_handler_);

  mmh_->set_protocol_handler(protocol_handler_);
  hmi_handler_->set_message_observer(app_manager_);

  media_manager_ = media_manager::MediaManagerImpl::getMediaManager();

  protocol_handler_->set_session_observer(connection_handler_);
  protocol_handler_->AddProtocolObserver(mmh_);
  protocol_handler_->AddProtocolObserver(media_manager_);
  media_manager_->SetProtocolHandler(protocol_handler_);

  media_manager::MediaManagerImpl::getMediaManager()->setVideoRedecoder(NULL);

  if ("socket" == profile::Profile::instance()->video_server_type()) {
    media_manager::MediaManagerImpl::getMediaManager()->setConsumer(
       new media_manager::video_stream_producer_consumer::SocketVideoServer());
  } else if ("pipe" == profile::Profile::instance()->video_server_type()) {
    media_manager::MediaManagerImpl::getMediaManager()->setConsumer(
       new media_manager::video_stream_producer_consumer::PipeVideoServer());
  }

  // TODO(PV): add media manager

  connection_handler_->set_transport_manager(transport_manager_);
  connection_handler_->set_connection_handler_observer(app_manager_);

  // It's important to initialise TM after setting up listener chain
  // [TM -> CH -> AM], otherwise some events from TM could arrive at nowhere
  transport_manager_->Init();

  app_manager_->set_mobile_message_handler(mmh_);
  mmh_->AddMobileMessageListener(app_manager_);
  app_manager_->set_connection_handler(connection_handler_);
  app_manager_->set_hmi_message_handler(hmi_handler_);

  return true;
}

#ifdef WEB_HMI
bool LifeCycle::InitMessageSystem() {
  message_broker_ =
    NsMessageBroker::CMessageBroker::getInstance();
  if (!message_broker_) {
    LOG4CXX_INFO(logger_, " Wrong pMessageBroker pointer!");
    return false;
  }

  message_broker_server_ =
    new NsMessageBroker::TcpServer(
    profile::Profile::instance()->server_address(),
    profile::Profile::instance()->server_port(),
    message_broker_);
  if (!message_broker_server_) {
    LOG4CXX_INFO(logger_, " Wrong pJSONRPC20Server pointer!");
    return false;
  }
  message_broker_->startMessageBroker(message_broker_server_);
  if (!networking::init()) {
    LOG4CXX_INFO(logger_, " Networking initialization failed!");
    return false;
  }

  if (!message_broker_server_->Bind()) {
    LOG4CXX_FATAL(logger_, "Bind failed!");
    return false;
  } else {
    LOG4CXX_INFO(logger_, "Bind successful!");
  }

  if (!message_broker_server_->Listen()) {
    LOG4CXX_FATAL(logger_, "Listen failed!");
    return false;
  } else {
    LOG4CXX_INFO(logger_, " Listen successful!");
  }

  mb_adapter_ =
    new hmi_message_handler::MessageBrokerAdapter(
    hmi_message_handler::HMIMessageHandlerImpl::instance(),
    profile::Profile::instance()->server_address(),
    profile::Profile::instance()->server_port());

  hmi_message_handler::HMIMessageHandlerImpl::instance()->AddHMIMessageAdapter(
    mb_adapter_);
  if (!mb_adapter_->Connect()) {
    LOG4CXX_INFO(logger_, "Cannot connect to remote peer!");
    return false;
  }

  LOG4CXX_INFO(logger_, "Start CMessageBroker thread!");
  mb_thread_ = new System::Thread(
    new System::ThreadArgImpl<NsMessageBroker::CMessageBroker>(
      *message_broker_, &NsMessageBroker::CMessageBroker::MethodForThread,
      NULL));
  mb_thread_->Start(false);

  LOG4CXX_INFO(logger_, "Start MessageBroker TCP server thread!");
  mb_server_thread_  = new System::Thread(
    new System::ThreadArgImpl<NsMessageBroker::TcpServer>(
      *message_broker_server_, &NsMessageBroker::TcpServer::MethodForThread,
      NULL));
  mb_server_thread_->Start(false);

  LOG4CXX_INFO(logger_, "StartAppMgr JSONRPC 2.0 controller receiver thread!");
  mb_adapter_thread_  = new System::Thread(
    new System::ThreadArgImpl<hmi_message_handler::MessageBrokerAdapter>(
      *mb_adapter_,
      &hmi_message_handler::MessageBrokerAdapter::MethodForReceiverThread,
      NULL));
  mb_adapter_thread_->Start(false);

  mb_adapter_->registerController();
  mb_adapter_->SubscribeTo();

  return true;
}
#endif  // WEB_HMI

#ifdef QT_HMI
/**
 * Initialize DBus component
 * @return true if success otherwise false.
 */
bool LifeCycle::InitMessageSystem() {
  log4cxx::LoggerPtr logger = log4cxx::LoggerPtr(
      log4cxx::Logger::getLogger("appMain"));

  dbus_adapter_ = new hmi_message_handler::DBusMessageAdapter(
      hmi_message_handler::HMIMessageHandlerImpl::instance());

  hmi_message_handler::HMIMessageHandlerImpl::instance()->AddHMIMessageAdapter(
      dbus_adapter_);
  if (!dbus_adapter_->Init()) {
    LOG4CXX_INFO(logger, "Cannot init DBus service!");
    return false;
  }

  dbus_adapter_->SubscribeTo();

  LOG4CXX_INFO(logger, "Start DBusMessageAdapter thread!");
  dbus_adapter_thread_ = new System::Thread(
      new System::ThreadArgImpl<hmi_message_handler::DBusMessageAdapter>(
          *dbus_adapter_,
          &hmi_message_handler::DBusMessageAdapter::MethodForReceiverThread,
          NULL));
  dbus_adapter_thread_->Start(false);

  return true;
}
#endif  // QT_HMI

void LifeCycle::StopComponents(int params) {
  utils::ResetSubscribeToTerminateSignal();
  LOG4CXX_INFO(logger_, "Destroying Application Manager.");
  instance()->hmi_handler_->set_message_observer(NULL);
  instance()->connection_handler_->set_connection_handler_observer(NULL);
  instance()->mmh_->RemoveMobileMessageListener(instance()->app_manager_);
  instance()->app_manager_->~ApplicationManagerImpl();

  LOG4CXX_INFO(logger_, "Destroying Connection Handler.");
  instance()->transport_manager_->RemoveEventListener(
    instance()->connection_handler_);
  instance()->protocol_handler_->set_session_observer(NULL);
  instance()->connection_handler_->~ConnectionHandlerImpl();

  LOG4CXX_INFO(logger_, "Destroying Mobile Message Handler.");
  instance()->protocol_handler_->RemoveProtocolObserver(instance()->mmh_);
  delete instance()->mmh_;

  LOG4CXX_INFO(logger_, "Destroying Protocol Handler");
  instance()->transport_manager_->SetProtocolHandler(NULL);
  instance()->transport_manager_->RemoveEventListener(
    instance()->protocol_handler_);
  instance()->media_manager_->SetProtocolHandler(NULL);
  delete instance()->protocol_handler_;
  delete instance()->media_manager_;

  LOG4CXX_INFO(logger_, "Fasten your seatbelts, we're going to remove TM");
  delete instance()->transport_manager_;

  LOG4CXX_INFO(logger_, "Destroying HMI Message Handler and adapter.");
#ifdef QT_HMI
  instance()->hmi_handler_->RemoveHMIMessageAdapter(instance()->dbus_adapter_);
  instance()->dbus_adapter_thread_->Stop();
  instance()->dbus_adapter_thread_->Join();
  delete instance()->dbus_adapter_;
#endif  // QT_HMI
#ifdef WEB_HMI
  instance()->hmi_handler_->RemoveHMIMessageAdapter(instance()->mb_adapter_);
  instance()->mb_adapter_->unregisterController();
  instance()->mb_adapter_thread_->Stop();
  instance()->mb_adapter_thread_->Join();
  instance()->mb_adapter_->Close();
  delete instance()->mb_adapter_;
#endif  // WEB_HMI
  instance()->hmi_handler_->~HMIMessageHandlerImpl();

#ifdef WEB_HMI
  LOG4CXX_INFO(logger_, "Destroying Message Broker");
  instance()->mb_server_thread_->Stop();
  instance()->mb_server_thread_->Join();
  instance()->mb_thread_->Stop();
  instance()->mb_thread_->Join();
  instance()->message_broker_server_->Close();
  instance()->message_broker_->stopMessageBroker();
  delete instance()->mb_server_thread_;
  instance()->message_broker_->~CMessageBroker();

  networking::cleanup();
#endif  // WEB_HMI

  utils::ForwardSignal();
}
}  //  namespace main_namespace

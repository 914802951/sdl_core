/**
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

#include "utils/logger.h"

#include "application_manager/commands/command_request_impl.h"
#include "application_manager/request_controller.h"

namespace application_manager {

namespace request_controller {
using namespace sync_primitives;

log4cxx::LoggerPtr logger_ =
  log4cxx::LoggerPtr(log4cxx::Logger::getLogger("RequestController"));

RequestController::RequestController()
  : watchdog_(NULL) {
  LOG4CXX_INFO(logger_, "RequestController::RequestController()");
  watchdog_ = request_watchdog::RequestWatchdog::instance();
  watchdog_->AddListener(this);
}

RequestController::~RequestController() {
  LOG4CXX_INFO(logger_, "RequestController::~RequestController()");
  request_list_.clear();

  if (watchdog_) {
    watchdog_->RemoveListener(this);
    watchdog_->~Watchdog();
  }
}

bool RequestController::addRequest(const Request& request) {
  LOG4CXX_INFO(logger_, "RequestController::addRequest()");

  bool result = true;
  {
    AutoLock auto_lock(request_list_lock_);

    const commands::CommandRequestImpl* request_impl =
      (static_cast<commands::CommandRequestImpl*>(&(*request)));

    if (true ==
        watchdog_->timeScaleMaxRequestExceed(request_impl->connection_key())) {
      LOG4CXX_ERROR(logger_, "Application requests count exceed limit");
      result = false;
      // remove all app request
    } else {

      request_list_.push_back(request);

      LOG4CXX_INFO(logger_, "Adding request to watchdog. Default timeout is "
                   << request_impl->default_timeout());

      watchdog_->addRequest(new request_watchdog::RequestInfo(
                              request_impl->function_id(),
                              request_impl->connection_key(),
                              request_impl->correlation_id(),
                              request_impl->default_timeout()));

      LOG4CXX_INFO(logger_, "Added request to watchdog.");
      LOG4CXX_INFO(logger_, "RequestController size is " << request_list_.size());
    }
  }

  return result;
}

void RequestController::terminateRequest(unsigned int mobile_correlation_id) {
  LOG4CXX_INFO(logger_, "RequestController::terminateRequest()");

  {
    AutoLock auto_lock(request_list_lock_);
    std::list<Request>::iterator it = request_list_.begin();
    for (; request_list_.end() != it; ++it) {
      const commands::CommandRequestImpl* request_impl =
        (static_cast<commands::CommandRequestImpl*>(&(*(*it))));
      if (request_impl->correlation_id() == mobile_correlation_id) {
        watchdog_->removeRequest(
          request_impl->connection_key(), request_impl->correlation_id());
        request_list_.erase(it);
        break;
      }
    }
  }
}

void RequestController::terminateAppRequests(unsigned int app_id) {
  LOG4CXX_INFO(logger_, "RequestController::terminateAppRequests()");

  {
    AutoLock auto_lock(request_list_lock_);
    std::list<Request>::iterator it = request_list_.begin();
    for (; request_list_.end() != it; ++it) {
      const commands::CommandRequestImpl* request_impl =
        (static_cast<commands::CommandRequestImpl*>(&(*(*it))));
      if (request_impl->connection_key() == app_id) {
        watchdog_->removeRequest(
          request_impl->connection_key(), request_impl->correlation_id());
        it = request_list_.erase(it);
      }
    }
  }
}

void RequestController::updateRequestTimeout(unsigned int connection_key,
                                             unsigned int mobile_correlation_id,
                                             unsigned int new_timeout) {
  LOG4CXX_INFO(logger_, "RequestController::updateRequestTimeout()");

  watchdog_->updateRequestTimeout(connection_key,
                                  mobile_correlation_id,
                                  new_timeout);
}

void RequestController::onTimeoutExpired(request_watchdog::RequestInfo info) {
  LOG4CXX_INFO(logger_, "RequestController::onTimeoutExpired()");

  commands::CommandRequestImpl* request_impl = NULL;
  {
    AutoLock auto_lock(request_list_lock_);
    std::list<Request>::iterator it = request_list_.begin();
    for (; request_list_.end() != it; ++it) {
      request_impl = (static_cast<commands::CommandRequestImpl*>(&(*(*it))));
      if (request_impl->correlation_id() == info.correlationID_ &&
          request_impl->connection_key() == info.connectionID_) {
        LOG4CXX_INFO(logger_, "Timeout for request id " << info.correlationID_ <<
                     " of application " << info.connectionID_ << " expired");
        break;
      }
    }
  }

  if (request_impl) {
    request_impl->onTimeOut();
  }
}

}  //  namespace request_controller

}  //  namespace application_manager

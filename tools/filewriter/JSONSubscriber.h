/*
 * JSONSubscriber.h
 *
 *  Created on: 25 May 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_JSONSUBSCRIBER_H_
#define TOOLS_FILEWRITER_JSONSUBSCRIBER_H_

#include <string>

#include <boost/thread.hpp>

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "zmq/zmq.hpp"

#include "IJSONCallback.h"
#include "JSONMessage.h"
#include "WorkQueue.h"

namespace filewriter
{

  class JSONSubscriber
  {
  public:
    JSONSubscriber(const std::string& socketName);
    virtual ~JSONSubscriber();
    void subscribe();
    void registerCallback(boost::shared_ptr<IJSONCallback> cb);

  private:
    void listenTask();

    LoggerPtr logger_;
    zmq::context_t *zmqContext_; // Global ZMQ context
    zmq::socket_t *zmqSocket_;   // ZMQ socket
    std::string zmqSocketName_;  // String name of the ZMQ socket
    boost::thread *thread_;
    std::vector<boost::shared_ptr<IJSONCallback> > callbacks_;
  };

} /* namespace filewriter */

#endif /* TOOLS_FILEWRITER_JSONSUBSCRIBER_H_ */

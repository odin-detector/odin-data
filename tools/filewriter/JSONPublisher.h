/*
 * Publisher.h
 *
 *  Created on: 31 May 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_JSONPUBLISHER_H_
#define TOOLS_FILEWRITER_JSONPUBLISHER_H_

#include <boost/shared_ptr.hpp>

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "zmq/zmq.hpp"

#include "JSONMessage.h"

namespace filewriter
{

  class JSONPublisher
  {
  public:
    JSONPublisher(const std::string& socketName);
    virtual ~JSONPublisher();
    void setSocketName(const std::string& socketName);
    void connect();
    void disconnect();
    size_t publish(boost::shared_ptr<JSONMessage> msg);

  private:
    LoggerPtr logger_;
    zmq::context_t *zmqContext_; // Global ZMQ context
    zmq::socket_t *zmqSocket_;   // ZMQ socket
    std::string zmqSocketName_;  // String name of the ZMQ socket
  };

} /* namespace filewriter */

#endif /* TOOLS_FILEWRITER_JSONPUBLISHER_H_ */

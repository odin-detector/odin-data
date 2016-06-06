/*
 * Publisher.cpp
 *
 *  Created on: 31 May 2016
 *      Author: gnx91527
 */

#include <JSONPublisher.h>

namespace filewriter
{

  JSONPublisher::JSONPublisher(const std::string& socketName) :
    zmqSocketName_(socketName)
  {
    // Setup logging for the class
    logger_ = Logger::getLogger("JSONPublisher");
    logger_->setLevel(Level::getAll());
    LOG4CXX_TRACE(logger_, "JSONPublisher constructor.");

    // Get the global ZMQ context
    zmqContext_ = new zmq::context_t; // Global ZMQ context
    // Create a zmq socket for publishing
    zmqSocket_ = new zmq::socket_t(*zmqContext_, ZMQ_PUB);
  }

  JSONPublisher::~JSONPublisher()
  {
    LOG4CXX_TRACE(logger_, "JSONPublisher destructor.");
    zmqSocket_->close();
    delete zmqSocket_;
    zmqContext_->close();
    delete zmqContext_;
  }

  void JSONPublisher::setSocketName(const std::string& socketName)
  {
    LOG4CXX_TRACE(logger_, "Setting socket name to " << socketName);
    zmqSocketName_ = socketName;
  }

  void JSONPublisher::connect()
  {
    LOG4CXX_TRACE(logger_, "Connecting to " << zmqSocketName_);
    zmqSocket_->connect(zmqSocketName_.c_str());
  }

  void JSONPublisher::disconnect()
  {
    LOG4CXX_TRACE(logger_, "Disconnecting from " << zmqSocketName_);
    zmqSocket_->disconnect(zmqSocketName_.c_str());
  }

  size_t JSONPublisher::publish(boost::shared_ptr<JSONMessage> msg)
  {
    LOG4CXX_DEBUG(logger_, "  sending: " << msg->toString());
    // Serialise and publish a JSONMessage object
    size_t nbytes = zmqSocket_->send(msg->toString().c_str(), msg->toString().size() + 1);
    LOG4CXX_DEBUG(logger_, "  sent: " << nbytes << " bytes");
    return nbytes;
  }

} /* namespace filewriter */

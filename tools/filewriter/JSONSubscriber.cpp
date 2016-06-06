/*
 * JSONSubscriber.cpp
 *
 *  Created on: 25 May 2016
 *      Author: gnx91527
 */

#include <JSONSubscriber.h>

namespace filewriter
{

  JSONSubscriber::JSONSubscriber(const std::string& socketName) :
    zmqSocketName_(socketName),
    thread_(0)
  {
    // Setup logging for the class
    logger_ = Logger::getLogger("FileWriter");
    logger_->setLevel(Level::getAll());
    LOG4CXX_TRACE(logger_, "JSONSubscriber constructor.");

    // Get the global ZMQ context
    zmqContext_ = new zmq::context_t; // Global ZMQ context
    // Create a zmq socket for subscribing
    zmqSocket_ = new zmq::socket_t(*zmqContext_, ZMQ_SUB);
  }

  JSONSubscriber::~JSONSubscriber()
  {
    // TODO Auto-generated destructor stub
  }

  void JSONSubscriber::subscribe()
  {
    thread_ = new boost::thread(&JSONSubscriber::listenTask, this);
  }

  void JSONSubscriber::registerCallback(boost::shared_ptr<IJSONCallback> cb)
  {
    // Record the callback pointer
    callbacks_.push_back(cb);
    // Start the callback thread
    cb->start();
  }

  void JSONSubscriber::listenTask()
  {
    zmqSocket_->connect(zmqSocketName_.c_str());
    zmqSocket_->setsockopt(ZMQ_SUBSCRIBE, "", strlen(""));
    zmq::pollitem_t poll_item;
    poll_item.socket = *zmqSocket_;
    poll_item.events = ZMQ_POLLIN;
    poll_item.fd = 0;
    poll_item.revents = 0;

    // The polling loop. Polls on all elements in poll_item
    // Stop the loop by setting keep_running=false
    // Loop automatically ends if notification_count > user option "frames"
    unsigned long notification_count = 0;
    bool keep_running = true;
    LOG4CXX_DEBUG(logger_, "Entering ZMQ polling loop (" << zmqSocketName_ << ")");
    while (keep_running)
    {
        // Always reset the last raw events before polling
        poll_item.revents = 0;

        // Do the poll with a 10ms timeout (i.e. 10Hz poll)
        zmq::poll(&poll_item, 1, 1000);

        // Check for poll error and exit the loop is one is found
        if (poll_item.revents & ZMQ_POLLERR)
        {
            LOG4CXX_ERROR(logger_, "Got ZMQ error in polling. Quitting polling loop.");
            keep_running = false;
        }
        // Do work if a message is received
        else if (poll_item.revents & ZMQ_POLLIN)
        {
            LOG4CXX_DEBUG(logger_, "Reading data from ZMQ socket");
            notification_count++;
            zmq::message_t msg;
            zmqSocket_->recv(&msg); // Read the message from the ready socket

            std::string msg_str(reinterpret_cast<char*>(msg.data()), msg.size()-1);
            LOG4CXX_DEBUG(logger_, "Parsing JSON msg string: " << msg_str);

            boost::shared_ptr<JSONMessage> msgJSON = boost::shared_ptr<JSONMessage>(new JSONMessage(msg_str));
            // Loop over callbacks, plaing the message onto each queue
            std::vector<boost::shared_ptr<IJSONCallback> >::iterator cbIter;
            for (cbIter = callbacks_.begin(); cbIter != callbacks_.end(); ++cbIter){
              (*cbIter)->getWorkQueue()->add(msgJSON);
            }

        } else
        {
            // No new data
          //LOG4CXX_WARN(logger_, "No data...");
        }

        // Quit the loop if we have received the desired number of frames
        //if (notification_count >= vm["frames"].as<unsigned int>()) keep_running=false;
    }

  }


} /* namespace filewriter */

/*
 * FrameDecoderZMQ.h
 *
 *  Created on: April 20, 2017
 *      Author: Alan Greer
 */

#ifndef INCLUDE_FRAMEDECODER_ZMQ_H_
#define INCLUDE_FRAMEDECODER_ZMQ_H_

#include <queue>
#include <map>

#include <stddef.h>
#include <stdint.h>
#include <netinet/in.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include <log4cxx/logger.h>
using namespace log4cxx;
using namespace log4cxx::helpers;
#include <DebugLevelLogger.h>

#include "FrameDecoder.h"

namespace FrameReceiver
{
class FrameDecoderZMQ : public FrameDecoder
{
public:

  FrameDecoderZMQ() :
      FrameDecoder()
  {
  };

  virtual ~FrameDecoderZMQ() = 0;

  virtual void* get_next_message_buffer(void) = 0;
  virtual FrameReceiveState process_message(size_t bytes_received) = 0;
  virtual void frame_meta_data(int meta) = 0;

};

inline FrameDecoderZMQ::~FrameDecoderZMQ() {};

typedef boost::shared_ptr<FrameDecoderZMQ> FrameDecoderZMQPtr;

} // namespace FrameReceiver
#endif /* INCLUDE_FRAMEDECODER_ZMQ_H_ */

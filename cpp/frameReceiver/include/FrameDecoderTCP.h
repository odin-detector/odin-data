/*
 * FrameDecoderTCP.h
 *
 *  Created on: March 30, 2020
 *
 */

#ifndef INCLUDE_FRAMEDECODER_TCP_H_
#define INCLUDE_FRAMEDECODER_TCP_H_

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
class FrameDecoderTCP : public FrameDecoder
{
public:

  FrameDecoderTCP() :
      FrameDecoder()
  {
  };

  virtual ~FrameDecoderTCP() = 0;

  virtual void* get_next_message_buffer(void) = 0;
  virtual const size_t get_next_message_size(void) const = 0;

  virtual FrameReceiveState process_message(size_t bytes_received) = 0;

  void* current_raw_buffer_;

};

inline FrameDecoderTCP::~FrameDecoderTCP() {};

typedef boost::shared_ptr<FrameDecoderTCP> FrameDecoderTCPPtr;

} // namespace FrameReceiver
#endif /* INCLUDE_FRAMEDECODER_TCP_H_ */

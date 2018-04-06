/*
 * FrameDecoder.h
 *
 *  Created on: Feb 24, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef INCLUDE_FRAMEDECODER_H_
#define INCLUDE_FRAMEDECODER_H_

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

#include "IpcMessage.h"
#include "OdinDataException.h"
#include "SharedBufferManager.h"

namespace FrameReceiver
{

  const std::string CONFIG_DECODER_ENABLE_PACKET_LOGGING = "enable_packet_logging";
  const std::string CONFIG_DECODER_FRAME_TIMEOUT_MS = "frame_timeout_ms";

class FrameDecoderException : public OdinData::OdinDataException
{
public:
  FrameDecoderException(const std::string what) : OdinData::OdinDataException(what) { };
};

typedef boost::function<void(int, int)> FrameReadyCallback;
typedef std::queue<int> EmptyBufferQueue;
typedef std::map<int, int> FrameBufferMap;

class FrameDecoder
{
public:

  enum FrameReceiveState
  {
    FrameReceiveStateEmpty,
    FrameReceiveStateIncomplete,
    FrameReceiveStateComplete,
    FrameReceiveStateTimedout,
    FrameReceiveStateError
  };

  FrameDecoder();

  virtual ~FrameDecoder() = 0;

  virtual void init(LoggerPtr& logger, OdinData::IpcMessage& config_msg);
  virtual void request_configuration(const std::string param_prefix,
      OdinData::IpcMessage& config_reply);
  virtual const size_t get_frame_buffer_size(void) const = 0;
  virtual const size_t get_frame_header_size(void) const = 0;

  void register_buffer_manager(OdinData::SharedBufferManagerPtr buffer_manager);
  void register_frame_ready_callback(FrameReadyCallback callback);
  void push_empty_buffer(int buffer_id);
  const size_t get_num_empty_buffers(void) const;
  const size_t get_num_mapped_buffers(void) const;
  void drop_all_buffers(void);
  const unsigned int get_frame_timeout_ms(void) const;
  const unsigned int get_num_frames_timedout(void) const;
  virtual void monitor_buffers(void) = 0;

protected:
  LoggerPtr logger_;  //!< Pointer to the logging facility

  bool enable_packet_logging_;  //!< Flag to enable packet logging by decoder
  LoggerPtr packet_logger_;     //!< Pointer to the packet logging facility

  OdinData::SharedBufferManagerPtr buffer_manager_; //!< Pointer to the shared buffer manager
  FrameReadyCallback   ready_callback_;             //!< Callback for frames ready to be processed

  EmptyBufferQueue empty_buffer_queue_; //!< Queue of empty buffers ready for use
  FrameBufferMap   frame_buffer_map_;   //!< Map of buffers currently receiving frame data

  unsigned int frame_timeout_ms_; //!< Incomplete frame timeout in ms
  unsigned int frames_timedout_;  //!< Number of frames timed out in decoder
};

inline FrameDecoder::~FrameDecoder() {};

typedef boost::shared_ptr<FrameDecoder> FrameDecoderPtr;

} // namespace FrameReceiver
#endif /* INCLUDE_FRAMEDECODER_H_ */

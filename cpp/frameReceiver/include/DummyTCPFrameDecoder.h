#ifndef INCLUDE_DUMMYFRAMEDECODERTCP_H_
#define INCLUDE_DUMMYFRAMEDECODERTCP_H_

#include <iostream>
#include <stdint.h>
#include <time.h>

#include "FrameDecoderTCP.h"

namespace FrameReceiver {

namespace DummyTcpFrameDecoderDefaults {
  const int frame_number = -1;
  const int buffer_id = 0;
  const size_t max_size = 1000; // maximum size of a frame for dropped buffer, incl. header size
  const size_t header_size = 100;
  const int num_buffers = 5;
} // namespace DummyTcpFrameDecoderDefaults

class DummyTCPFrameDecoder : public FrameDecoderTCP {
public:
  DummyTCPFrameDecoder();
  ~DummyTCPFrameDecoder();

  int get_version_major();
  int get_version_minor();
  int get_version_patch();
  std::string get_version_short();
  std::string get_version_long();

  void monitor_buffers(void);
  void get_status(const std::string param_prefix,
                  OdinData::IpcMessage &status_msg);

  void init(LoggerPtr &logger, OdinData::IpcMessage &config_msg);
  void request_configuration(const std::string param_prefix,
                             OdinData::IpcMessage &config_reply);

  void *get_next_message_buffer(void);
  const size_t get_next_message_size(void) const;
  FrameDecoder::FrameReceiveState process_message(size_t bytes_received);

  const size_t get_frame_buffer_size(void) const;
  const size_t get_frame_header_size(void) const;

  void reset_statistics(void);

  void *get_packet_header_buffer(void);

  uint32_t get_frame_number(void) const;
  uint32_t get_packet_number(void) const;

private:
  boost::shared_ptr<void> frame_buffer_;
  size_t frames_dropped_;
  size_t frames_sent_;
  size_t read_so_far_;
  int current_frame_number_;
  int current_frame_buffer_id_;
  size_t buffer_size_;
  size_t header_size_;
  size_t frame_size_;
  unsigned int num_buffers_;
  FrameDecoder::FrameReceiveState receive_state_;
};

} // namespace FrameReceiver
#endif /* INCLUDE_DUMMYFRAMEDECODERTCP_H_ */

/*!
 * FrameReceiverConfig.h - FrameReceiver configuration class
 *
 * This class implements a simple storage container for FrameReceiver
 * configuration parameters derived from command-line options and file
 * parsing.
 *
 *  Created on: Feb 4, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef FRAMERECEIVERCONFIG_H_
#define FRAMERECEIVERCONFIG_H_

#include <map>
#include <vector>
#include <iostream>

#include <boost/scoped_ptr.hpp>

#include "FrameReceiverDefaults.h"
#include "IpcMessage.h"

using namespace OdinData;

namespace FrameReceiver
{

  const std::string CONFIG_MAX_BUFFER_MEM = "max_buffer_mem";
  const std::string CONFIG_DECODER_PATH = "decoder_path";
  const std::string CONFIG_DECODER_TYPE = "decoder_type";
  const std::string CONFIG_DECODER_CONFIG = "decoder_config";
  const std::string CONFIG_RX_TYPE = "rx_type";
  const std::string CONFIG_CTRL_ENDPOINT = "ctrl_endpoint";
  const std::string CONFIG_RX_ENDPOINT = "rx_endpoint";
  const std::string CONFIG_FRAME_READY_ENDPOINT = "frame_ready_endpoint";
  const std::string CONFIG_FRAME_RELEASE_ENDPOINT = "frame_release_endpoint";
  const std::string CONFIG_RX_PORTS = "rx_ports";
  const std::string CONFIG_RX_ADDRESS = "rx_address";
  const std::string CONFIG_RX_RECV_BUFFER_SIZE = "rx_recv_buffer_size";
  const std::string CONFIG_SHARED_BUFFER_NAME = "shared_buffer_name";
  const std::string CONFIG_FRAME_TIMEOUT_MS = "frame_timeout_ms";
  const std::string CONFIG_FRAME_COUNT = "frame_count";
  const std::string CONFIG_ENABLE_PACKET_LOGGING = "enable_packet_logging";
  const std::string CONFIG_FORCE_RECONFIG = "force_reconfig";
  const std::string CONFIG_DEBUG = "debug_level";

class FrameReceiverConfig
{
public:

  FrameReceiverConfig() :
  
      max_buffer_mem_(Defaults::default_max_buffer_mem),
      decoder_path_(Defaults::default_decoder_path),
      decoder_type_(Defaults::default_decoder_type),
      decoder_config_(new IpcMessage()),
      rx_type_(Defaults::default_rx_type),
      rx_address_(Defaults::default_rx_address),
      rx_recv_buffer_size_(Defaults::default_rx_recv_buffer_size),
      io_threads_(Defaults::default_io_threads),
      rx_channel_endpoint_(Defaults::default_rx_chan_endpoint),
      ctrl_channel_endpoint_(Defaults::default_ctrl_chan_endpoint),
      frame_ready_endpoint_(Defaults::default_frame_ready_endpoint),
      frame_release_endpoint_(Defaults::default_frame_release_endpoint),
      shared_buffer_name_(Defaults::default_shared_buffer_name),
      frame_timeout_ms_(Defaults::default_frame_timeout_ms),
      enable_packet_logging_(Defaults::default_enable_packet_logging),
      force_reconfig_(Defaults::default_force_reconfig)
  {
    tokenize_port_list(rx_ports_, Defaults::default_rx_port_list);
  };

  void tokenize_port_list(std::vector<uint16_t>& port_list, const std::string port_list_str)
  {

    port_list.clear();
    const std::string delimiter(",");
    std::size_t start = 0, end = 0;

    while ( end != std::string::npos)
    {
      end = port_list_str.find(delimiter, start);
      const char* port_str = port_list_str.substr(start, (end == std::string::npos) ? std::string::npos : end - start).c_str();
      start = ((end > (std::string::npos - delimiter.size())) ? std::string::npos : end + delimiter.size());

      uint16_t port = static_cast<uint16_t>(strtol(port_str, NULL, 0));
      if (port != 0)
      {
        port_list.push_back( port );
      }
    }
  }

  static Defaults::RxType map_rx_name_to_type(std::string& rx_name)
  {
    Defaults::RxType rx_type = Defaults::RxTypeIllegal;

    static std::map<std::string, Defaults::RxType> rx_name_map;

    if (rx_name_map.empty()){
      rx_name_map["udp"] = Defaults::RxTypeUDP;
      rx_name_map["UDP"] = Defaults::RxTypeUDP;
      rx_name_map["zmq"]  = Defaults::RxTypeZMQ;
      rx_name_map["ZMQ"]  = Defaults::RxTypeZMQ;
    }

    if (rx_name_map.count(rx_name)){
      rx_type = rx_name_map[rx_name];
    }

    return rx_type;
  }

  static std::string map_rx_type_to_name(Defaults::RxType rx_type)
  {
    std::string rx_name;

    static std::map<Defaults::RxType, std::string> rx_type_map;

    if (rx_type_map.empty())
    {
      rx_type_map[Defaults::RxTypeUDP] = "udp";
      rx_type_map[Defaults::RxTypeZMQ] = "zmq";
      rx_type_map[Defaults::RxTypeIllegal] = "unknown";
    }

    if (rx_type_map.count(rx_type))
    {
      rx_name = rx_type_map[rx_type];
    }
    else
    {
      rx_name = rx_type_map[Defaults::RxTypeIllegal];
    }

    return rx_name;

  }

  std::string rx_port_list(void)
  {
    std::stringstream rx_ports_stream;
    std::copy(rx_ports_.begin(), rx_ports_.end(),
        std::ostream_iterator<uint16_t>(rx_ports_stream, ","));
    std::string rx_port_list = rx_ports_stream.str();
    rx_port_list.erase(rx_port_list.length()-1);

    return rx_port_list;
  }

  void as_ipc_message(OdinData::IpcMessage& config_msg)
  {

    config_msg.set_msg_type(OdinData::IpcMessage::MsgTypeCmd);
    config_msg.set_msg_val(OdinData::IpcMessage::MsgValCmdConfigure);

    config_msg.set_param<std::size_t>(CONFIG_MAX_BUFFER_MEM, max_buffer_mem_);
    config_msg.set_param<std::string>(CONFIG_DECODER_PATH, decoder_path_);
    config_msg.set_param<std::string>(CONFIG_DECODER_TYPE, decoder_type_);
    config_msg.set_param<std::string>(CONFIG_RX_TYPE, this->map_rx_type_to_name(rx_type_));

    config_msg.set_param<std::string>(CONFIG_RX_PORTS, rx_port_list());

    config_msg.set_param<std::string>(CONFIG_RX_ADDRESS, rx_address_);
    config_msg.set_param<int>(CONFIG_RX_RECV_BUFFER_SIZE, rx_recv_buffer_size_);
    config_msg.set_param<std::string>(CONFIG_RX_ENDPOINT, rx_channel_endpoint_);
    config_msg.set_param<std::string>(CONFIG_CTRL_ENDPOINT, ctrl_channel_endpoint_);
    config_msg.set_param<std::string>(CONFIG_FRAME_READY_ENDPOINT, frame_ready_endpoint_);
    config_msg.set_param<std::string>(CONFIG_FRAME_RELEASE_ENDPOINT, frame_release_endpoint_);
    config_msg.set_param<std::string>(CONFIG_SHARED_BUFFER_NAME, shared_buffer_name_);
    config_msg.set_param<int>(CONFIG_FRAME_COUNT, frame_count_);

    std::string decoder_config_path("decoder_config/");
    config_msg.set_param<int>(decoder_config_path + CONFIG_FRAME_TIMEOUT_MS, frame_timeout_ms_);
    config_msg.set_param<bool>(decoder_config_path + CONFIG_ENABLE_PACKET_LOGGING,
                               enable_packet_logging_);

    config_msg.set_param<bool>(CONFIG_FORCE_RECONFIG, force_reconfig_);
  }

private:

  std::size_t           max_buffer_mem_;         //!< Amount of shared buffer memory to allocate for frame buffers
  std::string           decoder_path_;           //!< Path to decoder library
  std::string           decoder_type_;           //!< Decoder type receiving data for - drives frame size
  boost::scoped_ptr<IpcMessage> decoder_config_; //!< Decoder configuration data as IpcMessage
  Defaults::RxType      rx_type_;                //!< Type of receiver interface (UDP or ZMQ)
  std::vector<uint16_t> rx_ports_;               //!< Port(s) to receive frame data on
  std::string           rx_address_;             //!< IP address to receive frame data on
  int                   rx_recv_buffer_size_;    //!< Receive socket buffer size
  unsigned int          io_threads_;             //!< Number of IO threads for IPC channels
  std::string           rx_channel_endpoint_;    //!< IPC channel endpoint for RX thread communication
  std::string           ctrl_channel_endpoint_;  //!< IPC channel endpoint for control communication with other processes
  std::string           frame_ready_endpoint_;   //!< IPC channel endpoint for transmitting frame ready notifications to other processes
  std::string           frame_release_endpoint_; //!< IPC channel endpoint for receiving frame release notifications from other processes
  std::string           shared_buffer_name_;     //!< Shared memory frame buffer name
  unsigned int          frame_timeout_ms_;       //!< Incomplete frame timeout in milliseconds
  unsigned int          frame_count_;            //!< Number of frames to receive before terminating
  bool                  enable_packet_logging_;  //!< Enable packet diagnostic logging
  bool                  force_reconfig_;         //!< Force a comlete reconfigure of the frame receiver

  friend class FrameReceiverApp;
  friend class FrameReceiverController;
  friend class FrameReceiverRxThread;
  friend class FrameReceiverUDPRxThread;
  friend class FrameReceiverZMQRxThread;
  friend class FrameReceiverConfigTestProxy;
  friend class FrameReceiverRxThreadTestProxy;
};

} // namespace FrameReceiver
#endif /* FRAMERECEIVERCONFIG_H_ */

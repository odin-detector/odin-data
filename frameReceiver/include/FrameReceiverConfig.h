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

#include "FrameReceiverDefaults.h"
#include "IpcMessage.h"

namespace FrameReceiver
{
class FrameReceiverConfig
{
public:

  FrameReceiverConfig() :
      max_buffer_mem_(Defaults::default_max_buffer_mem),
      sensor_type_(Defaults::default_sensor_type),
      rx_type_(Defaults::default_rx_type),
      rx_address_(Defaults::default_rx_address),
      rx_recv_buffer_size_(Defaults::default_rx_recv_buffer_size),
      rx_channel_endpoint_(Defaults::default_rx_chan_endpoint),
      ctrl_channel_endpoint_(Defaults::default_ctrl_chan_endpoint),
      frame_ready_endpoint_(Defaults::default_frame_ready_endpoint),
      frame_release_endpoint_(Defaults::default_frame_release_endpoint),
      shared_buffer_name_(Defaults::default_shared_buffer_name),
      frame_timeout_ms_(Defaults::default_frame_timeout_ms),
      enable_packet_logging_(Defaults::default_enable_packet_logging)
  {
    tokenize_port_list(rx_ports_, Defaults::default_rx_port_list);
  };

  void tokenize_port_list(std::vector<uint16_t>& port_list, const std::string port_list_str)
  {
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

  Defaults::RxType map_rx_name_to_type(std::string& rx_name)
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

  std::string map_rx_type_to_name(Defaults::RxType rx_type)
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

  void as_ipc_message(OdinData::IpcMessage& config_msg)
  {

    config_msg.set_param<std::size_t>("max_buffer_mem", max_buffer_mem_);
    config_msg.set_param<std::string>("sensor_path", sensor_path_);
    config_msg.set_param<std::string>("sensor_type", sensor_type_);
    config_msg.set_param<std::string>("rx_type", this->map_rx_type_to_name(rx_type_));

    std::stringstream rx_ports_stream;
    std::copy(rx_ports_.begin(), rx_ports_.end(), std::ostream_iterator<uint16_t>(rx_ports_stream, ","));
    std::string rx_ports_list = rx_ports_stream.str();
    rx_ports_list.erase(rx_ports_list.length()-1);
    config_msg.set_param<std::string>("rx_ports", rx_ports_list);

    config_msg.set_param<std::string>("rx_address", rx_address_);
    config_msg.set_param<int>("rx_recv_buffer_size", rx_recv_buffer_size_);
    config_msg.set_param<std::string>("rx_channel_endpoint", rx_channel_endpoint_);
    config_msg.set_param<std::string>("ctrl_channel_endpoint", ctrl_channel_endpoint_);
    config_msg.set_param<std::string>("frame_ready_endpoint", frame_ready_endpoint_);
    config_msg.set_param<std::string>("frame_release_endpoint", frame_release_endpoint_);
    config_msg.set_param<std::string>("shared_buffer_name", shared_buffer_name_);
    config_msg.set_param<unsigned int>("frame_timeout_ms", frame_timeout_ms_);
    config_msg.set_param<unsigned int>("frame_count", frame_count_);
    config_msg.set_param<bool>("enable_package_logging", enable_packet_logging_);

  }

private:

  std::size_t           max_buffer_mem_;         //!< Amount of shared buffer memory to allocate for frame buffers
  std::string           sensor_path_;            //!< Path to decoder library
  std::string           sensor_type_;            //!< Sensor type receiving data for - drives frame size
  Defaults::RxType      rx_type_;                //!< Type of receiver interface (UDP or ZMQ)
  std::vector<uint16_t> rx_ports_;               //!< Port(s) to receive frame data on
  std::string           rx_address_;             //!< IP address to receive frame data on
  int                   rx_recv_buffer_size_;    //!< Receive socket buffer size
  std::string           rx_channel_endpoint_;    //!< IPC channel endpoint for RX thread communication
  std::string           ctrl_channel_endpoint_;  //!< IPC channel endpoint for control communication with other processes
  std::string           frame_ready_endpoint_;   //!< IPC channel endpoint for transmitting frame ready notifications to other processes
  std::string           frame_release_endpoint_; //!< IPC channel endpoint for receiving frame release notifications from other processes
  std::string           shared_buffer_name_;     //!< Shared memory frame buffer name
  unsigned int          frame_timeout_ms_;       //!< Incomplete frame timeout in milliseconds
  unsigned int          frame_count_;            //!< Number of frames to receive before terminating
  bool                  enable_packet_logging_;  //!< Enable packet diagnostic logging

  friend class FrameReceiverApp;
  friend class FrameReceiverController; // TODO REMOVE THIS
  friend class FrameReceiverRxThread;
  friend class FrameReceiverUDPRxThread;
  friend class FrameReceiverZMQRxThread;
  friend class FrameReceiverConfigTestProxy;
  friend class FrameReceiverRxThreadTestProxy;
};

} // namespace FrameReceiver
#endif /* FRAMERECEIVERCONFIG_H_ */

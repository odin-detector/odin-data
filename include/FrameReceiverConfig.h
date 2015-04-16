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

namespace FrameReceiver
{
	class FrameReceiverConfig
	{
	public:

		FrameReceiverConfig() :
		    max_buffer_mem_(Defaults::default_max_buffer_mem),
		    sensor_type_(Defaults::SensorTypeIllegal),
		    rx_address_(Defaults::default_rx_address),
		    rx_recv_buffer_size_(Defaults::default_rx_recv_buffer_size),
		    rx_channel_endpoint_(Defaults::default_rx_chan_endpoint),
		    ctrl_channel_endpoint_(Defaults::default_ctrl_chan_endpoint),
		    frame_ready_endpoint_(Defaults::default_frame_ready_endpoint),
		    frame_release_endpoint_(Defaults::default_frame_release_endpoint),
		    shared_buffer_name_(Defaults::default_shared_buffer_name),
		    frame_timeout_ms_(Defaults::default_frame_timeout_ms)
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

		Defaults::SensorType map_sensor_name_to_type(std::string& sensor_name)
		{

		    Defaults::SensorType sensor_type = Defaults::SensorTypeIllegal;

		    static std::map<std::string, Defaults::SensorType> sensor_name_map;

		    if (sensor_name_map.empty())
		    {
		        sensor_name_map["percivalemulator"] = Defaults::SensorTypePercivalEmulator;
		        sensor_name_map["percival2m"]  = Defaults::SensorTypePercival2M;
		        sensor_name_map["percival13m"] = Defaults::SensorTypePercival13M;
		        sensor_name_map["excalibur3m"]   = Defaults::SensorTypeExcalibur3M;
		    }

		    if (sensor_name_map.count(sensor_name))
		    {
		        sensor_type = sensor_name_map[sensor_name];
		    }

		    return sensor_type;
		}

	private:

		std::size_t           max_buffer_mem_;         //!< Amount of shared buffer memory to allocate for frame buffers
		Defaults::SensorType  sensor_type_;            //!< Sensor type receiving data for - drives frame size
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

		friend class FrameReceiverApp;
		friend class FrameReceiverRxThread;
		friend class FrameReceiverConfigTestProxy;
		friend class FrameReceiverRxThreadTestProxy;
	};

} // namespace FrameReceiver


#endif /* FRAMERECEIVERCONFIG_H_ */

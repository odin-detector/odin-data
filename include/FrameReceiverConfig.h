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
#include "FrameReceiverDefaults.h"

namespace FrameReceiver
{
	class FrameReceiverConfig
	{
	public:

		FrameReceiverConfig() :
		    max_buffer_mem_(Defaults::default_max_buffer_mem),
		    sensor_type_(Defaults::SensorTypeIllegal),
		    rx_port_(Defaults::default_rx_port),
		    rx_address_(Defaults::default_rx_address)
		{
		};

		Defaults::SensorType map_sensor_name_to_type(std::string& sensor_name)
		{

		    Defaults::SensorType sensor_type = Defaults::SensorTypeIllegal;

		    static std::map<std::string, Defaults::SensorType> sensor_name_map;

		    if (sensor_name_map.empty())
		    {
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

		std::size_t          max_buffer_mem_; //!< Amount of shared buffer memory to allocate for frame buffers
		Defaults::SensorType sensor_type_;    //!< Sensor type receiving data for - drives frame size
		uint16_t             rx_port_;        //!< Port to receive frame data on
		std::string          rx_address_;     //!< IP address to receive frame data on

		friend class FrameReceiverApp;
		friend class FrameReceiverRxThread;
		friend class FrameReceiverConfigTestProxy;
	};

} // namespace FrameReceiver


#endif /* FRAMERECEIVERCONFIG_H_ */

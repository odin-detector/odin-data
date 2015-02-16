/*
 * FrameReceiverDefaults.h
 *
 *  Created on: Feb 4, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef FRAMERECEIVERDEFAULTS_H_
#define FRAMERECEIVERDEFAULTS_H_

#include <cstddef>
#include <stdint.h>
#include <string>

namespace FrameReceiver
{
	namespace Defaults
	{
		enum SensorType
		{
			SensorTypeIllegal = -1,
			SensorTypePercival2M,
			SensorTypePercival13M,
			SensorTypeExcalibur3M,
		};

		const int         default_node               = 1;
		const std::size_t default_max_buffer_mem     = 1048576;
		const SensorType  default_sensor_type        = SensorTypeIllegal;
		const uint16_t    default_rx_port            = 8989;
		const std::string default_rx_address         = "0.0.0.0";
		const std::string default_rx_chan_endpoint   = "inproc://rx_channel";
		const std::string default_ctrl_chan_endpoint = "tcp://*:5000"; //"ipc://frame_receiver_ctrl.ipc";

	}
}


#endif /* FRAMERECEIVERDEFAULTS_H_ */

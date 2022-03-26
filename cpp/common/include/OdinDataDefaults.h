/*
 * OdinDataDefaults.h
 *
 *  Created on: 07/07/2023
 *      Author: Gary Yendell
 */

#ifndef ODINDATADEFAULTS_H_
#define ODINDATADEFAULTS_H_

#include <cstddef>
#include <stdint.h>
#include <string>

namespace OdinData
{

namespace Defaults
{

const unsigned int default_io_threads             = 1;
const std::string  default_frame_ready_endpoint   = "tcp://127.0.0.1:5001";
const std::string  default_frame_release_endpoint = "tcp://127.0.0.1:5002";
const std::string  default_json_config_file       = "";
const std::string  default_shared_buffer_name     = "OdinDataBuffer";

} // namespace Defaults

} // namespace OdinData

#endif // ODINDATADEFAULTS_H_

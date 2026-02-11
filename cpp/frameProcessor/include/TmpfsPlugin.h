/*
 * BloscPlugin.h
 *
 *  Created on: 10 Feb 2026
 *      Author: Famous Alele
 */

#ifndef BLOSCPLUGIN_H_
#define BLOSCPLUGIN_H_
#include <log4cxx/logger.h>
using namespace log4cxx;

#include <boost/filesystem.hpp> // boost::filesystem::path, boost::filesystem::remove

#include "FrameProcessorPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{

class TmpfsPlugin : public FrameProcessorPlugin {
public:
  TmpfsPlugin();
  void process_frame(boost::shared_ptr<Frame> frame);
  void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
  void requestConfiguration(OdinData::IpcMessage& reply);
  int get_version_major() override;
  int get_version_minor() override;
  int get_version_patch() override;
  std::string get_version_short() override;
  std::string get_version_long() override;

  const static std::string CONFIG_FILE_PATH;
  const static std::string CONFIG_REMOVE_FILE;


private:
  /** Mutex used to make this class thread safe */
  boost::recursive_mutex mutex_;
  /** Pointer to logger */
  LoggerPtr logger_;
  /** The full file path of the underlying file */
  boost::filesystem::path full_file_path_;
};

};

#endif

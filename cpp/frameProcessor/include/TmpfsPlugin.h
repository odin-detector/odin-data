/*
 * TmpfsPlugin.h
 *
 *  Created on: 10 Feb 2026
 *      Author: Famous Alele
 */

#ifndef TMPFSPLUGIN_H_
#define TMPFSPLUGIN_H_
#include <log4cxx/logger.h>
using namespace log4cxx;

#include <boost/filesystem.hpp> // boost::filesystem::path

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

private:
  /** Mutex used to make this class thread safe */
  boost::recursive_mutex mutex_;
  /** Flag to stop the frame from writing if file root path creation fails! */
  bool is_writing;
  /** Pointer to logger */
  LoggerPtr logger_;
  /** Root path to write files to - files will be created in this directory or a nested directory, depending on Frame properties */
  boost::filesystem::path file_path_;
};

};

#endif

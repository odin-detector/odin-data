/*
 * BloscPlugin.h
 *
 *  Created on: 22 Jan 2018
 *      Author: Ulrik Pedersen
 */

#ifndef TOOLS_FILEWRITER_BLOSCPLUGIN_H_
#define TOOLS_FILEWRITER_BLOSCPLUGIN_H_

#include <log4cxx/logger.h>
using namespace log4cxx;

#include "FrameProcessorPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{

/**
 * This is a compression plugin using the Blosc library
 *
 * When this plugin receives a frame, processFrame is called and the class
 * uses the blosc compression methods to compress the data and output a new,
 * compressed Frame.
 */
class BloscPlugin : public FrameProcessorPlugin
{
public:
  BloscPlugin();
  virtual ~BloscPlugin();
  boost::shared_ptr<Frame> compress_frame(boost::shared_ptr<Frame> frame);

private:
  void process_frame(boost::shared_ptr<Frame> frame);

  /** Pointer to logger */
  LoggerPtr logger_;
  /** Current acquisition ID */
  std::string current_acquisition_;
  /** Compression settings */
  BloscCompressionSettings compression_settings_;
  /** Compression settings for the next acquisition */
  BloscCompressionSettings commanded_compression_settings_;

  int get_version_major();
  int get_version_minor();
  int get_version_patch();
  std::string get_version_short();
  std::string get_version_long();
};

/**
 * Registration of this plugin through the ClassLoader. This macro
 * registers the class without needing to worry about name mangling
 */
REGISTER(FrameProcessorPlugin, BloscPlugin, "BloscPlugin");

} /* namespace FrameProcessor */

#endif /* TOOLS_FILEWRITER_BLOSCPLUGIN_H_ */

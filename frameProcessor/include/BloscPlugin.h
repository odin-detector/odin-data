/*
 * BloscPlugin.h
 *
 *  Created on: 22 Jan 2018
 *      Author: Ulrik Pedersen
 */

#ifndef BLOSCPLUGIN_H_
#define BLOSCPLUGIN_H_

#include <log4cxx/logger.h>
using namespace log4cxx;

#include "FrameProcessorPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{

typedef struct{
  int compression_level;
  unsigned int shuffle;
  size_t type_size;
  size_t uncompressed_size;
  unsigned int threads;
  unsigned int blosc_compressor; // TODO: change from using blosc compressor definition as unsigned int to string
} BloscCompressionSettings;
void create_cd_values(const BloscCompressionSettings& settings, std::vector<unsigned int> cd_values);

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
  // Baseclass API to implement:
  void process_frame(boost::shared_ptr<Frame> frame);
  void status(OdinData::IpcMessage& status);
  void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
  void requestConfiguration(OdinData::IpcMessage& reply);
  int get_version_major();
  int get_version_minor();
  int get_version_patch();
  std::string get_version_short();
  std::string get_version_long();

  // Methods unique to this class
  void update_compression_settings();

  // private data
  /** Pointer to logger */
  LoggerPtr logger_;
  /** Mutex used to make this class thread safe */
  boost::recursive_mutex mutex_;
  /** Current acquisition ID */
  std::string current_acquisition_;
  /** Compression settings */
  BloscCompressionSettings compression_settings_;
  /** Compression settings for the next acquisition */
  BloscCompressionSettings commanded_compression_settings_;

private:
  /** Configuration constants */
  static const std::string CONFIG_BLOSC_COMPRESSOR;
  static const std::string CONFIG_BLOSC_THREADS;
  static const std::string CONFIG_BLOSC_LEVEL;
  static const std::string CONFIG_BLOSC_SHUFFLE;

};

/**
 * Registration of this plugin through the ClassLoader. This macro
 * registers the class without needing to worry about name mangling
 */
REGISTER(FrameProcessorPlugin, BloscPlugin, "BloscPlugin");

} /* namespace FrameProcessor */

#endif /* BLOSCPLUGIN_H_ */

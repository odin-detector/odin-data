/*
 * BloscPlugin.h
 *
 *  Created on: 22 Jan 2018
 *      Author: Ulrik Pedersen
 */

#ifndef BLOSCPLUGIN_H_
#define BLOSCPLUGIN_H_
#include <cstdint>

#include <blosc.h>
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
  unsigned int blosc_compressor;
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
  void * get_buffer(size_t nbytes);

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
  /** Temporary buffer for compressed data */
  void * data_buffer_ptr_;
  size_t data_buffer_size_;

private:
  /** Configuration constants */
  static const std::string CONFIG_BLOSC_COMPRESSOR;
  static const std::string CONFIG_BLOSC_THREADS;
  static const std::string CONFIG_BLOSC_LEVEL;
  static const std::string CONFIG_BLOSC_SHUFFLE;

/** These are private virtual (inline) methods
 *  MUST be customized by every derived FrameProcessor 
 *  plugin derived class that can requires metadata.
 * \returns a map of ParamMetadata's
 */
  using ParameterBucket_t = std::unordered_map<std::string, ParamMetadata>;
  virtual ParameterBucket_t& get_config_metadata() const noexcept{
    static ParameterBucket_t config_metadata_bucket{{CONFIG_BLOSC_COMPRESSOR, {"int", "ro", {}, BLOSC_BLOSCLZ, BLOSC_ZSTD} },
                                                    {CONFIG_BLOSC_THREADS, {"int", "ro", {}, 1, INT32_MAX} },
                                                    {CONFIG_BLOSC_LEVEL, {"int", "ro", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, 1, 9} },
                                                    {CONFIG_BLOSC_SHUFFLE, {"int", "ro", {BLOSC_NOSHUFFLE, BLOSC_SHUFFLE, BLOSC_BITSHUFFLE}, BLOSC_NOSHUFFLE, BLOSC_BITSHUFFLE} },
                                                                };
    std::cout << "In BLOSC PLUGIN NOW!!!!\n";
    return config_metadata_bucket;
  }
  virtual ParameterBucket_t& get_status_metadata() const noexcept{
    static BloscPlugin::ParameterBucket_t status_metadata_bucket{};
    return status_metadata_bucket;
  }

};

} /* namespace FrameProcessor */

#endif /* BLOSCPLUGIN_H_ */

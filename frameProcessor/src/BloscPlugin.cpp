/*
 * BloscPlugin.cpp
 *
 *  Created on: 22 Jan 2018
 *      Author: Ulrik Pedersen
 */
#include <cstdlib>
#include <blosc.h>
#include <version.h>
#include <BloscPlugin.h>

namespace FrameProcessor
{

/**
 * cd_values[7] meaning (see blosc.h):
 *   0: reserved
 *   1: reserved
 *   2: type size
 *   3: uncompressed size
 *   4: compression level
 *   5: 0: shuffle not active, 1: byte shuffle, 2: bit shuffle
 *   6: the actual Blosc compressor to use. See blosc.h
 *
 * @param settings
 * @return
 */
void create_cd_values(const BloscCompressionSettings& settings, std::vector<unsigned int>& cd_values)
{
  if (cd_values.size() < 7) cd_values.resize(7);
  cd_values[0] = 0;
  cd_values[1] = 0;
  cd_values[2] = static_cast<unsigned int>(settings.type_size);
  cd_values[3] = static_cast<unsigned int>(settings.uncompressed_size);
  cd_values[4] = settings.compression_level;
  cd_values[5] = settings.shuffle;
  cd_values[6] = settings.blosc_compressor;
}

/**
* The constructor sets up logging used within the class.
*/
BloscPlugin::BloscPlugin() :
current_acquisition_("")
{
  this->commanded_compression_settings_.blosc_compressor = BLOSC_LZ4;
  this->commanded_compression_settings_.shuffle = BLOSC_BITSHUFFLE;
  this->commanded_compression_settings_.compression_level = 1;
  this->commanded_compression_settings_.type_size = 2;
  this->commanded_compression_settings_.uncompressed_size = 0;
  this->compression_settings_ = this->commanded_compression_settings_;

  // Setup logging for the class
  logger_ = Logger::getLogger("FP.BloscPlugin");
  logger_->setLevel(Level::getAll());
  LOG4CXX_TRACE(logger_, "BloscPlugin constructor.");

  //blosc_init(); // not required for blosc >= v1.9
  int ret = 0;
  ret = blosc_set_compressor(BLOSC_LZ4_COMPNAME);
  if (ret < 0) LOG4CXX_ERROR(logger_, "Blosc unable to set compressor: " << BLOSC_LZ4_COMPNAME);
  LOG4CXX_TRACE(logger_, "Blosc Version: " << blosc_get_version_string());
  LOG4CXX_TRACE(logger_, "Blosc list available compressors: " << blosc_list_compressors());
  LOG4CXX_TRACE(logger_, "Blosc current compressor: " << blosc_get_compressor());
  LOG4CXX_TRACE(logger_, "Blosc # threads: " << blosc_get_nthreads());
}

/**
 * Destructor.
 */
BloscPlugin::~BloscPlugin()
{
  LOG4CXX_TRACE(logger_, "BloscPlugin destructor.");
}

/**
 * Compress one frame, return compressed frame.
 */
boost::shared_ptr<Frame> BloscPlugin::compress_frame(boost::shared_ptr<Frame> src_frame)
{
  int compressed_size = 0;
  BloscCompressionSettings c_settings;
  boost::shared_ptr <Frame> dest_frame;

  const void* src_data_ptr = static_cast<const void*>(
      static_cast<const char*>(src_frame->get_data())
  );

  c_settings = this->update_compression_settings(src_frame->get_acquisition_id());
  if (src_frame->get_data_type() >= 0) {
    c_settings.type_size = src_frame->get_data_type_size();
  }
  else { // TODO: This if/else is a hack to work around Frame::data_type_ not being set. See https://jira.diamond.ac.uk/browse/BC-811
    c_settings.type_size = 2; // hack: just default to 16bit per pixel as Excalibur use that
  }
  c_settings.uncompressed_size = src_frame->get_data_size();

  size_t dest_data_size = c_settings.uncompressed_size + BLOSC_MAX_OVERHEAD;
  // TODO: is this malloc really necessary? Can't we get writable DataBlocks somehow?
  void *dest_data_ptr = malloc(dest_data_size);
  if (dest_data_ptr == NULL) {throw std::runtime_error("Failed to malloc buffer for Blosc compression output");}

  try {
    LOG4CXX_TRACE(logger_, "Compressing frame no. " << src_frame->get_frame_number()
      << " with: " << blosc_get_compressor());
    LOG4CXX_TRACE(logger_, "calling blosc_compress: clevel=" << c_settings.compression_level
                            << " doshuffle=" << c_settings.shuffle
                            << " typesize=" << c_settings.type_size
                            << " nbytes=" << c_settings.uncompressed_size
                            << " src=" << src_data_ptr
                            << " dest=" << dest_data_ptr
                            << " destsize=" << dest_data_size);
    compressed_size = blosc_compress(c_settings.compression_level, c_settings.shuffle,
                                     c_settings.type_size,
                                     c_settings.uncompressed_size, src_data_ptr,
                                     dest_data_ptr, dest_data_size);
    if (compressed_size > 0) {
      double factor = 0.;
      factor = (double)src_frame->get_data_size() / (double)compressed_size;
      LOG4CXX_TRACE(logger_, "Compression factor of: " << factor);
    }

    dest_frame = boost::shared_ptr<Frame>(new Frame(src_frame->get_dataset_name()));

    LOG4CXX_TRACE(logger_, "Copying compressed data to output frame. (" << compressed_size << " bytes)");
    // I wish we had a pointer swap feature on the Frame class and avoid this unnecessary copy...
    dest_frame->copy_data(dest_data_ptr, compressed_size);
    if (dest_data_ptr != NULL) {free(dest_data_ptr); dest_data_ptr = NULL;}


    // I wish we had a shallow-copy feature on the Frame class...
    dest_frame->set_data_type(src_frame->get_data_type());
    dest_frame->set_frame_number(src_frame->get_frame_number());
    dest_frame->set_acquisition_id(src_frame->get_acquisition_id());
    // TODO: is this the correct way to get and set dimensions?
    dest_frame->set_dimensions("data", src_frame->get_dimensions("data"));
  }
  catch (const std::exception& e) {
    LOG4CXX_ERROR(logger_, "Serious error in Blosc compression: " << e.what());
    if (dest_data_ptr != NULL) {free(dest_data_ptr); dest_data_ptr = NULL;}
  }
  return dest_frame;
}

/**
 * Update the compression settings used if the acquisition ID differs from the current one.
 * i.e. if a new acquisition has been started.
 *
 * @param acquisition_id
 * @return a const ref to the compression settings
 */
const BloscCompressionSettings& BloscPlugin::update_compression_settings(const std::string &acquisition_id)
{
  if (acquisition_id != this->current_acquisition_){
    LOG4CXX_TRACE(logger_, "New acquisition detected: "<< acquisition_id);
    this->compression_settings_ = this->commanded_compression_settings_;
    this->current_acquisition_ = acquisition_id;
    int ret = 0;
    const char ** p_compressor_name;
    blosc_compcode_to_compname(this->compression_settings_.blosc_compressor, p_compressor_name);
    ret = blosc_set_compressor(*p_compressor_name);
    if (ret < 0) {
      LOG4CXX_ERROR(logger_, "Blosc failed to set compressor: "
          << " " << this->compression_settings_.blosc_compressor
          << " " << *p_compressor_name)
      throw std::runtime_error("Blosc failed to set compressor");
    }
  }
  return this->compression_settings_;
}

  /**
 * Perform compression on the frame and output a new, compressed Frame.
 *
 * \param[in] frame - Pointer to a Frame object.
 */
void BloscPlugin::process_frame(boost::shared_ptr<Frame> src_frame)
{
  LOG4CXX_TRACE(logger_, "Received a new frame...");
  boost::shared_ptr <Frame> compressed_frame = this->compress_frame(src_frame);
  LOG4CXX_TRACE(logger_, "Pushing compressed frame");
  this->push(compressed_frame);
}

int BloscPlugin::get_version_major()
{
  return ODIN_DATA_VERSION_MAJOR;
}

int BloscPlugin::get_version_minor()
{
  return ODIN_DATA_VERSION_MINOR;
}

int BloscPlugin::get_version_patch()
{
  return ODIN_DATA_VERSION_PATCH;
}

std::string BloscPlugin::get_version_short()
{
  return ODIN_DATA_VERSION_STR_SHORT;
}

std::string BloscPlugin::get_version_long()
{
  return ODIN_DATA_VERSION_STR;
}

} /* namespace FrameProcessor */

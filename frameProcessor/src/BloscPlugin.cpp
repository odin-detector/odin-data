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
 * The constructor sets up logging used within the class.
 */
BloscPlugin::BloscPlugin()
{
  // Setup logging for the class
  logger_ = Logger::getLogger("FW.BloscPlugin");
  logger_->setLevel(Level::getAll());
  LOG4CXX_TRACE(logger_, "BloscPlugin constructor.");

  //blosc_init(); // not required for blosc >= v1.9
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
  boost::shared_ptr <Frame> dest_frame;

  const void* src_data_ptr = static_cast<const void*>(
      static_cast<const char*>(src_frame->get_data())
  );
  const size_t raw_data_size = src_frame->get_data_size();
  LOG4CXX_TRACE(logger_, "Frame data size: " << raw_data_size);

  size_t dest_data_size = src_frame->get_data_size() + BLOSC_MAX_OVERHEAD;
  // TODO: is this malloc really necessary? Can't we get writable DataBlocks somehow?
  void *dest_data_ptr = malloc(dest_data_size);
  if (dest_data_ptr == NULL) {throw std::runtime_error("Failed to malloc buffer for Blosc compression output");}

  try {
    LOG4CXX_TRACE(logger_, "Compressing frame no. " << src_frame->get_frame_number());
    // TODO: all args are hard-coded here and need to be turned into parameters
    compressed_size = blosc_compress(1, 1, src_frame->get_data_type_size(),
                                     raw_data_size, src_data_ptr, dest_data_ptr, dest_data_size);
    if (compressed_size > 0) {
      double factor = 0.;
      factor = (double)raw_data_size / (double)compressed_size;
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

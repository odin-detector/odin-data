/*
 * BloscPlugin.cpp
 *
 *  Created on: 22 Jan 2018
 *      Author: Ulrik Pedersen
 */

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
 * Perform compression on the frame and output a new, compressed Frame.
 *
 * \param[in] frame - Pointer to a Frame object.
 */
void BloscPlugin::process_frame(boost::shared_ptr<Frame> src_frame)
{
  int compressed_size = 0;

  LOG4CXX_TRACE(logger_, "Received a new frame...");

  const void* src_data_ptr = static_cast<const void*>(
      static_cast<const char*>(src_frame->get_data())
  );
  const size_t raw_data_size = src_frame->get_data_size();
  LOG4CXX_TRACE(logger_, "Frame data size: " << raw_data_size);

  try {
    // TODO: is this malloc really necessary? Can't we get writable DataBlocks somehow?
    void *dest_data_ptr = malloc(src_frame->get_data_size());
    // TODO: error check on the malloc here...

    LOG4CXX_TRACE(logger_, "Compressing frame no. " << src_frame->get_frame_number());
    // TODO: all args are hard-coded here and need to be turned into parameters
    compressed_size = blosc_compress(1, 1, src_frame->get_data_type_size(),
                                     raw_data_size, src_data_ptr, dest_data_ptr, raw_data_size);
    if (compressed_size > 0) {
      double factor = 0.;
      factor = (double)raw_data_size / (double)compressed_size;
      LOG4CXX_TRACE(logger_, "Compression factor of: " << factor);
    }

    boost::shared_ptr <Frame> dest_frame;
    dest_frame = boost::shared_ptr<Frame>(new Frame(src_frame->get_dataset_name()));

    LOG4CXX_TRACE(logger_, "Copying compressed data to output frame. (" << compressed_size << " bytes)");
    // I wish we had a pointer swap feature on the Frame class and avoid this unnecessary copy...
    dest_frame->copy_data(dest_data_ptr, compressed_size);

    // I wish we had a shallow-copy feature on the Frame class...
    dest_frame->set_data_type(src_frame->get_data_type());
    dest_frame->set_frame_number(src_frame->get_frame_number());
    dest_frame->set_acquisition_id(src_frame->get_acquisition_id());
    // TODO: is this the correct way to get and set dimensions?
    dest_frame->set_dimensions("data", src_frame->get_dimensions("data"));

    LOG4CXX_TRACE(logger_, "Pushing compressed frame");
    this->push(dest_frame);
    free(dest_data_ptr);
    dest_data_ptr = NULL;
  }
  catch (const std::exception& e){
    LOG4CXX_ERROR(logger_, "Serious error in Blosc compression: " << e.what());
  }
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

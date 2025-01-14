//
// Created by hir12111 on 30/10/18.
//

#include "SumPlugin.h"
#include "version.h"

namespace FrameProcessor {

/**
 * The constructor sets up logging used within the class.
 */
  SumPlugin::SumPlugin()
  {
    // Setup logging for the class
    logger_ = Logger::getLogger("FP.SumPlugin");
    LOG4CXX_TRACE(logger_, "SumPlugin constructor.");
  }

/**
 * Destructor.
 */
  SumPlugin::~SumPlugin()
  {
    LOG4CXX_TRACE(logger_, "SumPlugin destructor.");
  }

  template<class PixelType>
  static uint64_t calculate_sum(std::shared_ptr <Frame> frame)
  {
    uint64_t sum_value = 0;
    const PixelType *data = static_cast<const PixelType *>(frame->get_image_ptr());
    size_t elements_count = frame->get_image_size() / sizeof(data[0]);

    for (size_t pixel_index = 0; pixel_index < elements_count; pixel_index++) {
      sum_value += data[pixel_index];
    }

    return sum_value;
  }

/**
 * Calculate the sum of each pixel based on the data type
 *
 * \param[in] frame - Pointer to a Frame object.
 */
  void SumPlugin::process_frame(std::shared_ptr <Frame> frame)
  {
    LOG4CXX_TRACE(logger_, "Received a new frame...");
    switch (frame->get_meta_data().get_data_type()) {
      case raw_8bit:
        frame->meta_data().set_parameter<uint64_t>(SUM_PARAM_NAME, calculate_sum<uint8_t>(frame));
        break;
      case raw_16bit:
        frame->meta_data().set_parameter<uint64_t>(SUM_PARAM_NAME, calculate_sum<uint16_t>(frame));
        break;
      case raw_32bit:
        frame->meta_data().set_parameter<uint64_t>(SUM_PARAM_NAME, calculate_sum<uint32_t>(frame));
        break;
      case raw_64bit:
        frame->meta_data().set_parameter<uint64_t>(SUM_PARAM_NAME, calculate_sum<uint64_t>(frame));
        break;
      default:
        LOG4CXX_ERROR(logger_, "SumPlugin doesn't support data type:"
            << get_type_from_enum(static_cast<DataType>(frame->get_meta_data().get_data_type())));
    }
    this->push(frame);
  }

  int SumPlugin::get_version_major()
  {
    return ODIN_DATA_VERSION_MAJOR;
  }

  int SumPlugin::get_version_minor()
  {
    return ODIN_DATA_VERSION_MINOR;
  }

  int SumPlugin::get_version_patch()
  {
    return ODIN_DATA_VERSION_PATCH;
  }

  std::string SumPlugin::get_version_short()
  {
    return ODIN_DATA_VERSION_STR_SHORT;
  }

  std::string SumPlugin::get_version_long()
  {
    return ODIN_DATA_VERSION_STR;
  }

} /* namespace FrameProcessor */

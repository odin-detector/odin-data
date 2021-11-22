/*
 *  Created on: 30 Oct 2018
 *      Authors: Emilio Perez Juarez,
 *               Gary Yendell
 */

#include "DebugLevelLogger.h"
#include "SumPlugin.h"
#include "version.h"

namespace FrameProcessor {

  /**
   * Get bin name corresponding to threshold
   *
   * \param[in] index - Threshold of bin to get name of
   */
  std::string& Histogram::get_name_by_index(uint index) {
    return this->threshold_name_map_[this->threshold_vector_[index]];
  }

  /**
   * Get read-only view of map of name -> threshold
   */
  const std::map<std::string, uint64_t>& Histogram::get_name_threshold_map() {
    return this->name_threshold_map_;
  }

  /**
   * Add a histogram bin
   *
   * \param[in] name - Name of bin to add
   * \param[in] threshold - Threshold of bin to add
   * \throw `std::runtime_error` if the name and/or threshold already exists
   */
  void Histogram::add_bin(std::string& name, uint64_t& threshold) {
    // Check for clashes
    std::map<std::string, uint64_t>::iterator bin;
    for (bin = this->name_threshold_map_.begin(); bin != this->name_threshold_map_.end(); ++bin) {
      if (threshold == bin->second) {
        throw std::runtime_error("A histogram bin with given threshold already exists");
      }
    }

    // Add to underlying containers
    this->name_threshold_map_[name] = threshold;
    this->threshold_name_map_[threshold] = name;
    // - Recreate vector from sorted keys
    this->threshold_vector_.clear();
    std::map<uint64_t, std::string>::iterator ibin;
    for (ibin = this->threshold_name_map_.begin(); ibin != this->threshold_name_map_.end(); ++ibin) {
      this->threshold_vector_.push_back(ibin->first);
    }
  }

  /**
   * Increment the highest bin that the given pixel counts fit into, if any
   *
   * If `this` has no configured bins or if the counts are lower than all of them nothing is done
   *
   * \param[in] sum - `Sum` to increment `histogram` bin of
   * \param[in] counts - Counts of pixel to find bin for
   */
  void Histogram::bin_pixel(Sum& sum, uint64_t counts) {
    if (this->threshold_vector_.size() > 0 && counts > this->threshold_vector_[0]) {
      this->_bin_pixel(sum, counts, 0);
    }
  }

  /**
   * Increment the highest bin that the given pixel counts fit into
   *
   * The caller must confirm that pixel at least fits into the lowest bin or this method could increment it
   * incorrectly.
   *
   * Must be called with `previous_index` = `0` except for recursive calls of itself where it will be incremented
   * until the highest bin is reached.
   *
   * This method calls recursively until it reaches the highest bin, or a bin that the pixel counts do not fit into
   *
   * \param[in] sum - `Sum` to increment `Histogram` of
   * \param[in] counts - Counts of pixel to find bin for
   * \param[in] previous_index - Previous index in array of thresholds
   */
  void Histogram::_bin_pixel(Sum& sum, uint64_t counts, uint previous_index) {
    int index = previous_index + 1;
    if (counts > this->threshold_vector_[index]) {
      // Pixel fits in this bin or a higher one
      if (index + 1 < this->threshold_vector_.size()) {
        // Try next bin and increment this bin if it does not fit
        this->_bin_pixel(sum, counts, index);
      } else {
        // No higher bins to check - increment this one
        ++sum.histogram[this->get_name_by_index(index)];
      }
    } else {
      // Pixel does not fit in this bin - increment previous one
      ++sum.histogram[this->get_name_by_index(previous_index)];
    }
  }

  const std::string SumPlugin::SUM_PARAM_NAME = "sum";
  const std::string SumPlugin::CONFIG_HISTOGRAM = "histogram";

  /**
   * The constructor sets up logging used within the class.
   */
  SumPlugin::SumPlugin() :
    histogram_()
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
  Sum SumPlugin::calculate_sum(boost::shared_ptr<Frame> frame)
  {
    const PixelType *data = static_cast<const PixelType *>(frame->get_image_ptr());
    size_t elements_count = frame->get_image_size() / sizeof(data[0]);

    Sum sum(this->histogram_.get_name_threshold_map());
    for (size_t pixel_index = 0; pixel_index < elements_count; pixel_index++) {
      sum.total_counts += data[pixel_index];
      this->histogram_.bin_pixel(sum, data[pixel_index]);
    }

    return sum;
  }

  /**
   * Calculate the sum of each pixel based on the data type
   *
   * \param[in] frame - Pointer to a Frame object.
   */
  void SumPlugin::process_frame(boost::shared_ptr<Frame> frame)
  {
    LOG4CXX_TRACE(logger_, "Received a new frame...");
    switch (frame->get_meta_data().get_data_type()) {
      case raw_8bit:
        {
          Sum sum = this->calculate_sum<uint8_t>(frame);
          this->add_data_to_frame(frame, sum);
        }
        break;
      case raw_16bit:
        {
          Sum sum = this->calculate_sum<uint16_t>(frame);
          this->add_data_to_frame(frame, sum);
        }
        break;
      case raw_32bit:
        {
          Sum sum = this->calculate_sum<uint32_t>(frame);
          this->add_data_to_frame(frame, sum);
        }
        break;
      case raw_64bit:
        {
          Sum sum = this->calculate_sum<uint64_t>(frame);
          this->add_data_to_frame(frame, sum);
        }
        break;
      default:
        LOG4CXX_ERROR(logger_,
          "SumPlugin doesn't support data type:"
            << get_type_from_enum(static_cast<DataType>(frame->get_meta_data().get_data_type()))
        );
    }
    this->push(frame);
  }

  /**
   * Add calculated parameters to Frame
   *
   * \param[in] frame - Pointer to a Frame object.
   * \param[in] sum - Sum object containing parameter values.
   */
  void SumPlugin::add_data_to_frame(boost::shared_ptr<Frame> frame, Sum& sum)
  {
    frame->meta_data().set_parameter<uint64_t>(SUM_PARAM_NAME, sum.total_counts);

    std::map<std::string, uint64_t>::const_iterator bin;
    for(bin = sum.histogram.begin(); bin != sum.histogram.end(); ++bin) {
      frame->meta_data().set_parameter<uint64_t>(bin->first, bin->second);
    }
  }

  /**
   * Set configuration options for this Plugin.
   *
   * \param[in] config - IpcMessage containing configuration data.
   * \param[out] reply - Response IpcMessage.
   */
  void SumPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
  {
    try {
      if (config.has_param(CONFIG_HISTOGRAM)) {
        OdinData::IpcMessage histogram(config.get_param<const rapidjson::Value &>(CONFIG_HISTOGRAM));
        std::vector<std::string> histogram_bins = histogram.get_param_names();

        // Update histogram bins
        std::vector<std::string>::iterator bin_name;
        for (bin_name = histogram_bins.begin(); bin_name != histogram_bins.end(); ++bin_name) {
          uint64_t bin_threshold = histogram.get_param<uint64_t>(*bin_name);
          this->histogram_.add_bin(*bin_name, bin_threshold);
          LOG4CXX_INFO(logger_, "Threshold " << *bin_name << " set to " << bin_threshold);
        }
      }
    }
    catch (std::runtime_error& e)
    {
      std::stringstream ss;
      ss << "Bad ctrl msg: " << e.what();
      this->set_error(ss.str());
      throw;
    }
  }

/**
 * Get the configuration values for this plugin
 *
 * \param[out] reply - Response IpcMessage.
 */
void SumPlugin::requestConfiguration(OdinData::IpcMessage& reply)
{
  std::map<std::string, uint64_t> histogram_bins = this->histogram_.get_name_threshold_map();
  std::map<std::string, uint64_t>::const_iterator bin;
  for(bin = histogram_bins.begin(); bin != histogram_bins.end(); ++bin) {
    reply.set_param(get_name() + "/" + CONFIG_HISTOGRAM + "/" + bin->first, bin->second);
  }
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

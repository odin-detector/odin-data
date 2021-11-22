/*
 *  Created on: 30 Oct 2018
 *      Authors: Emilio Perez Juarez,
 *               Gary Yendell
 */

#ifndef SUMPLUGIN_H
#define SUMPLUGIN_H

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameProcessorPlugin.h"

/**
 * This plugin class calculates the sum of each pixel and
 * adds it as a parameter
 *
 */
namespace FrameProcessor {

  struct Sum;

  /**
   * A container representing a histogram consisting of a pair of bidirectional maps and a vector
   *
   * The keys/values of the maps are `std::string` (the name of a histogram bin) and `uint64_t` (the number of counts)
   * a pixel must have to be placed in that bin.
   *
   * This struct enables:
   *   - Lookup of uint64_t by std::string key
   *   - Lookup of std::string by uint64_t key
   *   - Random access of uint64_t values by index (in an ordered vector of uint64_t)
   *
   * It can store the histogram threshold definitions and increment the correct bin of a corresponding `Sum` struct
   * according to those definitions and a given number of counts in a pixel.
   *
   * \param[in] index - Index of threshold to lookup
   */
  class Histogram {
    public:
      /**
       * Constructor
       */
      Histogram() :
        name_threshold_map_(),
        threshold_name_map_(),
        threshold_vector_()
      {}

      std::string& get_name_by_index(uint index);
      const std::map<std::string, uint64_t>& get_name_threshold_map();
      void add_bin(std::string& name, uint64_t& threshold);
      void bin_pixel(Sum& sum, uint64_t counts);

    private:
      std::map<std::string, uint64_t> name_threshold_map_;
      std::map<uint64_t, std::string> threshold_name_map_;
      std::vector<uint64_t> threshold_vector_;

      void _bin_pixel(Sum& sum, uint64_t counts, uint previous_index);
  };

  /**
   * A container representing the total counts and a histogram of arbitrary bins of the data elements in a `Frame`
   */
  struct Sum {
    uint64_t total_counts;
    std::map<std::string, uint64_t> histogram;

  /**
   * Constructor
   *
   * Initialise `this->histogram` recreating the bins from given `histogram`, but setting the values to `0`
   *
   * \param[in] histogram - map of histogram name to histogram threshold
   */
    Sum(std::map<std::string, uint64_t> histogram) {
      total_counts = 0;
      std::map<std::string, uint64_t>::const_iterator bin;
      for(bin = histogram.begin(); bin != histogram.end(); ++bin) {
        this->histogram[bin->first] = 0;
      }
    }
  };

  class SumPlugin : public FrameProcessorPlugin {
  public:
    SumPlugin();

    ~SumPlugin();

    void process_frame(boost::shared_ptr<Frame> frame);
    template<class PixelType>
    Sum calculate_sum(boost::shared_ptr<Frame> frame);
    void add_data_to_frame(boost::shared_ptr<Frame> frame, Sum& sum);
    void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);

    int get_version_major();

    int get_version_minor();

    int get_version_patch();

    std::string get_version_short();

    std::string get_version_long();

    // Parameter names on Frame
    static const std::string SUM_PARAM_NAME;
    // Config Strings
    static const std::string CONFIG_HISTOGRAM;

  private:
    /** Pointer to logger */
    LoggerPtr logger_;
    /** Container storing name and threshold of histogram bins */
    Histogram histogram_;

    void requestConfiguration(OdinData::IpcMessage& reply);

  };

} /* namespace FrameProcessor */

#endif //SUMPLUGIN_H

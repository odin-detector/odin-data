#ifndef FRAMEPROCESSOR_IFRAMEMETADATA_H
#define FRAMEPROCESSOR_IFRAMEMETADATA_H

#include <string>
#include <map>
#include <vector>
#include <stdint.h>

#include <boost/any.hpp>
#include <log4cxx/logger.h>

#include "FrameProcessorDefinitions.h"

typedef unsigned long long dimsize_t;
typedef std::vector<dimsize_t> dimensions_t;

namespace FrameProcessor {

class IFrameMetaData {

public:

  IFrameMetaData(const std::string& dataset_name,
                 const DataType& data_type,
                 const std::string& acquisition_ID,
                 const std::vector<unsigned long long>& dimensions,
                 const CompressionType& compression_type = no_compression);

  IFrameMetaData();

  IFrameMetaData(const IFrameMetaData& frame);

  /** Return frame parameters */
  const std::map <std::string, boost::any> &get_parameters() const;

  /** Get frame parameter
   *
   * @tparam T
   * @param parameter_name
   * @return
   */
  template<class T>
  T get_parameter(const std::string &parameter_name) const {
    std::map<std::string, boost::any>::const_iterator iter = parameters_.find(parameter_name);
    if (iter == parameters_.end())
    {
      LOG4CXX_ERROR(logger, "Unable to find parameter: " + parameter_name);
      throw std::runtime_error("Unable to find parameter");
    }
    try
    {
      return boost::any_cast<T>(iter->second);
    }
    catch (boost::bad_any_cast &e)
    {
      LOG4CXX_ERROR(logger, "Parameter has wrong type: " + parameter_name);
      throw std::runtime_error("Parameter has wrong type");
    }
    catch (std::exception &e) {
      throw std::runtime_error("Unknown exception fetching parameter");
    }
  }

  /** Set frame parameter
   *
   * @tparam T
   * @param parameter_name
   * @param value
   */
  template<class T>
  void set_parameter(const std::string &parameter_name, T value) {
    parameters_[parameter_name] = value;
  }

  /** Check if frame has parameter
   *
   * @param index
   * @return
   */
  bool has_parameter(const std::string& index) const {
    return (parameters_.count(index) == 1);
  }

  /** Return dataset_name */
  const std::string &get_dataset_name() const;

  /** Set dataset name */
  void set_dataset_name(const std::string &dataset_name);

  /** Return data type */
  DataType get_data_type() const;

  /** Set data type */
  void set_data_type(DataType data_type);

  /** Return acquisition ID */
  const std::string &get_acquisition_ID() const;

  /** Set acquisition ID */
  void set_acquisition_ID(const std::string &acquisition_ID);

  /** Return dimensions */
  const dimensions_t &get_dimensions() const;

  /** Set dimensions */
  void set_dimensions(const dimensions_t &dimensions);

  /** Return compression type */
  CompressionType get_compression_type() const;

  /** Return frame offset */
  int64_t get_frame_offset() const;

  /** Set frame offset */
  void set_frame_offset(const int64_t &offset);

  /** Adjust frame offset by increment */
  void adjust_frame_offset(const int64_t &increment);

private:

  /** Pointer to logger */
  log4cxx::LoggerPtr logger;

  /** Name of this dataset */
  std::string dataset_name_;

  /** Data type of raw data */
  DataType data_type_;

  /** Acquisition ID of the acquisition of this frame **/
  std::string acquisition_ID_;

  /** Vector of dimensions */
  dimensions_t dimensions_;

  /** Compression type of raw data */
  CompressionType compression_type_;

  /** Map of parameters */
  std::map <std::string, boost::any> parameters_;

  /** Frame offset */
  int64_t frame_offset_;

};

}

#endif

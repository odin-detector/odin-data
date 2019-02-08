#ifndef FRAMEPROCESSOR_IFRAMEMETADATA_H
#define FRAMEPROCESSOR_IFRAMEMETADATA_H

#include <string>
#include <map>
#include <vector>
#include <stdint.h>

#include <boost/any.hpp>

#include "FrameProcessorDefinitions.h"

typedef unsigned long long dimsize_t;
typedef std::vector<dimsize_t> dimensions_t;

namespace FrameProcessor {

  struct IFrameMetaData {

      IFrameMetaData(const std::string& dataset_name,
                     const long long& frame_number,
                     const DataType& data_type,
                     const std::string& acquisition_ID_,
                     const std::vector<unsigned long long>& dimensions,
                     const CompressionType& compression_type = no_compression);

      IFrameMetaData();

      IFrameMetaData(const IFrameMetaData& frame);

      /** Get frame parameter
       *
       * @tparam T
       * @param parameter_name
       * @return
       */
      template<class T>
      T get_parameter(const std::string &parameter_name) {
        return boost::any_cast<T>(parameters_[parameter_name]);
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
      bool has_parameter(const std::string& index) {
        return (parameters_.count(index) == 1);
      }

      /** Name of this dataset */
      std::string dataset_name_;

      /** Frame number */
      long long frame_number_;

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

  };

}

#endif

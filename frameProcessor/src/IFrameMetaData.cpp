#include "IFrameMetaData.h"

namespace FrameProcessor {

    IFrameMetaData::IFrameMetaData(const std::string &dataset_name,
                                   const long long &frame_number,
                                   const DataType &data_type,
                                   const std::string &acquisition_ID,
                                   const std::vector<unsigned long long> &dimensions,
                                   const CompressionType &compression_type) :
            dataset_name_(dataset_name),
            frame_number_(frame_number),
            data_type_(data_type),
            acquisition_ID_(acquisition_ID),
            dimensions_(dimensions),
            compression_type_(compression_type) {}

    IFrameMetaData::IFrameMetaData() : dataset_name_(""),
                                       frame_number_(0),
                                       data_type_(raw_8bit),
                                       acquisition_ID_(""),
                                       compression_type_(no_compression) {}

    IFrameMetaData::IFrameMetaData(const IFrameMetaData &frame) {
        dataset_name_ = frame.dataset_name_;
        frame_number_ = frame.frame_number_;
        data_type_ = frame.data_type_;
        acquisition_ID_ = frame.acquisition_ID_;
        dimensions_ = frame.dimensions_;
        compression_type_ = frame.compression_type_;
        parameters_ = frame.parameters_;

    }
}

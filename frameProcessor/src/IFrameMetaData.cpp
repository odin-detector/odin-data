#include "IFrameMetaData.h"

namespace FrameProcessor {

    IFrameMetaData::IFrameMetaData(const long long& frame_number,
                                   const std::string &dataset_name,
                                   const DataType &data_type,
                                   const std::string &acquisition_ID,
                                   const std::vector<unsigned long long> &dimensions,
                                   const CompressionType &compression_type) :
            frame_number_(frame_number),
            dataset_name_(dataset_name),
            data_type_(data_type),
            acquisition_ID_(acquisition_ID),
            dimensions_(dimensions),
            compression_type_(compression_type),
            frame_offset_(0),
            logger(log4cxx::Logger::getLogger("FP.IFrameMetaData")) {
      logger->setLevel(log4cxx::Level::getAll());
    }

    IFrameMetaData::IFrameMetaData() :
            frame_number_(-1),
            dataset_name_(""),
            data_type_(raw_8bit),
            compression_type_(no_compression),
            frame_offset_(0),
            logger(log4cxx::Logger::getLogger("FP.IFrameMetaData")) {
      logger->setLevel(log4cxx::Level::getAll());
    }

    IFrameMetaData::IFrameMetaData(const IFrameMetaData &frame) {
      frame_number_ = frame.frame_number_;
      dataset_name_ = frame.dataset_name_;
      data_type_ = frame.data_type_;
      acquisition_ID_ = frame.acquisition_ID_;
      dimensions_ = frame.dimensions_;
      compression_type_ = frame.compression_type_;
      parameters_ = frame.parameters_;
      frame_offset_ = frame.frame_offset_;
      logger = frame.logger;
    }

    /** Get frame parameters
     * @return std::map <std::string, boost::any>  map
     */
    const std::map <std::string, boost::any> &IFrameMetaData::get_parameters() const {
      return this->parameters_;
    };

    /** Return frame number
     * @return long long - frame number
     */
    long long IFrameMetaData::get_frame_number() const {
      return this->frame_number_;
    }

    /** Set frame number
     * @param long long - frame number
     */
    void IFrameMetaData::set_frame_number(const long long& frame_number) {
      this->frame_number_ = frame_number;
    }

    /** Return dataset name
     * @return std::string - dataset name
     */
    const std::string &IFrameMetaData::get_dataset_name() const {
      return this->dataset_name_;
    }

    /** Set dataset name
     * @param std::string dataset_name - name of dataset
     */
    void IFrameMetaData::set_dataset_name(const std::string &dataset_name) {
      this->dataset_name_ = dataset_name;
    }

    /** Return data type
     * @return DataType - data type
     */
    DataType IFrameMetaData::get_data_type() const {
      return this->data_type_;
    }

    /** Set data type
     * @param DataType - data type
     */
    void IFrameMetaData::set_data_type(DataType data_type) {
      this->data_type_ = data_type;
    }

    /** Return acquistion ID
     * @return std::string acquisition ID
     */
    const std::string &IFrameMetaData::get_acquisition_ID() const {
      return this->acquisition_ID_;
    }

    /** Set acquisition ID
     * @param std::string acquisition id
     */
    void IFrameMetaData::set_acquisition_ID(const std::string &acquisition_ID) {
      this->acquisition_ID_ = acquisition_ID;
    }

    /** Return dimensions
     * @return dimensions
     */
    const dimensions_t &IFrameMetaData::get_dimensions() const {
      return this->dimensions_;
    }

    /** Set dimensions
     * @param dimensions_t dimensions
     */
    void IFrameMetaData::set_dimensions(const dimensions_t &dimensions) {
      this->dimensions_ = dimensions;
    }

    /** Return compression type
     * @return compression type
     */
    CompressionType IFrameMetaData::get_compression_type() const {
      return this->compression_type_;
    }

    /** Set compression type
     * @ param CompressionType compression type
     */
    void IFrameMetaData::set_compression_type(CompressionType compression_type) {
      this->compression_type_ = compression_type;
    }

    /** Return frame offset
     * @return int64_t frame offset
     */
    int64_t IFrameMetaData::get_frame_offset() const {
      return this->frame_offset_;
    }

    /** Set frame offset
     * @param int64_t new offset
     */
    void IFrameMetaData::set_frame_offset(const int64_t &offset) {
      this->frame_offset_ = offset;
    }

    /** Adjust frame offset by increment
      * @param increment to adjust offset by
      */
    void IFrameMetaData::adjust_frame_offset(const int64_t &increment) {
      this->frame_offset_ += increment;
    }

}

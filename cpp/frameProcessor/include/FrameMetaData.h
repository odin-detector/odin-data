#ifndef FRAMEPROCESSOR_FRAMEMETADATA_H
#define FRAMEPROCESSOR_FRAMEMETADATA_H

#include <map>
#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/variant.hpp>

#include <log4cxx/logger.h>

#include "FrameProcessorDefinitions.h"

typedef unsigned long long dimsize_t;
typedef std::vector<dimsize_t> dimensions_t;

namespace FrameProcessor {

class FrameMetaData {
    // template <typename T>
    // using is_integral_val = std::is_same<T, uint8_t>::value || std::is_same<t, uint16_t>::value
    //     || std::is_same<T, uint32_t>::value || std::is_same<T, uint64_t>::value;

public:
    using pType_t = boost::variant<boost::blank, uint8_t, uint16_t, uint32_t, uint64_t, float>;
    FrameMetaData(
        long long frame_number,
        const std::string& dataset_name,
        DataType data_type,
        const std::string& acquisition_ID,
        const std::vector<unsigned long long>& dimensions,
        CompressionType compression_type = no_compression
    ) :
        frame_number_(frame_number),
        dataset_name_(dataset_name),
        data_type_(data_type),
        acquisition_ID_(acquisition_ID),
        dimensions_(dimensions),
        compression_type_(compression_type),
        frame_offset_(0),
        logger(log4cxx::Logger::getLogger("FP.FrameMetaData"))
    {
    }

    FrameMetaData(FrameMetaData&& rhs) = default; // move constructor

    FrameMetaData& operator=(FrameMetaData&& rhs) = default; // move constructor

    FrameMetaData& operator=(const FrameMetaData& rhs) = default; // move-assignment operator

    FrameMetaData() :
        frame_number_(-1),
        dataset_name_(""),
        data_type_(raw_unknown),
        compression_type_(unknown_compression),
        frame_offset_(0),
        logger(log4cxx::Logger::getLogger("FP.FrameMetaData"))
    {
    }

    FrameMetaData(const FrameMetaData& frame) = default;

    /** Return frame parameters */
    const std::unordered_map<std::string, pType_t>& get_parameters() const
    {
        return this->parameters_;
    }

    /** Get frame parameter
     *
     * @tparam T
     * @param parameter_name
     * @return
     */
    template <
        class T,
        typename = typename std::enable_if<
            std::is_same<T, uint8_t>::value || std::is_same<T, uint16_t>::value || std::is_same<T, uint32_t>::value
            || std::is_same<T, uint64_t>::value || std::is_floating_point<T>::value>::type>
    pType_t get_parameter(const std::string& parameter_name) const
    {
        if (parameters_.count(parameter_name)) {
            return parameters_.at(parameter_name);
        }
        LOG4CXX_ERROR(logger, "Unable to find parameter: " + parameter_name);
        return boost::blank {};
    }

    template <
        class T,
        typename = typename std::enable_if<
            std::is_same<T, uint8_t>::value || std::is_same<T, uint16_t>::value || std::is_same<T, uint32_t>::value
            || std::is_same<T, uint64_t>::value || std::is_floating_point<T>::value>::type>
    pType_t get_parameter(std::string&& parameter_name) const
    {
        if (parameters_.count(parameter_name)) {
            return parameters_.at(parameter_name);
        }
        LOG4CXX_ERROR(logger, "Unable to find parameter: " + parameter_name);
        return {};
    }

    /** Set frame parameter
     *
     * @tparam T
     * @param parameter_name
     * @param value
     */

    template <
        class T,
        typename = typename std::enable_if<
            std::is_same<T, uint8_t>::value || std::is_same<T, uint16_t>::value || std::is_same<T, uint32_t>::value
            || std::is_same<T, uint64_t>::value || std::is_floating_point<T>::value>::type>
    void set_parameter(const std::string& parameter_name, T value)
    {
        parameters_[parameter_name] = value;
    }

    template <
        class T,
        typename = typename std::enable_if<
            std::is_same<T, uint8_t>::value || std::is_same<T, uint16_t>::value || std::is_same<T, uint32_t>::value
            || std::is_same<T, uint64_t>::value || std::is_floating_point<T>::value>::type>
    void set_parameter(std::string&& parameter_name, T value)
    {
        parameters_[std::move(parameter_name)] = value;
    }

    /** Check if frame has parameter
     *
     * @param index
     * @return
     */
    bool has_parameter(const std::string& index) const
    {
        return parameters_.count(index);
    }

    /** Return frame number */
    long long get_frame_number() const
    {
        return this->frame_number_;
    }

    /** Set frame number */
    void set_frame_number(const long long& frame_number)
    {
        this->frame_number_ = frame_number;
    }

    /** Return dataset_name */
    const std::string& get_dataset_name() const
    {
        return this->dataset_name_;
    }

    /** Set dataset name */
    void set_dataset_name(const std::string& dataset_name)
    {
        this->dataset_name_ = dataset_name;
    }

    void set_dataset_name(std::string&& dataset_name)
    {
        this->dataset_name_ = std::move(dataset_name);
    }

    /** Return data type */
    DataType get_data_type() const
    {
        return this->data_type_;
    }

    /** Set data type */
    void set_data_type(DataType data_type)
    {
        this->data_type_ = data_type;
    }

    /** Return acquisition ID */
    const std::string& get_acquisition_ID() const
    {
        return this->acquisition_ID_;
    }

    /** Set acquisition ID */
    void set_acquisition_ID(const std::string& acquisition_ID)
    {
        this->acquisition_ID_ = acquisition_ID;
    }

    /** Return dimensions */
    const dimensions_t& get_dimensions() const
    {
        return this->dimensions_;
    }

    /** Set dimensions */
    void set_dimensions(const dimensions_t& dimensions)
    {
        this->dimensions_ = dimensions;
    }

    void set_dimensions(dimensions_t&& dimensions)
    {
        this->dimensions_ = std::move(dimensions);
    }

    /** Return compression type */
    CompressionType get_compression_type() const
    {
        return this->compression_type_;
    }

    /** Set compression type */
    void set_compression_type(CompressionType compression_type)
    {
        this->compression_type_ = compression_type;
    }

    /** Return frame offset */
    int64_t get_frame_offset() const
    {
        return this->frame_offset_;
    }

    /** Set frame offset */
    void set_frame_offset(const int64_t& offset)
    {
        this->frame_offset_ = offset;
    }

    /** Adjust frame offset by increment */
    void adjust_frame_offset(const int64_t& increment)
    {
        this->frame_offset_ += increment;
    }

private:
    /** Frame number */
    long long frame_number_;

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
    std::unordered_map<std::string, pType_t> parameters_;

    /** Frame offset */
    int64_t frame_offset_;
};

}

#endif

#ifndef FRAMEPROCESSOR_FRAMEMETADATA_H
#define FRAMEPROCESSOR_FRAMEMETADATA_H

#include <map>
#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/variant.hpp>

#include "FrameProcessorDefinitions.h"

typedef unsigned long long dimsize_t;
typedef std::vector<dimsize_t> dimensions_t;

namespace FrameProcessor {

class FrameMetaData {
    using pType_t = boost::variant<boost::blank, uint8_t, uint16_t, uint32_t, uint64_t, float>;

public:
    FrameMetaData(
        long long frame_number,
        const std::string& dataset_name,
        DataType data_type,
        const std::string& acquisition_ID,
        const std::vector<unsigned long long>& dimensions,
        CompressionType compression_type = no_compression
    ) :
        dataset_name_(dataset_name),
        acquisition_ID_(acquisition_ID),
        dimensions_(dimensions),
        frame_number_(frame_number),
        frame_offset_(0),
        data_type_(data_type),
        compression_type_(compression_type)
    {
    }

    FrameMetaData(
        long long frame_number,
        std::string&& dataset_name,
        DataType data_type,
        std::string&& acquisition_ID,
        dimensions_t&& dimensions,
        CompressionType compression_type = no_compression
    ) noexcept :
        dataset_name_(std::move(dataset_name)),
        acquisition_ID_(std::move(acquisition_ID)),
        dimensions_(std::move(dimensions)),
        frame_number_(frame_number),
        frame_offset_(0),
        data_type_(data_type),
        compression_type_(compression_type)
    {
    }

    FrameMetaData(const FrameMetaData& frame) = default; // copy constructor

    FrameMetaData(FrameMetaData&& rhs) = default; // move constructor

    FrameMetaData& operator=(const FrameMetaData& rhs) = default; // copy-assignment operator

    FrameMetaData& operator=(FrameMetaData&& rhs) = default; // move-assignment operator

    FrameMetaData() :
        dataset_name_(""),
        frame_number_(-1),
        frame_offset_(0),
        data_type_(raw_unknown),
        compression_type_(unknown_compression)
    {
    }

    /** Return frame parameters */
    const std::unordered_map<std::string, pType_t>& get_parameters() const noexcept
    {
        return this->parameters_;
    }

    /** Get frame parameter
     * @attention This function potentially THROWS exceptions
     *            if parameter_name is NOT found in the container.
     *            OR if the specified type T does NOT match what
     *            is stored.
     *            To avoid potential exceptions from invalid parameters
     *            check for validity of parameter_name using:
     *
     *               bool has_parameter(const std::string& index) const;
     *
     *            before calling this function or its overload(s).
     *            AND ensure the template type specialized - T,
     *            is what is stored in the FrameMetadata with:
     *
     *               bool is_type(const std::string& index) const;
     *
     * @tparam T
     * @param parameter_name
     * @return - the value of the parameter in the dictionary
     */
    template <
        typename T,
        typename = typename std::enable_if<std::is_unsigned<T>::value || std::is_same<T, float>::value>::type>
    T get_parameter(const std::string& parameter_name) const
    {
        return boost::get<T>(parameters_.at(parameter_name));
    }

    /** Set frame parameter
     *
     * @tparam T
     * @param parameter_name
     * @param value
     */
    template <
        typename T,
        typename = typename std::enable_if<std::is_unsigned<T>::value || std::is_same<T, float>::value>::type>
    void set_parameter(const std::string& parameter_name, T value) noexcept
    {
        parameters_[parameter_name] = value;
    }

    template <
        typename T,
        typename = typename std::enable_if<std::is_unsigned<T>::value || std::is_same<T, float>::value>::type>
    void set_parameter(std::string&& parameter_name, T value) noexcept
    {
        parameters_[std::move(parameter_name)] = value;
    }

    /**
     * Check if the assumed type of the parameter THROWS
     *
     * @attention This method assumes 'index' is present in the object
     *            It may THROW an std::out_of_range exception if index
     *            has not been validated to be present in the object!
     *            validate the parameter - 'index' is present with:
     *
     *                      bool has_parameter(const std::string& index) const
     *
     * @param index - the parameter whose's stored type is
     *                to be validated!
     * @tparam T - the assumed type of index.
     * @return bool
     *            true - if the type T matches stored type
     *            false - if the type T doesn't match stored type
     *
     */
    template <
        typename T,
        typename = typename std::enable_if<std::is_unsigned<T>::value || std::is_same<T, float>::value>::type>
    bool is_type(const std::string& index) const noexcept
    {
        static_assert(std::is_default_constructible<T>::value);
        if (parameters_.count(index)) {
            int id = parameters_.at(index).which();
            return id == (pType_t { T {} }).which();
        }
        return false;
    }

    /** Check if frame has parameter
     *
     * @param index - index/param name to lookup
     * @return
     */
    bool has_parameter(const std::string& index) const
    {
        return parameters_.count(index);
    }

    /** Return frame number
     * @return - frame number
     */
    long long get_frame_number() const noexcept
    {
        return this->frame_number_;
    }

    /** Set frame number
     * @param frame_numer - the frame number
     */
    void set_frame_number(const long long frame_number) noexcept
    {
        this->frame_number_ = frame_number;
    }

    /** Return dataset_name
     * @return - return const ref to dataset name string
     */
    const std::string& get_dataset_name() const noexcept
    {
        return this->dataset_name_;
    }

    /** Return dataset_name
     * @return - return copy of dataset name string
     */
    std::string get_dataset_name_c() const
    {
        return this->dataset_name_;
    }

    /** Set acquisition ID
     * @tparam T - std::string convertible type
     * @param dataset_name - Dataset string to set
     */
    template <typename T, typename = typename std::enable_if<std::is_convertible<T, std::string>::value>::type>
    void set_dataset_name(T&& dataset_name) noexcept(
        std::is_same<typename std::decay<T>::type, std::string>::value && std::is_rvalue_reference<T>::value
    )
    {
        this->dataset_name_ = std::move(dataset_name);
    }

    /** Return data type
     * @return - Data type enumeration
     */
    DataType get_data_type() const noexcept
    {
        return this->data_type_;
    }

    /** Set data type
     * @param data_type - data type to set
     */
    void set_data_type(DataType data_type) noexcept
    {
        this->data_type_ = data_type;
    }

    /** Return acquisition ID
     * @return - const ref to acquisition ID string
     */
    const std::string& get_acquisition_ID() const noexcept
    {
        return this->acquisition_ID_;
    }

    /** Return acquisition ID
     * @return - Copy of acquisition ID string
     */
    std::string get_acquisition_ID_c() const
    {
        return this->acquisition_ID_;
    }

    /** Set acquisition ID
     * @param acquisition_ID - acquisition ID to set
     */
    template <typename T, typename = typename std::enable_if<std::is_convertible<T, std::string>::value>::type>
    void set_acquisition_ID(T&& acquisition_ID) noexcept(
        std::is_same<typename std::decay<T>::type, std::string>::value && std::is_rvalue_reference<T>::value
    )
    {
        this->acquisition_ID_ = std::move(acquisition_ID);
    }

    /** Return const ref to dimensions */
    const dimensions_t& get_dimensions() const noexcept
    {
        return this->dimensions_;
    }

    /** Return copy of dimensions */
    dimensions_t get_dimensions_c() const
    {
        return this->dimensions_;
    }

    /** Set dimensions with const ref arg
     * @tparam T - std::vector<long long> after decay
     * @param dimensions - dimensions to set
     */
    template <
        typename T,
        typename = typename std::enable_if<std::is_same<typename std::decay<T>::type, dimensions_t>::value>::type>
    void set_dimensions(T&& dimensions) noexcept(std::is_rvalue_reference<T>::value)
    {
        this->dimensions_ = std::move(dimensions);
    }

    /** Return compression type */
    CompressionType get_compression_type() const noexcept
    {
        return this->compression_type_;
    }

    /** Set compression type */
    void set_compression_type(CompressionType compression_type) noexcept
    {
        this->compression_type_ = compression_type;
    }

    /** Return frame offset */
    int64_t get_frame_offset() const noexcept
    {
        return this->frame_offset_;
    }

    /** Set frame offset */
    void set_frame_offset(const int64_t offset) noexcept
    {
        this->frame_offset_ = offset;
    }

    /** Adjust frame offset by increment */
    void adjust_frame_offset(const int64_t increment) noexcept
    {
        this->frame_offset_ += increment;
    }

private:
    /** Name of this dataset */
    std::string dataset_name_;

    /** Acquisition ID of the acquisition of this frame **/
    std::string acquisition_ID_;

    /** Map of parameters */
    std::unordered_map<std::string, pType_t> parameters_;

    /** Vector of dimensions */
    dimensions_t dimensions_;

    /** Frame number */
    long long frame_number_;

    /** Frame offset */
    int64_t frame_offset_;

    /** Data type of raw data */
    DataType data_type_;

    /** Compression type of raw data */
    CompressionType compression_type_;
};

}

#endif

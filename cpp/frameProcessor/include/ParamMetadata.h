/*
 * ParamMetadata.h
 *
 *  Created on: 30 Oct 2025.
 *      Author: Famous Alele, sgr21863
 */

#include <string>
#include <boost/variant.hpp>

namespace FrameProcessor
{
/** This struct is a representation of the metadata
 */
struct ParamMetadata
{
using allowed_values_t = boost::variant<boost::blank, std::string, int>;

ParamMetadata(std::string& type, std::string& access_mode, std::vector<allowed_values_t>& allowed_values, int32_t min, int32_t max) : 
                type_{std::move(type)}, 
                access_mode_{std::move(access_mode)}, 
                allowed_values_{std::move(allowed_values)}, 
                min_{min}, max_{max}{} // moving constructor parameters in it's member -initializer list
ParamMetadata(ParamMetadata&& rhs) = default; // Move constructor
ParamMetadata& operator=(ParamMetadata&& rhs) = default; // Move operator
// delete copy assignment and constructor methods we expect it to always be move constructed/assigned.
ParamMetadata(const ParamMetadata& rhs) = delete;
ParamMetadata& operator=(const ParamMetadata& rhs) = delete;
static constexpr int MAX_UNSET = std::numeric_limits<int>::min();
static constexpr int MIN_UNSET = std::numeric_limits<int>::max();
friend class FrameProcessorPlugin;

private:
    std::string access_mode_;
    std::string type_;
    std::vector<allowed_values_t> allowed_values_;
    int32_t min_;
    int32_t max_;
};
}

/*
 * ParamMetadata.h
 *
 *  Created on: 30 Oct 2025.
 *      Author: Famous Alele, sgr21863
 */

#include <string>
#include <type_traits>
#include <variant>

namespace FrameProcessor {
/** This struct is a representation of the metadata
 */
struct ParamMetadata {
    using allowed_values_t = std::variant<std::monostate, std::string, int>;

    enum class AccessMode {
        READ_ONLY,
        READ_WRITE
    };
    enum class Datatype {
        STRING_T,
        STRINGARR_T,
        BOOL_T,
        BOOLARR_T,
        FLOAT_T,
        FLOATARR_T,
        FLOAT2DARR_T,
        FLOAT3DARR_T,
        INT_T,
        INTARR_T,
        INT2DARR_T,
        INT3DARR_T,
        UINT_T,
        UINTARR_T,
        UINT2DARR_T,
        UINT3DARR_T,
        // -----------------
        // internal use ONLY
        CUSTOM_T,
    };

    std::string access_mode_as_string() const
    {
        std::string ret;
        switch (this->access_mode_) {
        case AccessMode::READ_ONLY:
            ret = READ_ONLY;
            return READ_ONLY;
        case AccessMode::READ_WRITE:
            ret = READ_WRITE;
            return READ_WRITE;
        }
        return ret;
    }

    std::string datatype_as_string() const
    {
        switch (this->type_) {
        case Datatype::STRING_T:
            return STRING_T;
        case Datatype::STRINGARR_T:
            return STRINGARR_T;
        case Datatype::BOOL_T:
            return BOOL_T;
        case Datatype::BOOLARR_T:
            return BOOLARR_T;
        case Datatype::FLOAT_T:
            return FLOAT_T;
        case Datatype::FLOATARR_T:
            return FLOATARR_T;
        case Datatype::FLOAT2DARR_T:
            return FLOAT2DARR_T;
        case Datatype::FLOAT3DARR_T:
            return FLOAT3DARR_T;
        case Datatype::INT_T:
            return INT_T;
        case Datatype::INTARR_T:
            return INTARR_T;
        case Datatype::INT2DARR_T:
            return INT2DARR_T;
        case Datatype::INT3DARR_T:
            return INT3DARR_T;
        case Datatype::UINT_T:
            return UINT_T;
        case Datatype::UINTARR_T:
            return UINTARR_T;
        case Datatype::UINT2DARR_T:
            return UINT2DARR_T;
        case Datatype::UINT3DARR_T:
            return UINT3DARR_T;
        case Datatype::CUSTOM_T:
            return custom_str_;
        }
        return custom_str_;
    }

    ParamMetadata() = default;
    ParamMetadata(
        Datatype type,
        AccessMode access_mode,
        std::vector<allowed_values_t>& allowed_values,
        int32_t min,
        int32_t max
    ) :
        type_ { type },
        access_mode_ { access_mode },
        allowed_values_ { std::move(allowed_values) },
        min_ { min },
        max_ { max }
    {
    }
    template <typename TYPE>
    using is_str_conv = std::is_convertible<TYPE, std::string>; // can convert TYPE To std::string
    // We want to allow ONLY RVALUES and LVALUES of std::string or types that convert to std::string
    template <typename T, typename = typename std::enable_if<is_str_conv<T>::value>::type>
    ParamMetadata(
        T&& type,
        AccessMode access_mode,
        std::vector<allowed_values_t>& allowed_values,
        int32_t min,
        int32_t max
    ) :
        custom_str_ { std::move(type) },
        type_ { Datatype::CUSTOM_T },
        access_mode_ { access_mode },
        allowed_values_ { std::move(allowed_values) },
        min_ { min },
        max_ { max }
    {
    }
    ParamMetadata(ParamMetadata&& rhs) = default; // Move constructor
    ParamMetadata& operator=(ParamMetadata&& rhs) = default; // Move operator
    // delete copy assignment and constructor methods we expect it to always be move constructed/assigned.
    ParamMetadata(const ParamMetadata& rhs) = delete;
    ParamMetadata& operator=(const ParamMetadata& rhs) = delete;
    static constexpr int MAX_UNSET = std::numeric_limits<int>::min();
    static constexpr int MIN_UNSET = std::numeric_limits<int>::max();
    friend class FrameProcessorPlugin;

private:
    AccessMode access_mode_;
    Datatype type_;
    std::vector<allowed_values_t> allowed_values_;
    int32_t min_;
    int32_t max_;
    std::string custom_str_;

    constexpr static char READ_ONLY[2] = "r";
    constexpr static char READ_WRITE[3] = "rw";

    constexpr static char STRING_T[7] = "string";
    constexpr static char STRINGARR_T[9] = "string[]";
    constexpr static char BOOL_T[5] = "bool";
    constexpr static char BOOLARR_T[7] = "bool[]";
    constexpr static char FLOAT_T[6] = "float";
    constexpr static char FLOATARR_T[8] = "float[]";
    constexpr static char FLOAT2DARR_T[10] = "float[][]";
    constexpr static char FLOAT3DARR_T[12] = "float[][][]";
    constexpr static char INT_T[4] = "int";
    constexpr static char INTARR_T[6] = "int[]";
    constexpr static char INT2DARR_T[8] = "int[][]";
    constexpr static char INT3DARR_T[10] = "int[][][]";
    constexpr static char UINT_T[5] = "uint";
    constexpr static char UINTARR_T[7] = "uint[]";
    constexpr static char UINT2DARR_T[9] = "uint[][]";
    constexpr static char UINT3DARR_T[11] = "uint[][][]";
};
}

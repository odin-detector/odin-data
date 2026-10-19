/*
 * FrameProcessorDefinitions.h
 *
 *  Created on: 31 Oct 2017
 *      Author: vtu42223
 */

#ifndef FRAMEPROCESSOR_FrameProcessorDefinitions_H_
#define FRAMEPROCESSOR_FrameProcessorDefinitions_H_

#include "stdint.h"
#include <sstream>
#include <stdexcept>

namespace FrameProcessor {
/**
 * Enumeration to store the pixel type of the incoming image
 */
enum DataType {
    raw_unknown,
    raw_8bit,
    raw_16bit,
    raw_32bit,
    raw_64bit,
    raw_float,

    // Add no types byond this sentinel type and
    // NEVER use this type as a valid data type
    FINAL_DATATYPE
};
/**
 * Enumeration to store the compression type of the incoming image
 */
enum CompressionType {
    unknown_compression,
    no_compression,
    lz4,
    bslz4,
    blosc,

    // Add no types byond this sentinel type and
    // NEVER use this type as a valid data type
    FINAL_COMPRESSIONTYPE
};
/**
 * Enumeration to store the compression type of the incoming image
 */
enum ProcessFrameStatus {
    status_ok,
    status_complete,
    status_complete_missing_frames,
    status_invalid
};

/* Enum style arrays for the header*/
const std::string DATA_TYPES[] = { "unknown", "uint8", "uint16", "uint32", "uint64", "float" };
const std::string COMPRESS_TYPES[] = { "unknown", "none", "LZ4", "BSLZ4", "blosc" };

/**
 * Gets the DataType from string
 * @param str data type as string
 * @return DataType data type
 */
inline static DataType get_type_from_string(const std::string& str)
{
    if (str == "unknown")
        return raw_unknown;
    else if (str == "uint8")
        return raw_8bit;
    else if (str == "uint16")
        return raw_16bit;
    else if (str == "uint32")
        return raw_32bit;
    else if (str == "uint64")
        return raw_64bit;
    else if (str == "float")
        return raw_float;
    return raw_unknown;
}

/**
 * Gets the size of the data type
 * \param[in] type - enum value
 * \return size_t data type size
 */
inline static size_t get_size_from_enum(DataType type)
{
    type = ((type < 0) | (type >= FINAL_DATATYPE)) ? raw_unknown : type;
    uint16_t data_sizes[FINAL_COMPRESSIONTYPE];
    data_sizes[raw_unknown] = 256; // this disables shuffle for blosc
    data_sizes[raw_8bit] = sizeof(uint8_t);
    data_sizes[raw_16bit] = sizeof(uint16_t);
    data_sizes[raw_32bit] = sizeof(uint32_t);
    data_sizes[raw_64bit] = sizeof(uint64_t);
    data_sizes[raw_float] = sizeof(float);
    return data_sizes[type];
}

/**
 * Gets the type of the data, based on the enum value
 * Values as follows:
 * 0 - raw_8bit
 * 1 - raw_16bit
 * 2 - raw_32bit
 * \param[in] type - enum value
 * \return string value representing data type, or "unknown" if an unrecognised enum value
 */
inline static std::string get_type_from_enum(DataType type) noexcept
{
    type = ((type < 0) | (type >= FINAL_DATATYPE)) ? raw_unknown : type;
    return DATA_TYPES[type];
}

/**
 * Gets the CompressionType from string
 * @param str compression type as string
 * @return CompressionType compression type
 */
inline static CompressionType get_compression_from_string(const std::string& str)
{
    if (str == "unknown")
        return unknown_compression;
    else if (str == "none")
        return no_compression;
    else if (str == "LZ4")
        return lz4;
    else if (str == "BSLZ4")
        return bslz4;
    else if (str == "blosc")
        return blosc;
    return unknown_compression;
}

/**
 * Gets the type of compression, based on the enum value
 * 0: None, 1: LZ4, 2: BSLZ4
 * \param[in] compress - enum value
 * \return the string value representing the compression. Assumed none if the enum value is unrecognised.
 */
inline static std::string get_compress_from_enum(CompressionType compress)
{
    compress = ((compress < 0) | (compress >= FINAL_COMPRESSIONTYPE)) ? unknown_compression : compress;
    return COMPRESS_TYPES[compress];
}
/**
 * Defines a dataset to be saved in HDF5 format.
 */
struct DatasetDefinition {
    /** Name of the dataset **/
    std::string name;
    /** Data type for the dataset **/
    DataType data_type;
    /** Numer of frames expected to capture **/
    size_t num_frames;
    /** Array of dimensions of the dataset **/
    std::vector<long long unsigned int> frame_dimensions;
    /** Array of chunking dimensions of the dataset **/
    std::vector<long long unsigned int> chunks;
    /** Compression state of data **/
    CompressionType compression;
    /** Compression configuration settings **/
    unsigned int blosc_compressor;
    unsigned int blosc_level;
    unsigned int blosc_shuffle;
    /** Whether to create Low/High indexes for this dataset **/
    bool create_low_high_indexes;
};

/**
 * BLOSC_VERSION_FORMAT in blosc.h tells you the blosc-format
 * in the blosc library. We fix on 2 because we've always used that.
 */
static const int BLOSC_FORMAT_ODIN_USES = 2;

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_FrameProcessorDefinitions_H_ */

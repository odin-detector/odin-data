/*
 * FrameProcessorDefinitions.h
 *
 *  Created on: 31 Oct 2017
 *      Author: vtu42223
 */

#ifndef FRAMEPROCESSOR_FrameProcessorDefinitions_H_
#define FRAMEPROCESSOR_FrameProcessorDefinitions_H_

#include <sstream>
#include <stdexcept>
#include "stdint.h"

namespace FrameProcessor
{
  /**
   * Enumeration to store the pixel type of the incoming image
   */
  enum DataType { raw_8bit, raw_16bit, raw_32bit, raw_64bit, raw_float };
  /**
   * Enumeration to store the compression type of the incoming image
   */
  enum CompressionType { no_compression, lz4, bslz4, blosc };
  /**
   * Enumeration to store the compression type of the incoming image
   */
  enum ProcessFrameStatus { status_ok, status_complete, status_complete_missing_frames, status_invalid };


  /* Enum style arrays for the header*/
  const std::string DATA_TYPES[] = {"uint8","uint16","uint32","uint64","float"};
  const std::string COMPRESS_TYPES[] = {"none","LZ4","BSLZ4"};

  /**
   * Gets the size of the data type
   * \param[in] type - enum value
   * \return size_t data type size
   */
  static size_t get_size_from_enum(DataType type)
  {
    if (type == raw_8bit)
      return sizeof(uint8_t); // 1 byte
    else if (type == raw_16bit)
      return sizeof(uint16_t); // 2 bytes
    else if (type == raw_32bit)
      return sizeof(uint32_t); // 4 bytes
    else if (type == raw_64bit)
      return sizeof(uint64_t); // 8 bytes
    else if (type == raw_float)
        return sizeof(float);
    else {
      std::stringstream msg;
      msg << "Unable to determine data type size " << type;
      throw std::runtime_error(msg.str());
    }
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
  static std::string get_type_from_enum(DataType type)
  {
    if(type >= 0 && type < sizeof(DATA_TYPES)/sizeof(DATA_TYPES[0]))
    {
      return DATA_TYPES[type];
    }else{
      return "unknown";
    }
  }

  /**
   * Gets the type of compression, based on the enum value
   * 0: None, 1: LZ4, 2: BSLZ4
   * \param[in] compress - enum value
   * \return the string value representing the compression. Assumed none if the enum value is unrecognised.
   */
  static std::string get_compress_from_enum(CompressionType compress)
  {
    if(compress >= 0 && compress < sizeof(COMPRESS_TYPES)/sizeof(COMPRESS_TYPES[0]))
    {
      return COMPRESS_TYPES[compress];
    }else{
      return COMPRESS_TYPES[0];
    }
  }
  /**
   * Defines a dataset to be saved in HDF5 format.
   */
  struct DatasetDefinition
  {
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

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_FrameProcessorDefinitions_H_ */

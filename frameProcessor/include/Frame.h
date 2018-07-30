/*
 * Frame.h
 *
 * Created on: 15 Oct 2015
 * Author: up45
*/

#ifndef TOOLS_CLIENT_FRAMENOTIFIER_DATA_H_
#define TOOLS_CLIENT_FRAMENOTIFIER_DATA_H_

#include <string>
#include <stdint.h>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/shared_ptr.hpp>

#include <log4cxx/logger.h>

#include "DataBlock.h"
#include "DataBlockPool.h"
#include "IpcChannel.h"
#include "IpcMessage.h"
#include "FrameProcessorDefinitions.h"

/**
 *  Shared Buffer (IPC) Header
 */
typedef struct
{
  /** ID of the buffer manager */
  size_t manager_id;
  /** Number of buffers present in the shared memory block */
  size_t num_buffers;
  /** The size of each buffer in the shared memory block */
  size_t buffer_size;
} Header;


/** Frame Parameter Value */
union ParameterValue
{
    uint8_t i8_val;
    uint16_t i16_val;
    uint32_t i32_val;
    uint64_t i64_val;
    float float_val;
};

/**
 *  Frame Parameter
 */
typedef struct
{
  /** The value of this parameter */
  ParameterValue value;
  /** The type of this parameter */
  FrameProcessor::DataType type;
} Parameter;

typedef unsigned long long dimsize_t;
typedef std::vector<dimsize_t> dimensions_t;

//============== end of stolen bits ============================================================

namespace FrameProcessor
{

/** Store a raw dataset and associated parameters (meta data).
 *
 * The class provides access to DataBlock objects through the DataBlockPool
 * class which re-uses memory without re-allocating. It also provides methods
 * to store and access meta data for the raw data object.
 */
class Frame
{
public:
  Frame(const std::string& index);
  virtual ~Frame();
  void wrap_shared_buffer(const void* data_src,
                          size_t nbytes,
                          uint64_t bufferID,
                          int frameID,
                          OdinData::IpcChannel *relCh);
  void copy_data(const void* data_src, size_t nbytes);
  const void* get_data() const;
  size_t get_data_size() const;

  /** Get the dataset name for this Frame.
   *
   * This method returns the name of the dataset of the Frame.
   *
   * \return the name of the dataset.
   */
  const std::string& get_dataset_name() const { return dataset_name; }

  /** Set a dataset name for this Frame.
   *
   * This method sets the name of the dataset of the Frame.
   *
   * \param[in] dataset - the name of the dataset.
   */
  void set_dataset_name(const std::string& dataset) { this->dataset_name = dataset; }

  /** Set a frame number for this Frame.
   *
   * This method sets the frame number of the dataset of the Frame.
   *
   * \param[in] number - the frame number of the dataset.
   */
  void set_frame_number(unsigned long long number) { this->frameNumber_ = number; }

  /** Get the frame number for this Frame.
   *
   * This method returns the frame number of the dataset of the Frame.
   *
   * \return the frame number of the dataset.
   */
  unsigned long long get_frame_number() const { return this->frameNumber_; }

  /** Get the acquisition id for this Frame
   *
   * This method gets the id of the parent acquisition of this frame
   *
   * \return the id of the acquisition that this frame belongs to
   *
   */
  const std::string& get_acquisition_id() const { return acquisitionID_; }

  /** Set the acquisition id for this Frame
   *
   * This method sets the id of the parent acquisition of this frame
   *
   * \param[in] acquisitionID - the id of the acquisition that this frame belongs to
   *
   */
  void set_acquisition_id(const std::string& acquisitionID) { this->acquisitionID_ = acquisitionID; }

  void set_dimensions(const std::vector<unsigned long long>& dimensions);
  dimensions_t get_dimensions() const;
  int get_compression() const;
  int get_data_type() const;
  void set_parameter(const std::string& index, uint8_t value);
  void set_parameter(const std::string& index, uint16_t value);
  void set_parameter(const std::string& index, uint32_t value);
  void set_parameter(const std::string& index, uint64_t value);
  void set_parameter(const std::string& index, float value);
  std::map<std::string, Parameter> & get_parameters();
  Parameter get_parameter(const std::string& index) const;
  uint8_t get_i8_parameter(const std::string& index) const;
  uint16_t get_i16_parameter(const std::string& index) const;
  uint32_t get_i32_parameter(const std::string& index) const;
  uint64_t get_i64_parameter(const std::string& index) const;
  float get_float_parameter(const std::string& index) const;
  bool has_parameter(const std::string& index);
  void set_compression(int compression);
  void set_data_type(int data_type);

private:
  Frame();
  /**
   * Do not allow Frame copy
   */
  Frame(const Frame& src); // Don't try to copy one of these!

  /** Pointer to logger */
  log4cxx::LoggerPtr logger;
  /** Name of this dataset */
  std::string dataset_name;
  /** Index to retrieve data block from */
  std::string blockIndex_;
  /** Number of bytes per pixel */
  size_t bytes_per_pixel;
  /** Frame number */
  unsigned long long frameNumber_;
  /** Vector of dimensions */
  dimensions_t dimensions_;
  /** Compression type of raw data */
  int compression_;
  /** Data type of raw data */
  int data_type_;
  /** General parameter map */
  std::map<std::string, Parameter> parameters_;
  /** Pointer to raw data block */
  boost::shared_ptr<DataBlock> raw_;
  /** Pointer to shared memory raw block **/
  const void *shared_raw_;
  /** Shared memory size **/
  size_t shared_size_;
  /** Are we framing a shared memory buffer **/
  bool shared_memory_;
  /** Shared memory buffer ID **/
  uint64_t shared_id_;
  /** Shared memory frame ID **/
  int shared_frame_id_;
  /** ZMQ release channel for the shared buffer **/
  OdinData::IpcChannel *shared_channel_;
  /** Acquisition ID of the acquisition of this frame **/
  std::string acquisitionID_;
};

} /* namespace FrameProcessor */

#endif /* TOOLS_CLIENT_FRAMENOTIFIER_DATA_H_ */

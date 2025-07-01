/*
 * HDF5File.cpp
 *
 *  Created on: 31 Oct 2017
 *      Author: vtu42223
 */

#include "HDF5File.h"

#include <hdf5_hl.h>
#include "logging.h"
#include "DebugLevelLogger.h"

namespace FrameProcessor {

#define ensure_h5_result(success, message) ((success >= 0)                     \
    ? static_cast<void> (0)                                                    \
        : handle_h5_error(message, __PRETTY_FUNCTION__, __FILE__, __LINE__))

herr_t hdf5_error_cb(unsigned n, const H5E_error2_t* err_desc, void* client_data)
{
  HDF5File* fwPtr = (HDF5File*) client_data;
  fwPtr->hdf_error_handler(n, err_desc);
  return 0;
}

HDF5File::HDF5File(const HDF5ErrorDefinition_t& hdf5_error_definition) :
        hdf5_file_id_(-1),
        param_memspace_(-1),
        hdf5_error_flag_(false),
        file_index_(0),
        use_earliest_version_(false),
        unlimited_(false),
        watchdog_timer_(hdf5_error_definition.callback),
        hdf5_error_definition_(hdf5_error_definition)
{
  static bool hdf_initialised = false;
  this->logger_ = Logger::getLogger("FP.HDF5File");
  LOG4CXX_TRACE(logger_, "HDF5File constructor.");
  if (!hdf_initialised) {
    ensure_h5_result(H5Eset_auto2(H5E_DEFAULT, NULL, NULL), "H5Eset_auto2 failed");
    ensure_h5_result(H5Ewalk2(H5E_DEFAULT, H5E_WALK_DOWNWARD, hdf5_error_cb, this), "H5Ewalk2 failed");
    hdf_initialised = true;
  }
}

HDF5File::~HDF5File() {
  // Call to close file in case it hasn't been closed
  close_file();
  if (this->param_memspace_ >= 0) {
    ensure_h5_result(H5Sclose(this->param_memspace_), "H5Sclose failed");
  }
}

/**
 * Configure datasets to allow extension during write to an unlimited extent
 */
void HDF5File::set_unlimited() {
  if (hdf5_datasets_.empty()) {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting HDF5 datasets to use unlimited");
    unlimited_ = true;
  }
  else {
    throw std::runtime_error("Datasets have already been created. Cannot set unlimited.");
  }
}

/**
 * Handles an HDF5 error. Logs the error and throws a runtime exception
 *
 * \param[in] message - The error message to log
 * \param[in] function - The function that had the error
 * \param[in] filename - The filename
 * \param[in] line - The line number of the call
 */
void HDF5File::handle_h5_error(const std::string& message, const std::string& function,
                               const std::string& filename, int line)
{
  std::stringstream error;
  error << "HDF5 Function Error: (" << message << ") in " << filename << ":" << line << ": " << function;

  // Walk the HDF5 error stack and add each frame to hdf5_errors_
  H5Ewalk2(H5E_DEFAULT, H5E_WALK_DOWNWARD, hdf5_error_cb, (void *) this);
  // Iterate errors and add them to message to log
  std::stringstream full_error;
  full_error << error.str() << "\n HDF5 Stack Trace:";
  unsigned int frame = 0;
  std::vector<H5E_error2_t>::iterator it;
  for (it=hdf5_errors_.begin(); it != hdf5_errors_.end(); ++it) {
    full_error << "\n  [" << frame << "]: " << it->file_name << ":" << it->line << " in " << it->func_name << ": \"" << it->desc << "\"";
    frame++;
  }
  LOG4CXX_ERROR(logger_, full_error.str());
  this->clear_hdf_errors();

  throw std::runtime_error(error.str());
}

void HDF5File::hdf_error_handler(unsigned n, const H5E_error2_t* err_desc)
{
  // Protect this method
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  hdf5_error_flag_ = true;
  hdf5_errors_.push_back(*err_desc);
}

void HDF5File::clear_hdf_errors()
{
  // Protect this method
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  // Empty the error array
  hdf5_errors_.clear();
  // Now reset the error flag
  hdf5_error_flag_ = false;
}

/**
 * Create the HDF5 ready for writing datasets.
 *
 * \param[in] filename - Full file name of the file to create.
 * \param[in] file_index - File index of the file
 * \param[in] use_earliest_version - Whether to use the earliest version of HDF5 library
 * \param[in] alignment_threshold - Chunk threshold
 * \param[in] alignment_value - Chunk alignment value
 *
 * \return - The duration of the H5Fcreate call
 */
size_t HDF5File::create_file(std::string filename, size_t file_index, bool use_earliest_version, size_t alignment_threshold, size_t alignment_value)
{
  // Protect this method
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  hid_t fapl; // File access property list
  hid_t fcpl;
  filename_ = std::move(filename);
  use_earliest_version_ = use_earliest_version;

  // Create file access property list
  fapl = H5Pcreate(H5P_FILE_ACCESS);
  ensure_h5_result(fapl, "H5Pcreate failed to create the file access property list");

  ensure_h5_result(H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG), "H5Pset_fclose_degree failed");

  // Set chunk boundary alignment
  ensure_h5_result(H5Pset_alignment( fapl, alignment_threshold, alignment_value ), "H5Pset_alignment failed");

  // Set to use the desired library format
  if (use_earliest_version_) {
    ensure_h5_result(H5Pset_libver_bounds(fapl, H5F_LIBVER_EARLIEST, H5F_LIBVER_LATEST), "H5Pset_libver_bounds failed");
  } else {
    ensure_h5_result(H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST), "H5Pset_libver_bounds failed");
  }

  // Create file creation property list
  fcpl = H5Pcreate(H5P_FILE_CREATE);
  ensure_h5_result(fcpl, "H5Pcreate failed to create the file creation property list");

  // Creating the file with SWMR write access
  LOG4CXX_INFO(logger_, "Creating file: " << filename_);
  unsigned int flags = H5F_ACC_TRUNC;

  watchdog_timer_.start_timer("H5Fcreate", hdf5_error_definition_.create_duration);
  this->hdf5_file_id_ = H5Fcreate(filename_.c_str(), flags, fcpl, fapl);
  size_t create_duration = watchdog_timer_.finish_timer();

  ensure_h5_result(this->hdf5_file_id_, "Failed to create HDF5 file");
  if (this->hdf5_file_id_ < 0) {
    // Close file access property list
    ensure_h5_result(H5Pclose(fapl), "H5Pclose failed after create file failed");
    // Now throw a runtime error to explain that the file could not be created
    std::stringstream err;
    err << "Could not create file " << filename_;
    throw std::runtime_error(err.str().c_str());
  }
  // Close file access property list
  ensure_h5_result(H5Pclose(fapl), "H5Pclose failed to close the file access property list");
  ensure_h5_result(H5Pclose(fcpl), "H5Pclose failed to close the file creation property list");

  // Create the memspace for writing parameter datasets
  hsize_t elementSize[1] = {1};
  param_memspace_ = H5Screate_simple(1, elementSize, NULL);
  ensure_h5_result(param_memspace_, "Failed to create parameter dataspace");

  file_index_ = file_index;

  return create_duration;
}

/**
 * Close the currently open HDF5 file.
 *
 * \return - The hdf5 write metric with the durations of the write and flush calls
 */
size_t HDF5File::close_file() {
  // Protect this method
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  size_t close_duration = 0;
  if (this->hdf5_file_id_ >= 0) {
    // Close dataset handles
    std::map<std::string, HDF5Dataset_t>::iterator it;
    for(it = this->hdf5_datasets_.begin(); it != this->hdf5_datasets_.end(); ++it) {
      ensure_h5_result(H5Dclose(it->second.dataset_id), "H5Dclose failed");
    }
    this->hdf5_datasets_.clear();

    // Close file
    watchdog_timer_.start_timer("H5Fclose", hdf5_error_definition_.close_duration);
    hid_t status = H5Fclose(this->hdf5_file_id_);
    close_duration = watchdog_timer_.finish_timer();
    ensure_h5_result(status, "H5Fclose failed to close the file");
    this->hdf5_file_id_ = -1;
  }

  return close_duration;
}

/**
 * Write a frame to the file.
 *
 * \param[in] frame - Reference to the frame
 * \param[in] frame_offset - The offset in the file to write the frame into
 * \param[in] outer_chunk_dimension - The size of the outermost dimension of a chunk
 * \param[in] call_durations - Struct containing hdf5 call durations - write and flush will be updated
 *                             with the durations of the H5DOwrite_chunk and H5Dflush calls
 */
void HDF5File::write_frame(
    const Frame& frame,
    hsize_t frame_offset,
    uint64_t outer_chunk_dimension,
    HDF5CallDurations_t& call_durations
  ) {
  // Protect this method
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  LOG4CXX_TRACE(logger_, "Writing frame [" << frame.get_frame_number()
      << "] size [" << get_size_from_enum(frame.get_meta_data().get_data_type())
      << "] type [" << frame.get_meta_data().get_data_type()
      << "] name [" << frame.get_meta_data().get_dataset_name() << "]");
  HDF5Dataset_t& dset = this->get_hdf5_dataset(frame.get_meta_data().get_dataset_name());

  // We will need to extend the dataset in 1 dimension by the outer chunk dimension
  // For 3D datasets this would normally be 1 (a 2D image)
  // For 1D datasets this would normally be the quantity of data items present in a single chunk

  if (unlimited_) {
    this->extend_dataset(dset, (frame_offset + 1) * outer_chunk_dimension);
  }

  LOG4CXX_TRACE(logger_, "Writing frame offset=" << frame.get_frame_number()  << " (" << frame_offset <<
                         ") dset=" << frame.get_meta_data().get_dataset_name());

  // Set the offset
  std::vector<hsize_t>offset(dset.dataset_dimensions.size());
  offset[0] = frame_offset * outer_chunk_dimension;

  uint32_t filter_mask = 0x0;

  watchdog_timer_.start_timer("H5DOwrite_chunk", hdf5_error_definition_.write_duration);
  hid_t status = H5DOwrite_chunk(
    dset.dataset_id, H5P_DEFAULT, filter_mask, &offset.front(), frame.get_image_size(), frame.get_image_ptr()
  );
  unsigned int write_duration = watchdog_timer_.finish_timer();
  call_durations.write.update(write_duration);
  ensure_h5_result(status, "H5DOwrite_chunk failed");

#if H5_VERSION_GE(1,9,178)
  if (!use_earliest_version_) {
    watchdog_timer_.start_timer("H5Dflush", hdf5_error_definition_.flush_duration);
    hid_t status = H5Dflush(dset.dataset_id);
    unsigned int flush_duration = watchdog_timer_.finish_timer();
    call_durations.flush.update(flush_duration);
    ensure_h5_result(status, "Failed to flush data to disk");
  }
#endif

  // Check if the latest written frame has extended the dataset, and if it has then
  // adjust the actual_dataset_size_ member of the dset structure to match the real size
  if (offset[0] + frame.get_outer_chunk_size() > dset.actual_dataset_size_) {
    dset.actual_dataset_size_ = offset[0] + frame.get_outer_chunk_size();
  }
}

/**
 * Write a parameter to the file.
 *
 * \param[in] frame - Reference to the frame.
 * \param[in] dataset_definition - The dataset definition for this parameter.
 * \param[in] frame_offset - The offset to write the value to
 */
void HDF5File::write_parameter(const Frame& frame, DatasetDefinition dataset_definition, hsize_t frame_offset) {
  // Protect this method
  std::lock_guard<std::recursive_mutex> lock(mutex_);

  void* data_ptr;
  size_t size = 0;
  uint8_t u8value = 0;
  uint16_t u16value = 0;
  uint32_t u32value = 0;
  uint64_t u64value = 0;
  float f32value = 0;

  // Get the correct value and size from the parameter given its type
  switch( dataset_definition.data_type ) {
  case raw_8bit:
    u8value = frame.get_meta_data().get_parameter<uint8_t>(dataset_definition.name);
    data_ptr = &u8value;
    size = sizeof(uint8_t);
    break;
  case raw_16bit:
    u16value = frame.get_meta_data().get_parameter<uint16_t>(dataset_definition.name);
    data_ptr = &u16value;
    size = sizeof(uint16_t);
    break;
  case raw_32bit:
    u32value = frame.get_meta_data().get_parameter<uint32_t>(dataset_definition.name);
    data_ptr = &u32value;
    size = sizeof(uint32_t);
    break;
  case raw_64bit:
    u64value = frame.get_meta_data().get_parameter<uint64_t>(dataset_definition.name);
    data_ptr = &u64value;
    size = sizeof(uint64_t);
    break;
  case raw_float:
    f32value = frame.get_meta_data().get_parameter<float>(dataset_definition.name);
    data_ptr = &f32value;
    size = sizeof(float);
    break;
  default:
    u16value = frame.get_meta_data().get_parameter<uint16_t>(dataset_definition.name);
    data_ptr = &u16value;
    size = sizeof(uint16_t);
    break;
  }

  HDF5Dataset_t& dset = this->get_hdf5_dataset(dataset_definition.name);

  if (unlimited_) {
    this->extend_dataset(dset, frame_offset + 1);
  }

  LOG4CXX_TRACE(logger_, "Writing parameter [" << dataset_definition.name << "] at offset = " << frame_offset);

  // Set the offset
  std::vector<hsize_t>offset(dset.dataset_dimensions.size());
  offset[0] = frame_offset;

  // Create the hdf5 variables for writing
  hid_t dtype = datatype_to_hdf_type(dataset_definition.data_type);
  hsize_t elementSize[1] = {1};
  hid_t fspace = H5Dget_space(dset.dataset_id);
  ensure_h5_result(fspace, "Failed to get parameter dataset dataspace");

  // Select the hyperslab
  ensure_h5_result(H5Sselect_hyperslab(fspace, H5S_SELECT_SET, &offset.front(), NULL, elementSize, NULL),
      "H5Sselect_hyperslab failed");

  // Write the value to the dataset
  watchdog_timer_.start_timer("H5Dwrite", hdf5_error_definition_.write_duration);
  hid_t status = H5Dwrite(dset.dataset_id, dtype, param_memspace_, fspace, H5P_DEFAULT, data_ptr);
  watchdog_timer_.finish_timer();
  ensure_h5_result(status, "H5Dwrite failed");

  ensure_h5_result(H5Sclose(fspace), "H5Sclose failed");

  // Flush if necessary and update the time it was last flushed
#if H5_VERSION_GE(1,9,178)
  bool flush = false;
  boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
  std::map<std::string, boost::posix_time::ptime>::iterator flushed_iter;
  flushed_iter = last_flushed.find(dataset_definition.name);
  if (flushed_iter != last_flushed.end())
  {
    if ((now - flushed_iter->second).total_milliseconds() > PARAM_FLUSH_RATE)
    {
      flush = true;
      flushed_iter->second = boost::posix_time::microsec_clock::local_time();
    }
  }
  else
  {
    flush = true;
    last_flushed[dataset_definition.name] = now;
  }

  if (flush && !use_earliest_version_) {
    LOG4CXX_TRACE(logger_, "Flushing parameter [" << dataset_definition.name << "]");
    watchdog_timer_.start_timer("H5Dflush", hdf5_error_definition_.flush_duration);
    hid_t status = H5Dflush(dset.dataset_id);
    watchdog_timer_.finish_timer();
    ensure_h5_result(status, "Failed to flush data to disk");
  }
#endif
}

/**
 * Create a HDF5 dataset from the DatasetDefinition.
 *
 * \param[in] definition - Reference to the DatasetDefinition.
 * \param[in] low_index - Value of the lowest frame index in the file if in block mode.
 * \param[in] high_index - Value of the highest frame index in the file if in block mode.
 */
void HDF5File::create_dataset(const DatasetDefinition& definition, int low_index, int high_index)
{
  // Protect this method
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  // Handles all at the top so we can remember to close them
  hid_t dataspace = 0;
  hid_t prop = 0;
  hid_t dapl = 0;
  hid_t dtype = datatype_to_hdf_type(definition.data_type);
  size_t pixel_type_size = H5Tget_size(dtype);

  std::vector<hsize_t> frame_dims = definition.frame_dimensions;
  unsigned int frame_num_pixels = 1;
  std::vector<hsize_t>::iterator it;
  for (it=frame_dims.begin(); it != frame_dims.end(); ++it) {
    frame_num_pixels *= *it;
  }

  // Dataset dims: {1, <image size Y>, <image size X>}
  std::vector<hsize_t> dset_dims(1,1);
  dset_dims.insert(dset_dims.end(), frame_dims.begin(), frame_dims.end());

  // If chunking has not been defined then throw an error
  if (definition.chunks.size() != dset_dims.size()){
    throw std::runtime_error("Dataset chunk size not defined correctly");
  }
  std::vector<hsize_t> chunk_dims = definition.chunks;

  if (unlimited_) {
    std::vector<hsize_t> max_dims = dset_dims;
    max_dims[0] = H5S_UNLIMITED;
    // Create an unlimited dataspace with the given initial dimensions
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Creating unlimited dataspace");
    dataspace = H5Screate_simple(dset_dims.size(), &dset_dims.front(), &max_dims.front());
  }
  else {
    // Create a fixed size dataspace with the given dimensions
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Creating fixed size dataspace");
    dset_dims[0] = definition.num_frames;
    dataspace = H5Screate_simple(dset_dims.size(), &dset_dims.front(), NULL);
    // Limit outermost chunk dimension to maximum dataset size (H5Dchunk.c:891 [1.10.5])
    if (chunk_dims[0] > dset_dims[0]) {
      chunk_dims[0] = dset_dims[0];
    }
  }
  ensure_h5_result(dataspace, "H5Screate_simple failed to create the dataspace");

  /* Enable chunking  */
  std::stringstream ss;
  ss << "Chunking = " << chunk_dims[0];
  for (int index = 1; index < chunk_dims.size(); index++){
    ss << "," << chunk_dims[index];
  }
  LOG4CXX_DEBUG_LEVEL(1, logger_, ss.str());
  prop = H5Pcreate(H5P_DATASET_CREATE);
  ensure_h5_result(prop, "H5Pcreate failed to create the dataset");

  /* Enable defined compression mode */
  if (definition.compression == no_compression) {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Compression type: None");
  }
  else if (definition.compression == lz4){
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Compression type: LZ4");
    // Create cd_values for filter to set the LZ4 compression level
    unsigned int cd_values = 3;
    size_t cd_values_length = 1;
    ensure_h5_result(H5Pset_filter(prop, LZ4_FILTER, H5Z_FLAG_MANDATORY,
        cd_values_length, &cd_values), "H5Pset_filter failed to set the LZ4 filter");
  }
  else if (definition.compression == bslz4) {
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Compression type: BSLZ4");
    // Create cd_values for filter to set default block size and to enable LZ4
    unsigned int cd_values[2] = {0, 2};
    size_t cd_values_length = 2;
    ensure_h5_result(H5Pset_filter(prop, BSLZ4_FILTER, H5Z_FLAG_MANDATORY,
        cd_values_length, cd_values), "H5Pset_filter failed to set the BSLZ4 filter");
  }
  else if (definition.compression == blosc) {
    LOG4CXX_INFO(logger_, "Compression type: Blosc");
    // Create cd_values for filter to set default block size and to enable LZ4
    unsigned int cd_values[7] = {0, 0, 0, 0, 0, 0, 0};
    size_t cd_values_length = 7;
    cd_values[0] = 2;                                          // Blosc filter version: 2 (multiple compressors since Blosc 1.3)
    cd_values[1] = BLOSC_FORMAT_ODIN_USES;                     // Blosc buffer format version
    cd_values[2] = static_cast<unsigned int>(pixel_type_size); // type size
    cd_values[3] = frame_num_pixels * pixel_type_size;         // uncompressed size
    cd_values[4] = definition.blosc_level;                     // compression level
    cd_values[5] = definition.blosc_shuffle;                   // 0: shuffle not active, 1: shuffle, 2: bitshuffle
    cd_values[6] = definition.blosc_compressor;                // the actual Blosc compressor to use (default: LZ4). See blosc.h
    ensure_h5_result(H5Pset_filter(prop, BLOSC_FILTER, H5Z_FLAG_OPTIONAL,
                                   cd_values_length, cd_values), "H5Pset_filter failed to set the Blosc filter");
  }

  ensure_h5_result(H5Pset_chunk(prop, dset_dims.size(), &chunk_dims.front()), "H5Pset_chunk failed");

  char fill_value[8] = {0,0,0,0,0,0,0,0};
  ensure_h5_result(H5Pset_fill_value(prop, dtype, fill_value), "H5Pset_fill_value failed");

  dapl = H5Pcreate(H5P_DATASET_ACCESS);
  ensure_h5_result(dapl, "H5Pcreate failed to create the dataset access property list");

  /* Create dataset  */
  LOG4CXX_INFO(logger_, "Creating dataset: " << definition.name);
  HDF5Dataset_t dset;
  dset.dataset_id = H5Dcreate2(this->hdf5_file_id_, definition.name.c_str(), dtype, dataspace, H5P_DEFAULT, prop, dapl);
  ensure_h5_result(dset.dataset_id, "H5Dcreate2 failed");
  if (dset.dataset_id < 0) {
    // Unable to create the dataset, clean up resources
    ensure_h5_result(H5Pclose(prop), "H5Pclose failed to close the prop after failing to create the dataset");
    ensure_h5_result(H5Pclose(dapl), "H5Pclose failed to close the dapl after failing to create the dataset");
    ensure_h5_result(H5Sclose(dataspace), "H5Pclose failed to close the dataspace after failing to create the dataset");
    // Now throw a runtime error to notify that the dataset could not be created
    throw std::runtime_error("Unable to create the dataset");
  }

  /* Add attributes to dataset for low and high index so it can be read by Albula */
  if (definition.create_low_high_indexes)
  {
    hid_t space_inl = H5Screate(H5S_SCALAR);
    ensure_h5_result(space_inl, "Failed to create dataspace");
    hid_t attr_inl = H5Acreate2(dset.dataset_id, "image_nr_low", H5T_STD_I32LE, space_inl, H5P_DEFAULT, H5P_DEFAULT);
    ensure_h5_result(H5Awrite(attr_inl, H5T_STD_I32LE, &low_index), "Failed to write to low index attribute");
    ensure_h5_result(H5Aclose(attr_inl), "H5Aclose failed to close the low index attribute");
    ensure_h5_result(H5Sclose(space_inl), "H5Sclose failed to close the low index dataspace");

    hid_t space_inh = H5Screate(H5S_SCALAR);
    ensure_h5_result(space_inh, "Failed to create dataspace");
    hid_t attr_inh = H5Acreate2(dset.dataset_id, "image_nr_high", H5T_STD_I32LE, space_inh, H5P_DEFAULT, H5P_DEFAULT);
    ensure_h5_result(H5Awrite(attr_inh, H5T_STD_I32LE, &high_index), "Failed to write to high index attribute");
    ensure_h5_result(H5Aclose(attr_inh), "H5Aclose failed to close the high index attribute");
    ensure_h5_result(H5Sclose(space_inh), "H5Sclose failed to close the high index dataspace");
  }

  dset.dataset_dimensions = dset_dims;
  dset.dataset_offsets = std::vector<hsize_t>(3);
  dset.actual_dataset_size_ = 0;
  this->hdf5_datasets_[definition.name] = dset;

  LOG4CXX_DEBUG_LEVEL(1, logger_, "Closing intermediate open HDF objects");
  ensure_h5_result(H5Pclose(prop), "H5Pclose failed to close the prop");
  ensure_h5_result(H5Pclose(dapl), "H5Pclose failed to close the dapl");
  ensure_h5_result(H5Sclose(dataspace), "H5Pclose failed to close the dataspace");
}

/**
 * Get a HDF5Dataset_t definition by its name.
 *
 * The private map of HDF5 dataset definitions is searched and if found
 * the HDF5Dataset_t definition is returned. Throws a runtime error if
 * the dataset cannot be found.
 *
 * \param[in] dset_name - name of the dataset to search for.
 * \return - the dataset definition if found.
 */
HDF5File::HDF5Dataset_t& HDF5File::get_hdf5_dataset(const std::string& dset_name) {
  // Check if the frame destination dataset has been created
  if (this->hdf5_datasets_.find(dset_name) == this->hdf5_datasets_.end())
  {
    // no dataset of this name exist
    std::stringstream message;
    message << "Attempted to access non-existent dataset: \"" << dset_name << "\"\n";
    throw std::runtime_error(message.str());
  }
  return this->hdf5_datasets_.at(dset_name);
}

/** Extend the HDF5 dataset ready for new data
 *
 * Checks the frame_no is larger than the current dataset dimensions and then
 * sets the extent of the dataset to this new value.
 *
 * This is used in the case that the final size of the dataset is unknown initially
 * and set to H5S_UNLIMITED.
 *
 * \param[in] dset - Handle to the HDF5 dataset.
 * \param[in] frame_no - Number of the incoming frame to extend to.
 */
void HDF5File::extend_dataset(HDF5Dataset_t& dset, size_t frame_no) {
  if (frame_no > dset.dataset_dimensions[0]) {
    // Extend the dataset
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Extending dataset_dimensions[0] = " << frame_no);
    dset.dataset_dimensions[0] = frame_no;
    ensure_h5_result(H5Dset_extent( dset.dataset_id,
        &dset.dataset_dimensions.front()), "H5Dset_extent failed to extend the dataset");
  }
}

/** Read the current number of frames in an HDF5 dataset (including gaps, up to the highest written offset)
 *
 * \param[in] dataset - HDF5 dataset
 */
size_t HDF5File::get_dataset_frames(const std::string& dset_name)
{
  // Protect this method
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  return this->get_hdf5_dataset(dset_name).actual_dataset_size_;
}

/** Get the maximum size of the given dataset
 *
 * \param[in] dataset - HDF5 dataset
 * \return - 0 if unlimited_, else the extent of the outermost dimension of the dataset
 */
size_t HDF5File::get_dataset_max_size(const std::string& dset_name)
{
  // Protect this method
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  if (unlimited_) {
    return 0;
  } else {
    return this->get_hdf5_dataset(dset_name).dataset_dimensions[0];
  }
}

/**
 * Convert from a DataType type to the corresponding HDF5 type.
 *
 * \param[in] data_type - The DataType type to convert.
 * \return - the equivalent HDF5 type.
 */
hid_t HDF5File::datatype_to_hdf_type(DataType data_type) const {
  hid_t dtype = 0;
  switch(data_type)
  {
  case raw_64bit:
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Data type: UINT64");
    dtype = H5T_NATIVE_UINT64;
    break;
  case raw_32bit:
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Data type: UINT32");
    dtype = H5T_NATIVE_UINT32;
    break;
  case raw_16bit:
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Data type: UINT16");
    dtype = H5T_NATIVE_UINT16;
    break;
  case raw_8bit:
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Data type: UINT8");
    dtype = H5T_NATIVE_UINT8;
    break;
  case raw_float:
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Data type: FLOAT");
    dtype = H5T_NATIVE_FLOAT;
    break;
  default:
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Data type: UINT16");
    dtype = H5T_NATIVE_UINT16;
  }
  return dtype;
}

/**
 * Start SWMR writing
 */
void HDF5File::start_swmr() {
  // Protect this method
  std::lock_guard<std::recursive_mutex> lock(mutex_);
#if H5_VERSION_GE(1,9,178)
  if (!use_earliest_version_) {
    ensure_h5_result(H5Fstart_swmr_write(this->hdf5_file_id_), "Failed to enable SWMR writing");
  }
#endif
}

/**
 * Get the file index of this file
 *
 * \return - the file index (0 indexed)
 */
size_t HDF5File::get_file_index() {
  return file_index_;
}

/**
 * Get the file name of this file
 *
 * \return - the name of the file
 */
std::string HDF5File::get_filename() {
  return filename_;
}

} /* namespace FrameProcessor */

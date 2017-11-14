/*
 * HDF5FileWriter.cpp
 *
 *  Created on: 31 Oct 2017
 *      Author: vtu42223
 */

#include <hdf5_hl.h>

#include "Frame.h"
#include "HDF5FileWriter.h"

#include "logging.h"

namespace FrameProcessor {

#define ensure_h5_result(success, message) ((success >= 0)                     \
    ? static_cast<void> (0)                                                    \
        : handle_h5_error(message, __PRETTY_FUNCTION__, __FILE__, __LINE__))

herr_t hdf5_error_cb(unsigned n, const H5E_error2_t *err_desc, void* client_data)
{
  HDF5FileWriter *fwPtr = (HDF5FileWriter *)client_data;
  fwPtr->hdf_error_handler(n, err_desc);
  return 0;
}

HDF5FileWriter::HDF5FileWriter() :
        hdf5_file_id_(-1),
        hdf5_error_flag_(false),
        file_index_(0)
{
  static bool hdf_initialised = false;
  this->logger_ = Logger::getLogger("FP.HDF5FileWriter");
  this->logger_->setLevel(Level::getTrace());
  LOG4CXX_TRACE(logger_, "HDF5FileWriter constructor.");
  if (!hdf_initialised) {
    ensure_h5_result(H5Eset_auto2(H5E_DEFAULT, NULL, NULL), "H5Eset_auto2 failed");
    ensure_h5_result(H5Ewalk2(H5E_DEFAULT, H5E_WALK_DOWNWARD, hdf5_error_cb, this), "H5Ewalk2 failed");
    hdf_initialised = true;
  }
}

HDF5FileWriter::~HDF5FileWriter() {
  // Call to close file in case it hasn't been closed
  close_file();
}

/**
 * Handles an HDF5 error. Logs the error and throws a runtime exception
 *
 * \param[in] message - The error message to log
 * \param[in] function - The function that had the error
 * \param[in] filename - The filename
 * \param[in] line - The line number of the call
 */
void HDF5FileWriter::handle_h5_error(std::string message, std::string function, std::string filename, int line) const {
  std::stringstream err;
  err << "H5 function error: (" << message << ") in " << filename << ":" << line << ": " << function;
  LOG4CXX_ERROR(logger_, err.str());
  throw std::runtime_error(err.str().c_str());
}

void HDF5FileWriter::hdf_error_handler(unsigned n, const H5E_error2_t *err_desc)
{
  const int MSG_SIZE = 64;
  char maj[MSG_SIZE];
  char min[MSG_SIZE];
  char cls[MSG_SIZE];

  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);
  // Set the error flag true
  hdf5_error_flag_ = true;

  // Get descriptions for the major and minor error numbers
  H5Eget_class_name(err_desc->cls_id, cls, MSG_SIZE);
  H5Eget_msg(err_desc->maj_num, NULL, maj, MSG_SIZE);
  H5Eget_msg(err_desc->min_num, NULL, min, MSG_SIZE);

  // Record the error into the error stack
  std::stringstream err;
  err << "[" << cls << "] " << maj << " (" << min << ")";
  LOG4CXX_ERROR(logger_, "H5 error: " << err.str());
  hdf5_errors_.push_back(err.str());
}

bool HDF5FileWriter::check_for_hdf_errors()
{
  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);

  // Simply return the current error flag state
  return hdf5_error_flag_;
}

std::vector<std::string> HDF5FileWriter::read_hdf_errors()
{
  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);

  // Simply return the current error array
  return hdf5_errors_;
}

void HDF5FileWriter::clear_hdf_errors()
{
  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);

  // Empty the error array
  hdf5_errors_.clear();
  // Now reset the error flag
  hdf5_error_flag_ = false;
}

/**
 * Create the HDF5 ready for writing datasets.
 * Currently the file is created with the following:
 * Chunk boundary alignment is set to 4MB.
 * Using the latest library format
 * Created with SWMR access
 * chunk_align parameter not currently used
 *
 * \param[in] filename - Full file name of the file to create.
 * \param[in] chunk_align - Not currently used.
 */
void HDF5FileWriter::create_file(std::string filename, size_t file_index, size_t chunk_align)
{
  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);
  hid_t fapl; // File access property list
  hid_t fcpl;
  filename_ = filename;

  // Create file access property list
  fapl = H5Pcreate(H5P_FILE_ACCESS);
  ensure_h5_result(fapl, "H5Pcreate failed to create the file access property list");

  ensure_h5_result(H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG), "H5Pset_fclose_degree failed");

  // Set chunk boundary alignment to 4MB
  ensure_h5_result( H5Pset_alignment( fapl, 65536, 4*1024*1024 ), "H5Pset_alignment failed");

  // Set to use the latest library format
  ensure_h5_result(H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST), "H5Pset_libver_bounds failed");

  // Create file creation property list
  fcpl = H5Pcreate(H5P_FILE_CREATE);
  ensure_h5_result(fcpl, "H5Pcreate failed to create the file creation property list");

  // Creating the file with SWMR write access
  LOG4CXX_INFO(logger_, "Creating file: " << filename);
  unsigned int flags = H5F_ACC_TRUNC;
  this->hdf5_file_id_ = H5Fcreate(filename.c_str(), flags, fcpl, fapl);
  if (this->hdf5_file_id_ < 0) {
    // Close file access property list
    ensure_h5_result(H5Pclose(fapl), "H5Pclose failed after create file failed");
    // Now throw a runtime error to explain that the file could not be created
    std::stringstream err;
    err << "Could not create file " << filename;
    throw std::runtime_error(err.str().c_str());
  }
  // Close file access property list
  ensure_h5_result(H5Pclose(fapl), "H5Pclose failed to close the file access property list");

  file_index_ = file_index;

}

/**
 * Close the currently open HDF5 file.
 */
void HDF5FileWriter::close_file() {
  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);
  if (this->hdf5_file_id_ >= 0) {
    ensure_h5_result(H5Fclose(this->hdf5_file_id_), "H5Fclose failed to close the file");
    this->hdf5_file_id_ = -1;
  }
}

/**
 * Write a frame to the file.
 *
 * \param[in] frame - Reference to the frame.
 */
void HDF5FileWriter::write_frame(const Frame& frame, hsize_t frame_offset, uint64_t outer_chunk_dimension) {
  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);
  hsize_t frame_no = frame.get_frame_number();

  LOG4CXX_TRACE(logger_, "Writing frame [" << frame.get_frame_number()
      << "] size [" << frame.get_data_size()
      << "] type [" << frame.get_data_type()
      << "] name [" << frame.get_dataset_name() << "]");
  HDF5Dataset_t& dset = this->get_hdf5_dataset(frame.get_dataset_name());


  // We will need to extend the dataset in 1 dimension by the outer chunk dimension
  // For 3D datasets this would normally be 1 (a 2D image)
  // For 1D datasets this would normally be the quantity of data items present in a single chunk

  this->extend_dataset(dset, (frame_offset + 1) * outer_chunk_dimension);

  LOG4CXX_TRACE(logger_, "Writing frame offset=" << frame_no  <<
      " (" << frame_offset << ")" <<
      " dset=" << frame.get_dataset_name());

  // Set the offset
  std::vector<hsize_t>offset(dset.dataset_dimensions.size());
  offset[0] = frame_offset * outer_chunk_dimension;

  uint32_t filter_mask = 0x0;
  ensure_h5_result(H5DOwrite_chunk(dset.dataset_id, H5P_DEFAULT,
      filter_mask, &offset.front(),
      frame.get_data_size(), frame.get_data()), "H5DOwrite_chunk failed");

#if H5_VERSION_GE(1,9,178)
  ensure_h5_result(H5Dflush(dset.dataset_id), "Failed to flush data to disk");
#endif
}

/**
 * Create a HDF5 dataset from the DatasetDefinition.
 *
 * \param[in] definition - Reference to the DatasetDefinition.
 */
void HDF5FileWriter::create_dataset(const DatasetDefinition& definition)
{
  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);
  // Handles all at the top so we can remember to close them
  hid_t dataspace = 0;
  hid_t prop = 0;
  hid_t dapl = 0;
  hid_t dtype = pixel_to_hdf_type(definition.pixel);

  std::vector<hsize_t> frame_dims = definition.frame_dimensions;

  // Dataset dims: {1, <image size Y>, <image size X>}
  std::vector<hsize_t> dset_dims(1,1);
  dset_dims.insert(dset_dims.end(), frame_dims.begin(), frame_dims.end());

  // If chunking has not been defined then throw an error
  if (definition.chunks.size() != dset_dims.size()){
    throw std::runtime_error("Dataset chunk size not defined correctly");
  }
  std::vector<hsize_t> chunk_dims = definition.chunks;

  std::vector<hsize_t> max_dims = dset_dims;
  max_dims[0] = H5S_UNLIMITED;

  /* Create the dataspace with the given dimensions - and max dimensions */
  dataspace = H5Screate_simple(dset_dims.size(), &dset_dims.front(), &max_dims.front());
  ensure_h5_result(dataspace, "H5Screate_simple failed to create the dataspace");

  /* Enable chunking  */
  std::stringstream ss;
  ss << "Chunking = " << chunk_dims[0];
  for (int index = 1; index < chunk_dims.size(); index++){
    ss << "," << chunk_dims[index];
  }
  LOG4CXX_DEBUG(logger_, ss.str());
  prop = H5Pcreate(H5P_DATASET_CREATE);
  ensure_h5_result(prop, "H5Pcreate failed to create the dataset");

  /* Enable defined compression mode */
  if (definition.compression == no_compression) {
    LOG4CXX_INFO(logger_, "Compression type: None");
  }
  else if (definition.compression == lz4){
    LOG4CXX_INFO(logger_, "Compression type: LZ4");
    // Create cd_values for filter to set the LZ4 compression level
    unsigned int cd_values = 3;
    size_t cd_values_length = 1;
    ensure_h5_result(H5Pset_filter(prop, LZ4_FILTER, H5Z_FLAG_MANDATORY,
        cd_values_length, &cd_values), "H5Pset_filter failed to set the LZ4 filter");
  }
  else if (definition.compression == bslz4) {
    LOG4CXX_INFO(logger_, "Compression type: BSLZ4");
    // Create cd_values for filter to set default block size and to enable LZ4
    unsigned int cd_values[2] = {0, 2};
    size_t cd_values_length = 2;
    ensure_h5_result(H5Pset_filter(prop, BSLZ4_FILTER, H5Z_FLAG_MANDATORY,
        cd_values_length, cd_values), "H5Pset_filter failed to set the BSLZ4 filter");
  }

  ensure_h5_result(H5Pset_chunk(prop, dset_dims.size(), &chunk_dims.front()), "H5Pset_chunk failed");

  char fill_value[8] = {0,0,0,0,0,0,0,0};
  ensure_h5_result(H5Pset_fill_value(prop, dtype, fill_value), "H5Pset_fill_value failed");

  dapl = H5Pcreate(H5P_DATASET_ACCESS);
  ensure_h5_result(dapl, "H5Pcreate failed to create the dataset access property list");

  /* Create dataset  */
  LOG4CXX_INFO(logger_, "Creating dataset: " << definition.name);
  HDF5Dataset_t dset;
  dset.dataset_id = H5Dcreate2(this->hdf5_file_id_, definition.name.c_str(),
      dtype, dataspace,
      H5P_DEFAULT, prop, dapl);
  if (dset.dataset_id < 0) {
    // Unable to create the dataset, clean up resources
    ensure_h5_result( H5Pclose(prop), "H5Pclose failed to close the prop after failing to create the dataset");
    ensure_h5_result( H5Pclose(dapl), "H5Pclose failed to close the dapl after failing to create the dataset");
    ensure_h5_result( H5Sclose(dataspace), "H5Pclose failed to close the dataspace after failing to create the dataset");
    // Now throw a runtime error to notify that the dataset could not be created
    throw std::runtime_error("Unable to create the dataset");
  }
  dset.dataset_dimensions = dset_dims;
  dset.dataset_offsets = std::vector<hsize_t>(3);
  this->hdf5_datasets_[definition.name] = dset;

  LOG4CXX_DEBUG(logger_, "Closing intermediate open HDF objects");
  ensure_h5_result( H5Pclose(prop), "H5Pclose failed to close the prop");
  ensure_h5_result( H5Pclose(dapl), "H5Pclose failed to close the dapl");
  ensure_h5_result( H5Sclose(dataspace), "H5Pclose failed to close the dataspace");
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
HDF5FileWriter::HDF5Dataset_t& HDF5FileWriter::get_hdf5_dataset(const std::string dset_name) {
  // Check if the frame destination dataset has been created
  if (this->hdf5_datasets_.find(dset_name) == this->hdf5_datasets_.end())
  {
    // no dataset of this name exist
    LOG4CXX_ERROR(logger_, "Attempted to access non-existent dataset: \"" << dset_name << "\"\n");
    throw std::runtime_error("Attempted to access non-existent dataset");
  }
  return this->hdf5_datasets_.at(dset_name);
}

/** Extend the HDF5 dataset ready for new data
 *
 * Checks the frame_no is larger than the current dataset dimensions and then
 * sets the extent of the dataset to this new value.
 *
 * \param[in] dset - Handle to the HDF5 dataset.
 * \param[in] frame_no - Number of the incoming frame to extend to.
 */
void HDF5FileWriter::extend_dataset(HDF5Dataset_t& dset, size_t frame_no) const {
  if (frame_no > dset.dataset_dimensions[0]) {
    // Extend the dataset
    LOG4CXX_DEBUG(logger_, "Extending dataset_dimensions[0] = " << frame_no);
    dset.dataset_dimensions[0] = frame_no;
    ensure_h5_result(H5Dset_extent( dset.dataset_id,
        &dset.dataset_dimensions.front()), "H5Dset_extent failed to extend the dataset");
  }
}

/** Read the current number of frames in a HDF5 dataset
 *
 * \param[in] dataset - HDF5 dataset
 */
size_t HDF5FileWriter::get_dataset_frames(const std::string dset_name)
{
  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);
  hid_t dspace = H5Dget_space(this->get_hdf5_dataset(dset_name).dataset_id);
  const int ndims = H5Sget_simple_extent_ndims(dspace);
  hsize_t dims[ndims];
  H5Sget_simple_extent_dims(dspace, dims, NULL);

  return (size_t)dims[0];
}

/**
 * Convert from a PixelType type to the corresponding HDF5 type.
 *
 * \param[in] pixel - The PixelType type to convert.
 * \return - the equivalent HDF5 type.
 */
hid_t HDF5FileWriter::pixel_to_hdf_type(PixelType pixel) const {
  hid_t dtype = 0;
  switch(pixel)
  {
  case pixel_raw_64bit:
    LOG4CXX_DEBUG(logger_, "Data type: UINT64");
    dtype = H5T_NATIVE_UINT64;
    break;
  case pixel_float32:
    LOG4CXX_DEBUG(logger_, "Data type: UINT32");
    dtype = H5T_NATIVE_UINT32;
    break;
  case pixel_raw_16bit:
    LOG4CXX_DEBUG(logger_, "Data type: UINT16");
    dtype = H5T_NATIVE_UINT16;
    break;
  case pixel_raw_8bit:
    LOG4CXX_DEBUG(logger_, "Data type: UINT8");
    dtype = H5T_NATIVE_UINT8;
    break;
  default:
    LOG4CXX_DEBUG(logger_, "Data type: UINT16");
    dtype = H5T_NATIVE_UINT16;
  }
  return dtype;
}

/**
 * Start SWMR writing
 */
void HDF5FileWriter::start_swmr() {
  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);
#if H5_VERSION_GE(1,9,178)
  ensure_h5_result(H5Fstart_swmr_write(this->hdf5_file_id_), "Failed to enable SWMR writing");
#endif
}

/**
 * Get the file index that this writer is writing
 *
 * \return - the file index (0 indexed)
 */
size_t HDF5FileWriter::get_file_index() {
  return file_index_;
}

/**
 * Get the file name that this writer is writing
 *
 * \return - the name of the file
 */
std::string HDF5FileWriter::get_filename() {
  return filename_;
}

} /* namespace FrameProcessor */

/*
 * FileWriter.cpp
 *
 */

#include <assert.h>

#include <boost/filesystem.hpp>
#include <hdf5_hl.h>

#include "Frame.h"
#include "FileWriterPlugin.h"

namespace FrameProcessor
{

const std::string FileWriterPlugin::CONFIG_PROCESS             = "process";
const std::string FileWriterPlugin::CONFIG_PROCESS_NUMBER      = "number";
const std::string FileWriterPlugin::CONFIG_PROCESS_RANK        = "rank";
const std::string FileWriterPlugin::CONFIG_PROCESS_BLOCKSIZE   = "frames_per_block";

const std::string FileWriterPlugin::CONFIG_FILE                = "file";
const std::string FileWriterPlugin::CONFIG_FILE_NAME           = "name";
const std::string FileWriterPlugin::CONFIG_FILE_PATH           = "path";

const std::string FileWriterPlugin::CONFIG_DATASET             = "dataset";
const std::string FileWriterPlugin::CONFIG_DATASET_CMD         = "cmd";
const std::string FileWriterPlugin::CONFIG_DATASET_NAME        = "name";
const std::string FileWriterPlugin::CONFIG_DATASET_TYPE        = "datatype";
const std::string FileWriterPlugin::CONFIG_DATASET_DIMS        = "dims";
const std::string FileWriterPlugin::CONFIG_DATASET_CHUNKS      = "chunks";
const std::string FileWriterPlugin::CONFIG_DATASET_COMPRESSION = "compression";

const std::string FileWriterPlugin::CONFIG_FRAMES              = "frames";
const std::string FileWriterPlugin::CONFIG_MASTER_DATASET      = "master";
const std::string FileWriterPlugin::CONFIG_OFFSET_ADJUSTMENT   = "offset";
const std::string FileWriterPlugin::CONFIG_WRITE               = "write";
const std::string FileWriterPlugin::ACQUISITION_ID             = "acquisition_id";
const std::string FileWriterPlugin::CLOSE_TIMEOUT_PERIOD       = "timeout_timer_period";
const std::string FileWriterPlugin::START_CLOSE_TIMEOUT        = "start_timeout_timer";

#define ensureH5result(success, message) ((success >= 0)                     \
  ? static_cast<void> (0)                                                    \
  : handleH5error(message, __PRETTY_FUNCTION__, __FILE__, __LINE__))

herr_t hdf5_error_cb(unsigned n, const H5E_error2_t *err_desc, void* client_data)
{
  FileWriterPlugin *fwPtr = (FileWriterPlugin *)client_data;
  fwPtr->hdfErrorHandler(n, err_desc);
  return 0;
}

/**
 * Create a FileWriterPlugin with default values.
 * File path is set to default of current directory, and the
 * filename is set to a default.
 *
 * The writer plugin is also configured to be a single
 * process writer (no other expected writers) with an offset
 * of 0.
 */
FileWriterPlugin::FileWriterPlugin() :
    writing_(false),
    framesWritten_(0),
    framesProcessed_(0),
    concurrent_processes_(1),
    concurrent_rank_(0),
    hdf5_fileid_(0),
    hdf5ErrorFlag_(false),
    frame_offset_adjustment_(0),
	frames_per_block_(1),
    timeoutPeriod(0),
    timeoutThreadRunning(true),
	timeoutThread(boost::bind(&FileWriterPlugin::runCloseFileTimeout, this))
{
  this->logger_ = Logger::getLogger("FP.FileWriterPlugin");
  this->logger_->setLevel(Level::getTrace());
  LOG4CXX_TRACE(logger_, "FileWriterPlugin constructor.");

  ensureH5result(H5Eset_auto2(H5E_DEFAULT, NULL, NULL), "H5Eset_auto2 failed");
  ensureH5result(H5Ewalk2(H5E_DEFAULT, H5E_WALK_DOWNWARD, hdf5_error_cb, this), "H5Ewalk2 failed");
  //H5Eset_auto2(H5E_DEFAULT, my_hdf5_error_handler, NULL);
}

/**
 * Destructor.
 */
FileWriterPlugin::~FileWriterPlugin()
{
  timeoutThreadRunning = false;
  timeoutActive = false;
  // Notify the close timeout thread to clean up resources
  {
    boost::mutex::scoped_lock lock2(m_closeFileMutex);
    m_timeoutCondition.notify_all();
  }
  {
    boost::mutex::scoped_lock lock(m_startTimeoutMutex);
    m_startCondition.notify_all();
  }
  timeoutThread.join();
  if (writing_) {
    stopWriting();
  }
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
void FileWriterPlugin::createFile(std::string filename, size_t chunk_align)
{
  hid_t fapl; // File access property list
  hid_t fcpl;

  // Create file access property list
  fapl = H5Pcreate(H5P_FILE_ACCESS);
  ensureH5result(fapl, "H5Pcreate failed to create the file access property list");

  ensureH5result(H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG), "H5Pset_fclose_degree failed");

  // Set chunk boundary alignment to 4MB
  ensureH5result( H5Pset_alignment( fapl, 65536, 4*1024*1024 ), "H5Pset_alignment failed");

  // Set to use the latest library format
  ensureH5result(H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST), "H5Pset_libver_bounds failed");

  // Create file creation property list
  fcpl = H5Pcreate(H5P_FILE_CREATE);
  ensureH5result(fcpl, "H5Pcreate failed to create the file creation property list");

  // Creating the file with SWMR write access
  LOG4CXX_INFO(logger_, "Creating file: " << filename);
  unsigned int flags = H5F_ACC_TRUNC;
  this->hdf5_fileid_ = H5Fcreate(filename.c_str(), flags, fcpl, fapl);
  if (this->hdf5_fileid_ < 0) {
    // Close file access property list
	ensureH5result(H5Pclose(fapl), "H5Pclose failed after create file failed");
    // Now throw a runtime error to explain that the file could not be created
    std::stringstream err;
    err << "Could not create file " << filename;
    throw std::runtime_error(err.str().c_str());
  }
  // Close file access property list
  ensureH5result(H5Pclose(fapl), "H5Pclose failed to close the file access property list");

  // Send meta data message to notify of file creation
  publishMeta("createfile", filename, getCreateMetaHeader());
}

/**
 * Write a frame to the file.
 *
 * \param[in] frame - Reference to the frame.
 */
void FileWriterPlugin::writeFrame(const Frame& frame) {
  hsize_t frame_no = frame.get_frame_number();

  LOG4CXX_TRACE(logger_, "Writing frame [" << frame.get_frame_number()
		  << "] size [" << frame.get_data_size()
		  << "] type [" << frame.get_data_type()
		  << "] name [" << frame.get_dataset_name() << "]");
  HDF5Dataset_t& dset = this->get_hdf5_dataset(frame.get_dataset_name());

  hsize_t frame_offset = 0;
  frame_offset = this->getFrameOffset(frame_no);
  // We will need to extend the dataset in 1 dimension by the outer chunk dimension
  // For 3D datasets this would normally be 1 (a 2D image)
  // For 1D datasets this would normally be the quantity of data items present in a single chunk
  uint64_t outer_chunk_dimension = 1;
  if (this->currentAcquisition_.dataset_defs_.size() != 0){
	  outer_chunk_dimension = this->currentAcquisition_.dataset_defs_[frame.get_dataset_name()].chunks[0];
  }
  this->extend_dataset(dset, (frame_offset + 1) * outer_chunk_dimension);

  LOG4CXX_TRACE(logger_, "Writing frame offset=" << frame_no  <<
                         " (" << frame_offset << ")" <<
                         " dset=" << frame.get_dataset_name());

  // Set the offset
  std::vector<hsize_t>offset(dset.dataset_dimensions.size());
  offset[0] = frame_offset * outer_chunk_dimension;

  uint32_t filter_mask = 0x0;
  ensureH5result(H5DOwrite_chunk(dset.datasetid, H5P_DEFAULT,
                           filter_mask, &offset.front(),
                           frame.get_data_size(), frame.get_data()), "H5DOwrite_chunk failed");

#if H5_VERSION_GE(1,9,178)
  ensureH5result(H5Dflush(dset.datasetid), "Failed to flush data to disk");
#endif

  // Send the meta message containing the frame written and the offset written to
  rapidjson::Document document;
  document.SetObject();

  // Add Frame number
  rapidjson::Value keyFrame("frame", document.GetAllocator());
  rapidjson::Value valueFrame;
  valueFrame.SetUint64(frame_no);
  document.AddMember(keyFrame, valueFrame, document.GetAllocator());

  // Add offset
  rapidjson::Value keyOffset("offset", document.GetAllocator());
  rapidjson::Value valueOffset;
  valueOffset.SetUint64(frame_offset);
  document.AddMember(keyOffset, valueOffset, document.GetAllocator());

  // Add rank
  rapidjson::Value keyRank("rank", document.GetAllocator());
  rapidjson::Value valueRank;
  valueRank.SetUint64(concurrent_rank_);
  document.AddMember(keyRank, valueRank, document.GetAllocator());

  // Add num consumers
  rapidjson::Value keyNumProcesses("proc", document.GetAllocator());
  rapidjson::Value valueNumProcesses;
  valueNumProcesses.SetUint64(concurrent_processes_);
  document.AddMember(keyNumProcesses, valueNumProcesses, document.GetAllocator());

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  document.Accept(writer);

  publishMeta("writeframe", buffer.GetString(), getMetaHeader());
}

/**
 * Write horizontal subframes direct to dataset chunk.
 *
 * \param[in] frame - Reference to a frame object containing the subframe.
 */
void FileWriterPlugin::writeSubFrames(const Frame& frame) {
  uint32_t filter_mask = 0x0;
  hsize_t frame_no = frame.get_frame_number();

  HDF5Dataset_t& dset = this->get_hdf5_dataset(frame.get_dataset_name());

  hsize_t frame_offset = 0;
  frame_offset = this->getFrameOffset(frame_no);
  LOG4CXX_DEBUG(logger_, "Frame offset: " << frame_offset);

  this->extend_dataset(dset, frame_offset + 1);

  LOG4CXX_DEBUG(logger_, "Writing frame=" << frame_no <<
                         " (" << frame_offset << ")" <<
                         " dset=" << frame.get_dataset_name());

  // Set the offset
  std::vector<hsize_t>offset(dset.dataset_dimensions.size(), 0);
  offset[0] = frame_offset;

  for (int i = 0; i < frame.get_parameter("subframe_count"); i++)
  {
    offset[2] = i * frame.get_dimensions("subframe")[1]; // For P2M: subframe is 704 pixels
    LOG4CXX_DEBUG(logger_, "    offset=" << offset[0] << ","
                                         << offset[1] << ","
                                         << offset[2]);

    LOG4CXX_DEBUG(logger_, "    subframe_size=" << frame.get_parameter("subframe_size"));

    ensureH5result(H5DOwrite_chunk(dset.datasetid, H5P_DEFAULT,
                             filter_mask, &offset.front(),
                             frame.get_parameter("subframe_size"),
                             (static_cast<const char*>(frame.get_data())+(i*frame.get_parameter("subframe_size")))), "H5DOwrite_chunk failed");
  }
}

/**
 * Create a HDF5 dataset from the DatasetDefinition.
 *
 * \param[in] definition - Reference to the DatasetDefinition.
 */
void FileWriterPlugin::createDataset(const FileWriterPlugin::DatasetDefinition& definition)
{
  // Handles all at the top so we can remember to close them
  hid_t dataspace = 0;
  hid_t prop = 0;
  hid_t dapl = 0;
  hid_t dtype = pixelToHdfType(definition.pixel);

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
  ensureH5result(dataspace, "H5Screate_simple failed to create the dataspace");

  /* Enable chunking  */
  std::stringstream ss;
  ss << "Chunking = " << chunk_dims[0];
  for (int index = 1; index < chunk_dims.size(); index++){
	  ss << "," << chunk_dims[index];
  }
  LOG4CXX_DEBUG(logger_, ss.str());
  prop = H5Pcreate(H5P_DATASET_CREATE);
  ensureH5result(prop, "H5Pcreate failed to create the dataset");

  /* Enable defined compression mode */
  if (definition.compression == no_compression) {
    LOG4CXX_INFO(logger_, "Compression type: None");
  }
  else if (definition.compression == lz4){
    LOG4CXX_INFO(logger_, "Compression type: LZ4");
    // Create cd_values for filter to set the LZ4 compression level
    unsigned int cd_values = 3;
    size_t cd_values_length = 1;
    ensureH5result(H5Pset_filter(prop, LZ4_FILTER, H5Z_FLAG_MANDATORY,
                           cd_values_length, &cd_values), "H5Pset_filter failed to set the LZ4 filter");
  }
  else if (definition.compression == bslz4) {
    LOG4CXX_INFO(logger_, "Compression type: BSLZ4");
    // Create cd_values for filter to set default block size and to enable LZ4
    unsigned int cd_values[2] = {0, 2};
    size_t cd_values_length = 2;
    ensureH5result(H5Pset_filter(prop, BSLZ4_FILTER, H5Z_FLAG_MANDATORY,
                           cd_values_length, cd_values), "H5Pset_filter failed to set the BSLZ4 filter");
  }

  ensureH5result(H5Pset_chunk(prop, dset_dims.size(), &chunk_dims.front()), "H5Pset_chunk failed");

  char fill_value[8] = {0,0,0,0,0,0,0,0};
  ensureH5result(H5Pset_fill_value(prop, dtype, fill_value), "H5Pset_fill_value failed");

  dapl = H5Pcreate(H5P_DATASET_ACCESS);
  ensureH5result(dapl, "H5Pcreate failed to create the dataset access property list");

  /* Create dataset  */
  LOG4CXX_INFO(logger_, "Creating dataset: " << definition.name);
  FileWriterPlugin::HDF5Dataset_t dset;
  dset.datasetid = H5Dcreate2(this->hdf5_fileid_, definition.name.c_str(),
                              dtype, dataspace,
                              H5P_DEFAULT, prop, dapl);
  if (dset.datasetid < 0) {
    // Unable to create the dataset, clean up resources
    ensureH5result( H5Pclose(prop), "H5Pclose failed to close the prop after failing to create the dataset");
    ensureH5result( H5Pclose(dapl), "H5Pclose failed to close the dapl after failing to create the dataset");
    ensureH5result( H5Sclose(dataspace), "H5Pclose failed to close the dataspace after failing to create the dataset");
    // Now throw a runtime error to notify that the dataset could not be created
    throw std::runtime_error("Unable to create the dataset");
  }
  dset.dataset_dimensions = dset_dims;
  dset.dataset_offsets = std::vector<hsize_t>(3);
  this->hdf5_datasets_[definition.name] = dset;

  LOG4CXX_DEBUG(logger_, "Closing intermediate open HDF objects");
  ensureH5result( H5Pclose(prop), "H5Pclose failed to close the prop");
  ensureH5result( H5Pclose(dapl), "H5Pclose failed to close the dapl");
  ensureH5result( H5Sclose(dataspace), "H5Pclose failed to close the dataspace");
}

/**
 * Close the currently open HDF5 file.
 */
void FileWriterPlugin::closeFile() {
  LOG4CXX_INFO(logger_, "Closing file " << this->currentAcquisition_.filePath_ << "/" << this->currentAcquisition_.fileName_);
  if (this->hdf5_fileid_ >= 0) {
    ensureH5result(H5Fclose(this->hdf5_fileid_), "H5Fclose failed to close the file");
    this->hdf5_fileid_ = 0;
    // Send meta data message to notify of file creation
    publishMeta("closefile", "", getMetaHeader());
  }
}

/**
 * Convert from a PixelType type to the corresponding HDF5 type.
 *
 * \param[in] pixel - The PixelType type to convert.
 * \return - the equivalent HDF5 type.
 */
hid_t FileWriterPlugin::pixelToHdfType(FileWriterPlugin::PixelType pixel) const {
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
 * Get a HDF5Dataset_t definition by its name.
 *
 * The private map of HDF5 dataset definitions is searched and i found
 * the HDF5Dataset_t definition is returned. Throws a runtime error if
 * the dataset cannot be found.
 *
 * \param[in] dset_name - name of the dataset to search for.
 * \return - the dataset definition if found.
 */
FileWriterPlugin::HDF5Dataset_t& FileWriterPlugin::get_hdf5_dataset(const std::string dset_name) {
  // Check if the frame destination dataset has been created
  if (this->hdf5_datasets_.find(dset_name) == this->hdf5_datasets_.end())
  {
    // no dataset of this name exist
    LOG4CXX_ERROR(logger_, "Attempted to access non-existent dataset: \"" << dset_name << "\"\n");
    throw std::runtime_error("Attempted to access non-existent dataset");
  }
  return this->hdf5_datasets_.at(dset_name);
}

/**
 * Return the dataset offset for the supplied frame number.
 *
 * This method checks that the frame really belongs to this writer instance
 * in the case of multiple writer instances. It then calculates the dataset
 * offset for this frame, which is the offset divided by the number of FileWriterPlugin
 * concurrent processes.
 *
 * \param[in] frame_no - Frame number of the frame.
 * \return - the dataset offset for the frame number.
 */
size_t FileWriterPlugin::getFrameOffset(size_t frame_no) const {
  size_t frame_offset = this->adjustFrameOffset(frame_no);

  if (this->concurrent_processes_ > 1) {
    // Check whether this frame should really be in this process
    // Note: this expects the frame numbering from HW/FW to start at 0, not 1!
    if ((frame_offset / frames_per_block_) % this->concurrent_processes_ != this->concurrent_rank_) {
      LOG4CXX_WARN(logger_,"Unexpected frame: " << frame_no <<
                           " in this process rank: " << this->concurrent_rank_);
      throw std::runtime_error("Unexpected frame in this process rank");
    }

    // Calculate the new offset based on how many concurrent processes are running
    size_t block_index = frame_offset / (frames_per_block_ * this->concurrent_processes_);
    size_t first_frame_offset_of_block = block_index * frames_per_block_;
    size_t offset_within_block = frame_offset % frames_per_block_;
    frame_offset = first_frame_offset_of_block + offset_within_block;
  }
  return frame_offset;
}

/** Adjust the incoming frame number with an offset
 *
 * This is a hacky work-around a missing feature in the Mezzanine
 * firmware: the frame number is never reset and is ever incrementing.
 * The file writer can deal with it, by inserting the frame right at
 * the end of a very large dataset (fortunately sparsely written to disk).
 *
 * This function latches the first frame number and subtracts this number
 * from every incoming frame.
 *
 * Throws a std::range_error if a frame is received which has a smaller
 * frame number than the initial frame used to set the offset.
 *
 * Returns the dataset offset for frame number (frame_no)
 */
size_t FileWriterPlugin::adjustFrameOffset(size_t frame_no) const {
  size_t frame_offset = 0;
  if (frame_no < this->frame_offset_adjustment_) {
    // Deal with a frame arriving after the very first frame
    // which was used to set the offset: throw a range error
    throw std::range_error("Frame out of order at start causing negative file offset");
  }

  // Normal case: apply offset
  LOG4CXX_DEBUG(logger_, "Raw frame number: " << frame_no);
  LOG4CXX_DEBUG(logger_, "Frame offset adjustment: " << frame_offset_adjustment_);
  frame_offset = frame_no - this->frame_offset_adjustment_;
  return frame_offset;
}

/** Set frame offset
 *
 */
void FileWriterPlugin::setFrameOffsetAdjustment(size_t frame_no) {
  this->frame_offset_adjustment_ = frame_no;
}

/** Extend the HDF5 dataset ready for new data
 *
 * Checks the frame_no is larger than the current dataset dimensions and then
 * sets the extent of the dataset to this new value.
 *
 * \param[in] dset - Handle to the HDF5 dataset.
 * \param[in] frame_no - Number of the incoming frame to extend to.
 */
void FileWriterPlugin::extend_dataset(HDF5Dataset_t& dset, size_t frame_no) const {
  if (frame_no > dset.dataset_dimensions[0]) {
    // Extend the dataset
    LOG4CXX_DEBUG(logger_, "Extending dataset_dimensions[0] = " << frame_no);
    dset.dataset_dimensions[0] = frame_no;
    ensureH5result(H5Dset_extent( dset.datasetid,
                            &dset.dataset_dimensions.front()), "H5Dset_extent failed to extend the dataset");
  }
}

/** Process an incoming frame.
 *
 * Checks we have been asked to write frames. If we are in writing mode
 * then the frame is checked for subframes. If subframes are found then
 * writeSubFrames is called. If no subframes are found then writeFrame
 * is called.
 * Finally counters are updated and if the number of required frames has
 * been reached then the stopWriting method is called.
 *
 * \param[in] frame - Pointer to the Frame object.
 */
void FileWriterPlugin::processFrame(boost::shared_ptr<Frame> frame)
{
  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);

  // Start a new file if the frame object contains a different acquisition ID from the current one
  checkAcquisitionID(frame);

  if (writing_) {
    if (checkFrameValid(frame)) {
      // Check if the frame has defined subframes
      if (frame->has_parameter("subframe_count")) {
        // The frame has subframes so write them out
        this->writeSubFrames(*frame);
      } else {
        // The frame has no subframes so write the whole frame
        this->writeFrame(*frame);
      }

      // Check if this is a master frame (for multi dataset acquisitions)
      // or if no master frame has been defined. If either of these conditions
      // are true then increment the number of frames written.
      if (currentAcquisition_.masterFrame_ == "" || currentAcquisition_.masterFrame_ == frame->get_dataset_name()) {
        size_t datasetFrames = this->getDatasetFrames(frame->get_dataset_name());
        if (datasetFrames == framesWritten_) {
          LOG4CXX_TRACE(logger_, "Frame " << datasetFrames << " rewritten");
        }
        else {
          framesWritten_ = datasetFrames;
        }
        LOG4CXX_TRACE(logger_, "Master frame processed");
      }
      else {
        LOG4CXX_TRACE(logger_, "Non-master frame processed");
      }

      framesProcessed_++;

      // Check if we have written enough frames and stop
      if (currentAcquisition_.framesToWrite_ > 0 && framesWritten_ == currentAcquisition_.framesToWrite_) {
        if (framesProcessed_ == currentAcquisition_.framesToWrite_) {
          stopAcquisition();
          // Prevent the timeout from closing the file as it's just been closed. This will also stop the timer if it's running
          timeoutActive = false;
        } else {
          LOG4CXX_INFO(logger_, "Starting close file timeout as number of frames processed (" <<
                  framesProcessed_ << ") doesn't match expected (" << currentAcquisition_.framesToWrite_ << ")");
          startCloseFileTimeout();
        }
      }

      // Push frame to any registered callbacks
      this->push(frame);
    }

    // Notify the close timeout thread that a frame has been processed
    boost::mutex::scoped_lock lock(m_closeFileMutex);
    m_timeoutCondition.notify_all();
  }
}

/** Check incoming frame is valid for its target dataset.
 *
 * Check the dimensions, data type and compression of the frame data.
 *
 * \param[in] frame - Pointer to the Frame object.
 * \return - true if the frame was valid
 */
bool FileWriterPlugin::checkFrameValid(boost::shared_ptr<Frame> frame)
{
  bool invalid = false;
  FileWriterPlugin::DatasetDefinition dataset = this->currentAcquisition_.dataset_defs_[frame->get_dataset_name()];
  if (frame->get_compression() >= 0 && frame->get_compression() != dataset.compression) {
    LOG4CXX_ERROR(logger_, "Frame has compression " << frame->get_compression() <<
                           ", expected " << dataset.compression <<
                           " for dataset " << dataset.name <<
                           " (0: None, 1: LZ4, 2: BSLZ4)");
    invalid = true;
  }
  if (frame->get_data_type() >= 0 && frame->get_data_type() != dataset.pixel) {
    LOG4CXX_ERROR(logger_, "Frame has data type " << frame->get_data_type() <<
                           ", expected " << dataset.pixel <<
                           " for dataset " << dataset.name <<
                           " (0: UINT8, 1: UINT16, 2: UINT32, 3: UINT64)");
    invalid = true;
  }
  if (frame->get_dimensions(frame->get_dataset_name()) != dataset.frame_dimensions) {
    std::vector<unsigned long long> dimensions = frame->get_dimensions(frame->get_dataset_name());
    LOG4CXX_ERROR(logger_, "Frame has dimensions [" << dimensions[0] << ", " << dimensions[1] <<
                           "], expected [" << dataset.frame_dimensions[0] << ", " << dataset.frame_dimensions[1] <<
                           "] for dataset " << dataset.name);
    invalid = true;
  }
  if (invalid) {
    LOG4CXX_ERROR(logger_, "Frame invalid. Stopping write");
    this->stopWriting();
  }
  return !invalid;
}

/** Read the current number of frames in a HDF5 dataset
 *
 * \param[in] dataset - HDF5 dataset
 */
size_t FileWriterPlugin::getDatasetFrames(const std::string dset_name)
{
  hid_t dspace = H5Dget_space(this->get_hdf5_dataset(dset_name).datasetid);
  const int ndims = H5Sget_simple_extent_ndims(dspace);
  hsize_t dims[ndims];
  H5Sget_simple_extent_dims(dspace, dims, NULL);

  return (size_t)dims[0];
}

/** Start writing frames to file.
 *
 * This method checks that the writer is not already writing. Then it creates
 * the datasets required (from their definitions) and creates the HDF5 file
 * ready to write frames. The framesWritten counter is reset to 0.
 */
void FileWriterPlugin::startWriting()
{
  // Set the current acquisition details to the ones held for the next acquisition and reset the next ones
  if (!writing_) {
    this->currentAcquisition_ = nextAcquisition_;
    this->nextAcquisition_ = Acquisition();

    // If filename hasn't been explicitly specified, create it from the acquisition ID and rank
    if (this->currentAcquisition_.fileName_.empty() && (!this->currentAcquisition_.acquisitionID_.empty())) {
      std::stringstream generatedFilename;
      generatedFilename << this->currentAcquisition_.acquisitionID_ << "_r" << this->concurrent_rank_ << ".hdf5";
      this->currentAcquisition_.fileName_ = generatedFilename.str();
    }

    if (this->currentAcquisition_.fileName_.empty()) {
      LOG4CXX_ERROR(logger_, "Unable to start writing - no filename to write to");
      return;
    }

    // Create the file
    boost::filesystem::path full_path =
        boost::filesystem::path(currentAcquisition_.filePath_) / boost::filesystem::path(currentAcquisition_.fileName_);
    this->createFile(full_path.string());

    // Create the datasets from the definitions
    std::map<std::string, FileWriterPlugin::DatasetDefinition>::iterator iter;
    for (iter = this->currentAcquisition_.dataset_defs_.begin(); iter != this->currentAcquisition_.dataset_defs_.end(); ++iter) {
      FileWriterPlugin::DatasetDefinition dset_def = iter->second;
      dset_def.num_frames = currentAcquisition_.framesToWrite_;
      this->createDataset(dset_def);
    }

#if H5_VERSION_GE(1,9,178)
    // Start SWMR writing
    ensureH5result(H5Fstart_swmr_write(this->hdf5_fileid_), "Failed to enable SWMR writing");
#endif

    // Reset counters
    framesWritten_ = 0;
    framesProcessed_ = 0;

    // Set writing flag to true
    writing_ = true;
  }
}

/** Stop writing frames to file.
 *
 * This method checks that the writer is currently writing. Then it closes
 * the file and stops writing frames.
 */
void FileWriterPlugin::stopWriting()
{
  if (writing_) {
    writing_ = false;
    this->closeFile();
  }
}

/**
 * Set configuration options for the file writer.
 *
 * This sets up the file writer plugin according to the configuration IpcMessage
 * objects that are received. The options are searched for:
 * CONFIG_PROCESS - Calls the method processConfig
 * CONFIG_FILE - Calls the method fileConfig
 * CONFIG_DATASET - Calls the method dsetConfig
 *
 * Checks to see if the number of frames to write has been set.
 * Checks to see if the writer should start or stop writing frames.
 *
 * \param[in] config - IpcMessage containing configuration data.
 * \param[out] reply - Response IpcMessage.
 */
void FileWriterPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);

  LOG4CXX_INFO(logger_, config.encode());

  // Check to see if we are configuring the process number and rank
  if (config.has_param(FileWriterPlugin::CONFIG_PROCESS)) {
    OdinData::IpcMessage processConfig(config.get_param<const rapidjson::Value&>(FileWriterPlugin::CONFIG_PROCESS));
    this->configureProcess(processConfig, reply);
  }

  // Check to see if we are configuring the file path and name
  if (config.has_param(FileWriterPlugin::CONFIG_FILE)) {
    OdinData::IpcMessage fileConfig(config.get_param<const rapidjson::Value&>(FileWriterPlugin::CONFIG_FILE));
    this->configureFile(fileConfig, reply);
  }

  // Check to see if we are configuring a dataset
  if (config.has_param(FileWriterPlugin::CONFIG_DATASET)) {
    OdinData::IpcMessage dsetConfig(config.get_param<const rapidjson::Value&>(FileWriterPlugin::CONFIG_DATASET));
    this->configureDataset(dsetConfig, reply);
  }

  // Check to see if we are being told how many frames to write
  if (config.has_param(FileWriterPlugin::CONFIG_FRAMES) && config.get_param<size_t>(FileWriterPlugin::CONFIG_FRAMES) > 0) {
    size_t totalFrames = config.get_param<size_t>(FileWriterPlugin::CONFIG_FRAMES);
    nextAcquisition_.totalFrames_ = totalFrames;
    nextAcquisition_.framesToWrite_ = calcNumFrames(totalFrames);

    LOG4CXX_INFO(logger_, "Expecting " << nextAcquisition_.framesToWrite_ << " frames "
                           "(total " << totalFrames << ")");
  }

  // Check to see if the master dataset is being set
  if (config.has_param(FileWriterPlugin::CONFIG_MASTER_DATASET)) {
    nextAcquisition_.masterFrame_ = config.get_param<std::string>(FileWriterPlugin::CONFIG_MASTER_DATASET);
  }

  // Check if we are setting the frame offset adjustment
  if (config.has_param(FileWriterPlugin::CONFIG_OFFSET_ADJUSTMENT)) {
    LOG4CXX_INFO(logger_, "Setting frame offset adjustment to "
                           << config.get_param<int>(FileWriterPlugin::CONFIG_OFFSET_ADJUSTMENT));
    frame_offset_adjustment_ = (size_t)config.get_param<int>(FileWriterPlugin::CONFIG_OFFSET_ADJUSTMENT);
  }

  // Check to see if the acquisition id is being set
  if (config.has_param(FileWriterPlugin::ACQUISITION_ID)) {
    nextAcquisition_.acquisitionID_ = config.get_param<std::string>(FileWriterPlugin::ACQUISITION_ID);
	LOG4CXX_INFO(logger_, "Setting next Acquisition ID to " << nextAcquisition_.acquisitionID_);
  }

  // Check to see if the close file timeout period is being set
  if (config.has_param(FileWriterPlugin::CLOSE_TIMEOUT_PERIOD)) {
    timeoutPeriod = config.get_param<size_t>(FileWriterPlugin::CLOSE_TIMEOUT_PERIOD);
    LOG4CXX_INFO(logger_, "Setting close file timeout to " << timeoutPeriod);
  }

  // Check to see if the close file timeout is being started
  if (config.has_param(FileWriterPlugin::START_CLOSE_TIMEOUT)) {
    if (config.get_param<bool>(FileWriterPlugin::START_CLOSE_TIMEOUT) == true) {
      LOG4CXX_INFO(logger_, "Configure call to start close file timeout");
      startCloseFileTimeout();
    }
  }

  // Final check is to start or stop writing
  if (config.has_param(FileWriterPlugin::CONFIG_WRITE)) {
    if (config.get_param<bool>(FileWriterPlugin::CONFIG_WRITE) == true) {
      // Only start writing if we have frames to write, or if the total number of frames is 0 (free running mode)
      if (nextAcquisition_.totalFrames_ > 0 && nextAcquisition_.framesToWrite_ == 0) {
        // We're not expecting any frames, so just clear out the nextAcquisition for the next one and don't start writing
        this->nextAcquisition_ = Acquisition();
        this->currentAcquisition_ = Acquisition();
        framesWritten_ = 0;
        LOG4CXX_INFO(logger_, "FrameProcessor will not receive any frames from this acquisition and so no output file will be created");
      } else {
        this->startWriting();
      }
    } else {
      this->stopWriting();
    }
  }
}

/**
 * Set configuration options for the file writer process count.
 *
 * This sets up the file writer plugin according to the configuration IpcMessage
 * objects that are received. The options are searched for:
 * CONFIG_PROCESS_NUMBER - Sets the number of writer processes executing
 * CONFIG_PROCESS_RANK - Sets the rank of this process
 *
 * The configuration is not applied if the writer is currently writing.
 *
 * \param[in] config - IpcMessage containing configuration data.
 * \param[out] reply - Response IpcMessage.
 */
void FileWriterPlugin::configureProcess(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  // Check for process number
  if (config.has_param(FileWriterPlugin::CONFIG_PROCESS_NUMBER)) {
    size_t processes = config.get_param<size_t>(FileWriterPlugin::CONFIG_PROCESS_NUMBER);
    if (this->concurrent_processes_ != processes) {
      // If we are writing a file then we cannot change concurrent processes
      if (this->writing_) {
        LOG4CXX_ERROR(logger_, "Cannot change concurrent processes whilst writing");
        throw std::runtime_error("Cannot change concurrent processes whilst writing");
      }
      this->concurrent_processes_ = processes;
      LOG4CXX_DEBUG(logger_, "Concurrent processes changed to " << this->concurrent_processes_);
    }
    else {
      LOG4CXX_DEBUG(logger_, "Concurrent processes is already " << this->concurrent_processes_);
    }
  }
  // Check for rank number
  if (config.has_param(FileWriterPlugin::CONFIG_PROCESS_RANK)) {
    size_t rank = config.get_param<size_t>(FileWriterPlugin::CONFIG_PROCESS_RANK);
    if (this->concurrent_rank_ != rank) {
      // If we are writing a file then we cannot change concurrent rank
      if (this->writing_) {
        LOG4CXX_ERROR(logger_, "Cannot change process rank whilst writing");
        throw std::runtime_error("Cannot change process rank whilst writing");
      }
      this->concurrent_rank_ = rank;
      LOG4CXX_DEBUG(logger_, "Process rank changed to " << this->concurrent_rank_);
    }
    else {
      LOG4CXX_DEBUG(logger_, "Process rank is already " << this->concurrent_rank_);
    }
  }

  // Check to see if the frames per block is being set
  if (config.has_param(FileWriterPlugin::CONFIG_PROCESS_BLOCKSIZE)) {
    size_t block_size = config.get_param<size_t>(FileWriterPlugin::CONFIG_PROCESS_BLOCKSIZE);
    if (this->frames_per_block_ != block_size) {
      // If we are writing a file then we cannot change block size
      if (this->writing_) {
        LOG4CXX_ERROR(logger_, "Cannot change block size whilst writing");
        throw std::runtime_error("Cannot change block size whilst writing");
      }
      this->frames_per_block_ = block_size;
      LOG4CXX_INFO(logger_, "Setting number of frames per block to " << frames_per_block_);
    }
    else {
      LOG4CXX_DEBUG(logger_, "Block size is already " << this->frames_per_block_);
    }
  }
}

/**
 * Set file configuration options for the file writer.
 *
 * This sets up the file writer plugin according to the configuration IpcMessage
 * objects that are received. The options are searched for:
 * CONFIG_FILE_PATH - Sets the path of the file to write to
 * CONFIG_FILE_NAME - Sets the filename of the file to write to
 *
 * The configuration is not applied if the writer is currently writing.
 *
 * \param[in] config - IpcMessage containing configuration data.
 * \param[out] reply - Response IpcMessage.
 */
void FileWriterPlugin::configureFile(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  LOG4CXX_DEBUG(logger_, "Configure file name and path");
  // Check for file path and file name
  if (config.has_param(FileWriterPlugin::CONFIG_FILE_PATH)) {
    this->nextAcquisition_.filePath_ = config.get_param<std::string>(FileWriterPlugin::CONFIG_FILE_PATH);
    LOG4CXX_DEBUG(logger_, "Next file path changed to " << this->nextAcquisition_.filePath_);
  }
  if (config.has_param(FileWriterPlugin::CONFIG_FILE_NAME)) {
    this->nextAcquisition_.fileName_ = config.get_param<std::string>(FileWriterPlugin::CONFIG_FILE_NAME);
    LOG4CXX_DEBUG(logger_, "Next file name changed to " << this->nextAcquisition_.fileName_);
  }
}

/**
 * Set dataset configuration options for the file writer.
 *
 * This sets up the file writer plugin according to the configuration IpcMessage
 * objects that are received. The options are searched for:
 * CONFIG_DATASET_CMD - Should we create/delete a dataset definition
 * CONFIG_DATASET_NAME - Name of the dataset
 * CONFIG_DATASET_TYPE - Datatype of the dataset
 * CONFIG_DATASET_DIMS - Dimensions of the dataset
 * CONFIG_DATASET_CHUNKS - Chunking parameters of the dataset
 * CONFIG_DATASET_COMPRESSION - Compression of raw data
 *
 * The configuration is not applied if the writer is currently writing.
 *
 * \param[in] config - IpcMessage containing configuration data.
 * \param[out] reply - Response IpcMessage.
 */
void FileWriterPlugin::configureDataset(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  LOG4CXX_DEBUG(logger_, "Configure dataset");
  // Read the dataset command
  if (config.has_param(FileWriterPlugin::CONFIG_DATASET_CMD)) {
    std::string cmd = config.get_param<std::string>(FileWriterPlugin::CONFIG_DATASET_CMD);

    // Command for creating a new dataset description
    if (cmd == "create") {
      FileWriterPlugin::DatasetDefinition dset_def;
      // There must be a name present for the dataset
      if (config.has_param(FileWriterPlugin::CONFIG_DATASET_NAME)) {
        dset_def.name = config.get_param<std::string>(FileWriterPlugin::CONFIG_DATASET_NAME);
      } else {
        LOG4CXX_ERROR(logger_, "Cannot create a dataset without a name");
        throw std::runtime_error("Cannot create a dataset without a name");
      }

      // There must be a type present for the dataset
      if (config.has_param(FileWriterPlugin::CONFIG_DATASET_TYPE)) {
        dset_def.pixel = (FileWriterPlugin::PixelType)config.get_param<int>(FileWriterPlugin::CONFIG_DATASET_TYPE);
      } else {
        LOG4CXX_ERROR(logger_, "Cannot create a dataset without a data type");
        throw std::runtime_error("Cannot create a dataset without a data type");
      }

      // There may be dimensions present for the dataset
      if (config.has_param(FileWriterPlugin::CONFIG_DATASET_DIMS)) {
        const rapidjson::Value& val = config.get_param<const rapidjson::Value&>(FileWriterPlugin::CONFIG_DATASET_DIMS);
        // Loop over the dimension values
        dimensions_t dims(val.Size());
        for (rapidjson::SizeType i = 0; i < val.Size(); i++) {
          const rapidjson::Value& dim = val[i];
          dims[i] = dim.GetUint64();
        }
        dset_def.frame_dimensions = dims;
      } else {
        // This is a single dimensioned dataset so store dimensions as NULL
        dimensions_t dims(0);
        dset_def.frame_dimensions = dims;
      }

      // There might be chunking dimensions present for the dataset, this is not required
      if (config.has_param(FileWriterPlugin::CONFIG_DATASET_CHUNKS)) {
        const rapidjson::Value& val = config.get_param<const rapidjson::Value&>(FileWriterPlugin::CONFIG_DATASET_CHUNKS);
        // Loop over the dimension values
        dimensions_t chunks(val.Size());
        for (rapidjson::SizeType i = 0; i < val.Size(); i++) {
          const rapidjson::Value& dim = val[i];
          chunks[i] = dim.GetUint64();
        }
        dset_def.chunks = chunks;
      } else {
        // No chunks were specified, creating defaults from the dataset dimensions
        // Chunk number of dimensions will be 1 greater than dataset (to include n dimension)
        dimensions_t chunks(dset_def.frame_dimensions.size()+1);
        // Set first chunk dimension (n dimension) to a single frame or item
        chunks[0] = 1;
        // Set the remaining chunk dimensions to the same as the dataset dimensions
        for (int index = 0; index < dset_def.frame_dimensions.size(); index++){
          chunks[index+1] = dset_def.frame_dimensions[index];
        }
        dset_def.chunks = chunks;
      }

      // Check if compression has been specified for the raw data
      if (config.has_param(FileWriterPlugin::CONFIG_DATASET_COMPRESSION)) {
        dset_def.compression = (FileWriterPlugin::CompressionType)config.get_param<int>(FileWriterPlugin::CONFIG_DATASET_COMPRESSION);
        LOG4CXX_INFO(logger_, "Enabling compression: " << dset_def.compression);
      }
      else {
        dset_def.compression = no_compression;
      }

      if (dset_def.frame_dimensions.size() > 0){
          LOG4CXX_DEBUG(logger_, "Creating dataset [" << dset_def.name << "] (" << dset_def.frame_dimensions[0] << ", " << dset_def.frame_dimensions[1] << ")");
      } else {
          LOG4CXX_DEBUG(logger_, "Creating 1D dataset [" << dset_def.name << "] with no additional dimensions");
      }
      // Add the dataset definition to the store
      this->nextAcquisition_.dataset_defs_[dset_def.name] = dset_def;
    }
  }
}

/**
 * Collate status information for the plugin. The status is added to the status IpcMessage object.
 *
 * \param[out] status - Reference to an IpcMessage value to store the status.
 */
void FileWriterPlugin::status(OdinData::IpcMessage& status)
{
  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);

  // Record the plugin's status items
  LOG4CXX_DEBUG(logger_, "File name " << this->currentAcquisition_.fileName_);

  status.set_param(getName() + "/writing", this->writing_);
  status.set_param(getName() + "/frames_max", (int)this->currentAcquisition_.framesToWrite_);
  status.set_param(getName() + "/frames_written", (int)this->framesWritten_);
  status.set_param(getName() + "/file_path", this->currentAcquisition_.filePath_);
  status.set_param(getName() + "/file_name", this->currentAcquisition_.fileName_);
  status.set_param(getName() + "/acquisition_id", this->currentAcquisition_.acquisitionID_);
  status.set_param(getName() + "/processes", (int)this->concurrent_processes_);
  status.set_param(getName() + "/rank", (int)this->concurrent_rank_);

  // Check for datasets
  std::map<std::string, FileWriterPlugin::DatasetDefinition>::iterator iter;
  for (iter = this->currentAcquisition_.dataset_defs_.begin(); iter != this->currentAcquisition_.dataset_defs_.end(); ++iter) {
    // Add the dataset type
    status.set_param(getName() + "/datasets/" + iter->first + "/type", (int)iter->second.pixel);

    // Check for and add dimensions
    if (iter->second.frame_dimensions.size() > 0) {
      std::string dimParamName = getName() + "/datasets/" + iter->first + "/dimensions[]";
      for (int index = 0; index < iter->second.frame_dimensions.size(); index++) {
        status.set_param(dimParamName, (int)iter->second.frame_dimensions[index]);
      }
    }
    // Check for and add chunking dimensions
    if (iter->second.chunks.size() > 0) {
      std::string chunkParamName = getName() + "/datasets/" + iter->first + "/chunks[]";
      for (int index = 0; index < iter->second.chunks.size(); index++) {
        status.set_param(chunkParamName, (int)iter->second.chunks[index]);
      }
    }
  }
}

void FileWriterPlugin::hdfErrorHandler(unsigned n, const H5E_error2_t *err_desc)
{
  const int MSG_SIZE = 64;
  char maj[MSG_SIZE];
  char min[MSG_SIZE];
  char cls[MSG_SIZE];

  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);
  printf("In here!!!\n");
  // Set the error flag true
  hdf5ErrorFlag_ = true;

  // Get descriptions for the major and minor error numbers
  H5Eget_class_name(err_desc->cls_id, cls, MSG_SIZE);
  H5Eget_msg(err_desc->maj_num, NULL, maj, MSG_SIZE);
  H5Eget_msg(err_desc->min_num, NULL, min, MSG_SIZE);

  // Record the error into the error stack
  std::stringstream err;
  err << "[" << cls << "] " << maj << " (" << min << ")";
  hdf5Errors_.push_back(err.str());
}

bool FileWriterPlugin::checkForHdfErrors()
{
  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);

  // Simply return the current error flag state
  return hdf5ErrorFlag_;
}

std::vector<std::string> FileWriterPlugin::readHdfErrors()
{
  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);

  // Simply return the current error array
  return hdf5Errors_;
}

void FileWriterPlugin::clearHdfErrors()
{
  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);

  // Empty the error array
  hdf5Errors_.clear();
  // Now reset the error flag
  hdf5ErrorFlag_ = false;
}

/** Check if the frame contains an acquisition ID and start a new file if it does and it's different from the current one
 *
 * If the frame object contains an acquisition ID, then we need to check if the current acquisition we are writing has
 * the same ID. If it is different, then we close the current file and create a new one and start writing.
 * If we are not currently writing then we just create a new file and start writing.
 *
 * \param[in] frame - Pointer to the Frame object.
 */
void FileWriterPlugin::checkAcquisitionID(boost::shared_ptr<Frame> frame) {
  if (!frame->get_acquisition_id().empty()) {
    if (writing_) {
      if (frame->get_acquisition_id() == currentAcquisition_.acquisitionID_) {
        // On same file, take no action
    	return;
      }
    }

    if (frame->get_acquisition_id() == nextAcquisition_.acquisitionID_) {
      LOG4CXX_DEBUG(logger_, "Acquisition ID sent in frame matches next acquisition ID. Closing current file and starting next");
      stopWriting();
      startWriting();
    } else {
      LOG4CXX_WARN(logger_, "Unexpected acquisition ID on frame [" << frame->get_acquisition_id() << "] for frame " << frame->get_frame_number());
      // TODO set status? (There's currently no mechanism to report this in the status message)
    }
  }
}

/** Creates and returns the Meta Header json string to be sent out over the meta data channel when a file is created
 *
 * This will typically include details about the current acquisition (e.g. the ID)
 *
 * \return - a string containing the json meta data header
 */
std::string FileWriterPlugin::getCreateMetaHeader() {
  rapidjson::Document metaDocument;
  metaDocument.SetObject();

  // Add Acquisition ID
  rapidjson::Value keyAcqID("acqID", metaDocument.GetAllocator());
  rapidjson::Value valueAcqID;
  valueAcqID.SetString(currentAcquisition_.acquisitionID_.c_str(), currentAcquisition_.acquisitionID_.length(), metaDocument.GetAllocator());
  metaDocument.AddMember(keyAcqID, valueAcqID, metaDocument.GetAllocator());

  // Add Number of Frames
  rapidjson::Value keyNumFrames("totalFrames", metaDocument.GetAllocator());
  rapidjson::Value valueNumFrames;
  valueNumFrames.SetUint64(currentAcquisition_.totalFrames_);
  metaDocument.AddMember(keyNumFrames, valueNumFrames, metaDocument.GetAllocator());

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  metaDocument.Accept(writer);

  return buffer.GetString();
}

/** Creates and returns the Meta Header json string to be sent out over the meta data channel
 *
 * This will typically include details about the current acquisition (e.g. the ID)
 *
 * \return - a string containing the json meta data header
 */
std::string FileWriterPlugin::getMetaHeader() {
  rapidjson::Document metaDocument;
  metaDocument.SetObject();

  // Add Acquisition ID
  rapidjson::Value keyAcqID("acqID", metaDocument.GetAllocator());
  rapidjson::Value valueAcqID;
  valueAcqID.SetString(currentAcquisition_.acquisitionID_.c_str(), currentAcquisition_.acquisitionID_.length(), metaDocument.GetAllocator());
  metaDocument.AddMember(keyAcqID, valueAcqID, metaDocument.GetAllocator());

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  metaDocument.Accept(writer);

  return buffer.GetString();
}

/**
 * Handles an HDF5 error. Logs the error and throws a runtime exception
 *
 * \param[in] message - The error message to log
 * \param[in] function - The function that had the error
 * \param[in] filename - The filename
 * \param[in] line - The line number of the call
 */
void FileWriterPlugin::handleH5error(std::string message, std::string function, std::string filename, int line) const {
  std::stringstream err;
  err << "H5 function error: (" << message << ") in " << filename << ":" << line << ": " << function;
  LOG4CXX_ERROR(logger_, err.str());
  throw std::runtime_error(err.str().c_str());
}

/**
 * Stops the current acquisition and starts the next if it is configured
 *
 */
void FileWriterPlugin::stopAcquisition() {
  this->stopWriting();
  // Start next acquisition if we have a filename or acquisition ID to use
  if (!nextAcquisition_.fileName_.empty() || !nextAcquisition_.acquisitionID_.empty()) {
    if (nextAcquisition_.totalFrames_ > 0 && nextAcquisition_.framesToWrite_ == 0) {
      // We're not expecting any frames, so just clear out the nextAcquisition for the next one and don't start writing
      this->nextAcquisition_ = Acquisition();
      LOG4CXX_INFO(logger_, "FrameProcessor will not receive any frames from this acquisition and so no output file will be created");
    } else {
      this->startWriting();
    }
  }
}

/**
 * Starts the close file timeout
 *
 */
void FileWriterPlugin::startCloseFileTimeout()
{
  if (timeoutActive == false) {
    LOG4CXX_DEBUG(logger_, "Starting close file timeout");
    boost::mutex::scoped_lock lock(m_startTimeoutMutex);
    m_startCondition.notify_all();
  } else {
	  LOG4CXX_DEBUG(logger_, "Close file timeout already active");
  }
}

/**
 * Function that is run by the close file timeout thread
 *
 * This waits until notified to start, and then runs a timer. If the timer times out,
 * then the current acquisition is stopped. If the timer is notified before timing out
 * (by a frame being written) then no action is taken, as it will either start the timer
 * again or go back to the wait for start state, depending on the value of timeoutActive.
 *
 */
void FileWriterPlugin::runCloseFileTimeout()
{
  boost::mutex::scoped_lock startLock(m_startTimeoutMutex);
  while (timeoutThreadRunning) {
    m_startCondition.wait(startLock);
    if (timeoutThreadRunning) {
      timeoutActive = true;
      boost::mutex::scoped_lock lock(m_closeFileMutex);
      while (timeoutActive) {
        if (!m_timeoutCondition.timed_wait(lock, boost::posix_time::milliseconds(timeoutPeriod))) {
          // Timeout
          LOG4CXX_DEBUG(logger_, "Close file Timeout timed out");
          boost::lock_guard<boost::recursive_mutex> lock(mutex_);
          if (writing_ && timeoutActive) {
            LOG4CXX_INFO(logger_, "Timed out waiting for frames, stopping writing");
            stopAcquisition();
          }
          timeoutActive = false;
        } else {
          // Event - No need to do anything. The timeout will either wait for another period if active or wait until it is restarted
        }
      }
    }
  }
}

/**
 * Calculates the number of frames that this FileWriter can expect to write based on the total number of frames
 *
 * \param[in] totalFrames - The total number of frames in the acquisition
 * \return - The number of frames that this FileWriter is expected to write
 */
size_t FileWriterPlugin::calcNumFrames(size_t totalFrames)
{
  size_t num_of_frames = 0;

  // Work out how many 'rounds' where all processes are writing whole blocks
  size_t blocks_needed = totalFrames / frames_per_block_;
  size_t num_whole_rounds_needed = blocks_needed / this->concurrent_processes_;
  num_of_frames = num_whole_rounds_needed * frames_per_block_;

  // Now work out if there are any left over half-complete rounds
  size_t leftover = totalFrames - (num_of_frames * this->concurrent_processes_);

  size_t remaining = 0;

  // If there is a leftover, and this processor gets any of the remaining frames, add this to the total
  if (leftover > (this->concurrent_rank_ * frames_per_block_))
  {
  	remaining = leftover - (this->concurrent_rank_ * frames_per_block_);
    if (remaining > 0) {
      num_of_frames += std::min(remaining, frames_per_block_);
    }
  }

  return num_of_frames;
}

} /* namespace FrameProcessor */

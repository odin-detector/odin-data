/*
 * FileWriter.cpp
 *
 */

#include <assert.h>

#include <hdf5_hl.h>

#include "Frame.h"
#include "FileWriterPlugin.h"

namespace FrameProcessor
{

const std::string FileWriterPlugin::CONFIG_PROCESS             = "process";
const std::string FileWriterPlugin::CONFIG_PROCESS_NUMBER      = "number";
const std::string FileWriterPlugin::CONFIG_PROCESS_RANK        = "rank";

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
    concurrent_processes_(1),
    concurrent_rank_(0),
    hdf5_fileid_(0),
    hdf5ErrorFlag_(false),
    frame_offset_adjustment_(0)
{
  this->logger_ = Logger::getLogger("FW.FileWriterPlugin");
  this->logger_->setLevel(Level::getTrace());
  LOG4CXX_TRACE(logger_, "FileWriterPlugin constructor.");

  H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
  H5Ewalk2(H5E_DEFAULT, H5E_WALK_DOWNWARD, hdf5_error_cb, this);
  //H5Eset_auto2(H5E_DEFAULT, my_hdf5_error_handler, NULL);
}

/**
 * Destructor.
 */
FileWriterPlugin::~FileWriterPlugin()
{
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
  assert(fapl >= 0);

  assert(H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG) >= 0);

  // Set chunk boundary alignment to 4MB
  assert( H5Pset_alignment( fapl, 65536, 4*1024*1024 ) >= 0);

  // Set to use the latest library format
  assert(H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST) >= 0);

  // Create file creation property list
  if ((fcpl = H5Pcreate(H5P_FILE_CREATE)) < 0)
    assert(fcpl >= 0);

  // Creating the file with SWMR write access
  LOG4CXX_INFO(logger_, "Creating file: " << filename);
  unsigned int flags = H5F_ACC_TRUNC;
  this->hdf5_fileid_ = H5Fcreate(filename.c_str(), flags, fcpl, fapl);
  if (this->hdf5_fileid_ < 0) {
    // Close file access property list
    assert(H5Pclose(fapl) >= 0);
    // Now throw a runtime error to explain that the file could not be created
    std::stringstream err;
    err << "Could not create file " << filename;
    throw std::runtime_error(err.str().c_str());
  }
  // Close file access property list
  assert(H5Pclose(fapl) >= 0);

  // Send meta data message to notify of file creation
  publishMeta("createfile", filename, getCreateMetaHeader());
}

/**
 * Write a frame to the file.
 *
 * \param[in] frame - Reference to the frame.
 */
void FileWriterPlugin::writeFrame(const Frame& frame) {
  herr_t status;
  hsize_t frame_no = frame.get_frame_number();

  HDF5Dataset_t& dset = this->get_hdf5_dataset(frame.get_dataset_name());

  hsize_t frame_offset = 0;
  frame_offset = this->getFrameOffset(frame_no);
  this->extend_dataset(dset, frame_offset + 1);

  LOG4CXX_DEBUG(logger_, "Writing frame offset=" << frame_no  <<
                         " (" << frame_offset << ")" <<
                         " dset=" << frame.get_dataset_name());

  // Set the offset
  std::vector<hsize_t>offset(dset.dataset_dimensions.size());
  offset[0] = frame_offset;

  uint32_t filter_mask = 0x0;
  status = H5DOwrite_chunk(dset.datasetid, H5P_DEFAULT,
                           filter_mask, &offset.front(),
                           frame.get_data_size(), frame.get_data());
  if (status < 0) {
    throw RuntimeException("Failed to write chunk");
  }

  status = H5Dflush(dset.datasetid);
  if (status < 0) {
    throw RuntimeException("Failed to flush data to disk");
  }

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
  herr_t status;
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

    status = H5DOwrite_chunk(dset.datasetid, H5P_DEFAULT,
                             filter_mask, &offset.front(),
                             frame.get_parameter("subframe_size"),
                             (static_cast<const char*>(frame.get_data())+(i*frame.get_parameter("subframe_size"))));
    assert(status >= 0);
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
  herr_t status;
  hid_t dtype = pixelToHdfType(definition.pixel);

  std::vector<hsize_t> frame_dims = definition.frame_dimensions;

  // Dataset dims: {1, <image size Y>, <image size X>}
  std::vector<hsize_t> dset_dims(1,1);
  dset_dims.insert(dset_dims.end(), frame_dims.begin(), frame_dims.end());

  // If chunking has not been defined it defaults to a single full frame
  std::vector<hsize_t> chunk_dims(1, 1);
  if (definition.chunks.size() != dset_dims.size()) {
    chunk_dims = dset_dims;
  } else {
    chunk_dims = definition.chunks;
  }

  std::vector<hsize_t> max_dims = dset_dims;
  max_dims[0] = H5S_UNLIMITED;

  /* Create the dataspace with the given dimensions - and max dimensions */
  dataspace = H5Screate_simple(dset_dims.size(), &dset_dims.front(), &max_dims.front());
  assert(dataspace >= 0);

  /* Enable chunking  */
  LOG4CXX_DEBUG(logger_, "Chunking=" << chunk_dims[0] << ","
                                     << chunk_dims[1] << ","
                                     << chunk_dims[2]);
  prop = H5Pcreate(H5P_DATASET_CREATE);

  /* Enable defined compression mode */
  if (definition.compression == no_compression) {
    LOG4CXX_DEBUG(logger_, "Compression type: None");
  }
  else if (definition.compression == lz4){
    LOG4CXX_DEBUG(logger_, "Compression type: LZ4");
    // Create cd_values for filter to set the LZ4 compression level
    unsigned int cd_values = 3;
    size_t cd_values_length = 1;
    status = H5Pset_filter(prop, LZ4_FILTER, H5Z_FLAG_MANDATORY,
                           cd_values_length, &cd_values);
    assert(status >= 0);
  }
  else if (definition.compression == bslz4) {
    LOG4CXX_DEBUG(logger_, "Compression type: BSLZ4");
    // Create cd_values for filter to set default block size and to enable LZ4
    unsigned int cd_values[2] = {0, 2};
    size_t cd_values_length = 2;
    status = H5Pset_filter(prop, BSLZ4_FILTER, H5Z_FLAG_MANDATORY,
                           cd_values_length, cd_values);
    assert(status >= 0);
  }

  status = H5Pset_chunk(prop, dset_dims.size(), &chunk_dims.front());
  assert(status >= 0);

  char fill_value[8] = {0,0,0,0,0,0,0,0};
  status = H5Pset_fill_value(prop, dtype, fill_value);
  assert(status >= 0);

  dapl = H5Pcreate(H5P_DATASET_ACCESS);

  /* Create dataset  */
  LOG4CXX_DEBUG(logger_, "Creating dataset: " << definition.name);
  FileWriterPlugin::HDF5Dataset_t dset;
  dset.datasetid = H5Dcreate2(this->hdf5_fileid_, definition.name.c_str(),
                              dtype, dataspace,
                              H5P_DEFAULT, prop, dapl);
  if (dset.datasetid < 0) {
    // Unable to create the dataset, clean up resources
    assert( H5Pclose(prop) >= 0);
    assert( H5Pclose(dapl) >= 0);
    assert( H5Sclose(dataspace) >= 0);
    // Now throw a runtime error to notify that the dataset could not be created
    throw std::runtime_error("Unable to create the dataset");
  }
  dset.dataset_dimensions = dset_dims;
  dset.dataset_offsets = std::vector<hsize_t>(3);
  this->hdf5_datasets_[definition.name] = dset;

  LOG4CXX_DEBUG(logger_, "Closing intermediate open HDF objects");
  assert( H5Pclose(prop) >= 0);
  assert( H5Pclose(dapl) >= 0);
  assert( H5Sclose(dataspace) >= 0);
}

/**
 * Close the currently open HDF5 file.
 */
void FileWriterPlugin::closeFile() {
  LOG4CXX_TRACE(logger_, "Closing file " << this->currentAcquisition_.filePath_ << "/" << this->currentAcquisition_.fileName_);
  if (this->hdf5_fileid_ >= 0) {
    assert(H5Fclose(this->hdf5_fileid_) >= 0);
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
    // Note: this expects the frame numbering from HW/FW to start at 1, not 0!
    if ( (((frame_no-1) % this->concurrent_processes_) - this->concurrent_rank_) != 0) {
      LOG4CXX_WARN(logger_, "Unexpected frame: " << frame_no <<
                                                 " in this process rank: " << this->concurrent_rank_);
      throw std::runtime_error("Unexpected frame in this process rank");
    }

    // Calculate the new offset based on how many concurrent processes are running
    frame_offset = frame_offset / this->concurrent_processes_;
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
  herr_t status;
  if (frame_no > dset.dataset_dimensions[0]) {
    // Extend the dataset
    LOG4CXX_DEBUG(logger_, "Extending dataset_dimensions[0] = " << frame_no);
    dset.dataset_dimensions[0] = frame_no;
    status = H5Dset_extent( dset.datasetid,
                            &dset.dataset_dimensions.front());
    assert(status >= 0);
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
    checkFrameValid(frame);
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
        LOG4CXX_DEBUG(logger_, "Frame " << datasetFrames << " rewritten");
      }
      else {
        framesWritten_ = datasetFrames;
      }
      LOG4CXX_DEBUG(logger_, "Master frame processed");
    }
    else {
      LOG4CXX_DEBUG(logger_, "Non-master frame processed");
    }

    // Check if we have written enough frames and stop
    if (currentAcquisition_.framesToWrite_ > 0 && framesWritten_ == currentAcquisition_.framesToWrite_) {
      this->stopWriting();
      // Start next acquisition if we have a filename or acquisition ID to use
      if (!nextAcquisition_.fileName_.empty() || !nextAcquisition_.acquisitionID_.empty()) {
        this->startWriting();
      }
    }

    // Push frame to any registered callbacks
    this->push(frame);
  }
}

/** Check incoming frame is valid for its target dataset.
 *
 * Check the dimensions, data type and compression of the frame data.
 *
 * \param[in] frame - Pointer to the Frame object.
 */
void FileWriterPlugin::checkFrameValid(boost::shared_ptr<Frame> frame)
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
    throw std::runtime_error("Got invalid frame");
  }
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
    this->createFile(currentAcquisition_.filePath_ + currentAcquisition_.fileName_);

    // Create the datasets from the definitions
    std::map<std::string, FileWriterPlugin::DatasetDefinition>::iterator iter;
    for (iter = this->currentAcquisition_.dataset_defs_.begin(); iter != this->currentAcquisition_.dataset_defs_.end(); ++iter) {
      FileWriterPlugin::DatasetDefinition dset_def = iter->second;
      dset_def.num_frames = currentAcquisition_.framesToWrite_;
      this->createDataset(dset_def);
    }

    // Start SWMR writing
    herr_t status = H5Fstart_swmr_write(this->hdf5_fileid_);
    if (status < 0) {
      throw RuntimeException("Failed to enable SWMR writing");
    }

    // Reset counters
    framesWritten_ = 0;

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

  LOG4CXX_DEBUG(logger_, config.encode());

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
    nextAcquisition_.framesToWrite_ = totalFrames / this->concurrent_processes_;
    if (totalFrames % this->concurrent_processes_ > this->concurrent_rank_) {
      nextAcquisition_.framesToWrite_++;
    }
    LOG4CXX_DEBUG(logger_, "Expecting " << nextAcquisition_.framesToWrite_ << " frames "
                           "(total " << totalFrames << ")");
  }

  // Check to see if the master dataset is being set
  if (config.has_param(FileWriterPlugin::CONFIG_MASTER_DATASET)) {
    nextAcquisition_.masterFrame_ = config.get_param<std::string>(FileWriterPlugin::CONFIG_MASTER_DATASET);
  }

  // Check if we are setting the frame offset adjustment
  if (config.has_param(FileWriterPlugin::CONFIG_OFFSET_ADJUSTMENT)) {
    LOG4CXX_DEBUG(logger_, "Setting frame offset adjustment to "
                           << config.get_param<int>(FileWriterPlugin::CONFIG_OFFSET_ADJUSTMENT));
    frame_offset_adjustment_ = (size_t)config.get_param<int>(FileWriterPlugin::CONFIG_OFFSET_ADJUSTMENT);
  }

  // Check to see if the acquisition id is being set
  if (config.has_param(FileWriterPlugin::ACQUISITION_ID)) {
    nextAcquisition_.acquisitionID_ = config.get_param<std::string>(FileWriterPlugin::ACQUISITION_ID);
	LOG4CXX_DEBUG(logger_, "Setting next Acquisition ID to " << nextAcquisition_.acquisitionID_);
  }

  // Final check is to start or stop writing
  if (config.has_param(FileWriterPlugin::CONFIG_WRITE)) {
    if (config.get_param<bool>(FileWriterPlugin::CONFIG_WRITE) == true) {
      this->startWriting();
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
  // If we are writing a file then we cannot change these items
  if (this->writing_) {
    LOG4CXX_ERROR(logger_, "Cannot change concurrent processes or rank whilst writing");
    throw std::runtime_error("Cannot change concurrent processes or rank whilst writing");
  }

  // Check for process number and rank number
  if (config.has_param(FileWriterPlugin::CONFIG_PROCESS_NUMBER)) {
    this->concurrent_processes_ = config.get_param<size_t>(FileWriterPlugin::CONFIG_PROCESS_NUMBER);
    LOG4CXX_DEBUG(logger_, "Concurrent processes changed to " << this->concurrent_processes_);
  }
  if (config.has_param(FileWriterPlugin::CONFIG_PROCESS_RANK)) {
    this->concurrent_rank_ = config.get_param<size_t>(FileWriterPlugin::CONFIG_PROCESS_RANK);
    LOG4CXX_DEBUG(logger_, "Process rank changed to " << this->concurrent_rank_);
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

      // There must be dimensions present for the dataset
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
        LOG4CXX_ERROR(logger_, "Cannot create a dataset without dimensions");
        throw std::runtime_error("Cannot create a dataset without dimensions");
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
      }

      // Check if compression has been specified for the raw data
      if (config.has_param(FileWriterPlugin::CONFIG_DATASET_COMPRESSION)) {
        dset_def.compression = (FileWriterPlugin::CompressionType)config.get_param<int>(FileWriterPlugin::CONFIG_DATASET_COMPRESSION);
        LOG4CXX_DEBUG(logger_, "Enabling compression: " << dset_def.compression);
      }
      else {
        dset_def.compression = no_compression;
      }

      LOG4CXX_DEBUG(logger_, "Creating dataset [" << dset_def.name << "] (" << dset_def.frame_dimensions[0] << ", " << dset_def.frame_dimensions[1] << ")");
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

} /* namespace FrameProcessor */

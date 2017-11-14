/*
 * FileWriter.cpp
 *
 */

#include <assert.h>

#include <boost/filesystem.hpp>
#include <hdf5_hl.h>

#include "Frame.h"
#include "FileWriterPlugin.h"
#include "FrameProcessorDefinitions.h"

#include "logging.h"

namespace FrameProcessor
{

const std::string FileWriterPlugin::CONFIG_PROCESS             = "process";
const std::string FileWriterPlugin::CONFIG_PROCESS_NUMBER      = "number";
const std::string FileWriterPlugin::CONFIG_PROCESS_RANK        = "rank";
const std::string FileWriterPlugin::CONFIG_PROCESS_BLOCKSIZE   = "frames_per_block";
const std::string FileWriterPlugin::CONFIG_PROCESS_BLOCKS_PER_FILE   = "blocks_per_file";

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
        concurrent_processes_(1),
        concurrent_rank_(0),
        frame_offset_adjustment_(0),
        frames_per_block_(1),
        blocks_per_file_(0),
        timeout_period_(0),
        timeout_thread_running_(true),
        timeout_thread_(boost::bind(&FileWriterPlugin::run_close_file_timeout, this))
{
  this->logger_ = Logger::getLogger("FP.FileWriterPlugin");
  this->logger_->setLevel(Level::getTrace());
  LOG4CXX_TRACE(logger_, "FileWriterPlugin constructor.");
  this->current_acquisition_ = boost::shared_ptr<Acquisition>(new Acquisition());
  this->next_acquisition_ = boost::shared_ptr<Acquisition>(new Acquisition());
}

/**
 * Destructor.
 */
FileWriterPlugin::~FileWriterPlugin()
{
  timeout_thread_running_ = false;
  timeout_active_ = false;
  // Notify the close timeout thread to clean up resources
  {
    boost::mutex::scoped_lock lock2(close_file_mutex_);
    timeout_condition_.notify_all();
  }
  {
    boost::mutex::scoped_lock lock(start_timeout_mutex_);
    start_condition_.notify_all();
  }
  timeout_thread_.join();
  if (writing_) {
    stop_writing();
  }
}

/** Set frame offset
 *
 */
void FileWriterPlugin::set_frame_offset_adjustment(size_t frame_no) {
  this->frame_offset_adjustment_ = frame_no;
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
void FileWriterPlugin::process_frame(boost::shared_ptr<Frame> frame)
{
  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);

  // Start a new acquisition if the frame object contains a different acquisition ID from the current one
  bool can_process_frame = check_acquisition_id(frame);

  if (can_process_frame) {

    if (writing_) {

      ProcessFrameStatus status = current_acquisition_->process_frame(frame);

      // Check if this acquisition is complete and stop
      if (status == status_complete) {
        stop_acquisition();
        // Prevent the timeout from closing the file as it's just been closed. This will also stop the timer if it's running
        timeout_active_ = false;
      } else if (status == status_complete_missing_frames) {
        LOG4CXX_INFO(logger_, "Starting close file timeout as received last frame but missing some frames");
        start_close_file_timeout();
      } else if (status == status_invalid) {
        LOG4CXX_WARN(logger_, "Frame invalid");
      }

      // Push frame to any registered callbacks
      this->push(frame);

      // Notify the close timeout thread that a frame has been processed
      boost::mutex::scoped_lock lock(close_file_mutex_);
      timeout_condition_.notify_all();
    }
  }
}

/** Start writing frames to file.
 *
 * This method checks that the writer is not already writing. Then it creates
 * the datasets required (from their definitions) and creates the HDF5 file
 * ready to write frames. The framesWritten counter is reset to 0.
 */
void FileWriterPlugin::start_writing()
{
  // Set the current acquisition details to the ones held for the next acquisition and reset the next ones
  if (!writing_) {
    this->current_acquisition_ = next_acquisition_;
    this->next_acquisition_ = boost::shared_ptr<Acquisition>(new Acquisition());

    this->current_acquisition_->start_acquisition(concurrent_rank_, concurrent_processes_, frame_offset_adjustment_, frames_per_block_, blocks_per_file_);

    // Set writing flag to true
    writing_ = true;
  }
}

/** Stop writing frames to file.
 *
 * This method checks that the writer is currently writing. Then it closes
 * the file and stops writing frames.
 */
void FileWriterPlugin::stop_writing()
{
  if (writing_) {
    writing_ = false;
    this->current_acquisition_->stop_acquisition();
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
    this->configure_process(processConfig, reply);
  }

  // Check to see if we are configuring the file path and name
  if (config.has_param(FileWriterPlugin::CONFIG_FILE)) {
    OdinData::IpcMessage fileConfig(config.get_param<const rapidjson::Value&>(FileWriterPlugin::CONFIG_FILE));
    this->configure_file(fileConfig, reply);
  }

  // Check to see if we are configuring a dataset
  if (config.has_param(FileWriterPlugin::CONFIG_DATASET)) {
    OdinData::IpcMessage dsetConfig(config.get_param<const rapidjson::Value&>(FileWriterPlugin::CONFIG_DATASET));
    this->configure_dataset(dsetConfig, reply);
  }

  // Check to see if we are being told how many frames to write
  if (config.has_param(FileWriterPlugin::CONFIG_FRAMES) && config.get_param<size_t>(FileWriterPlugin::CONFIG_FRAMES) > 0) {
    size_t totalFrames = config.get_param<size_t>(FileWriterPlugin::CONFIG_FRAMES);
    next_acquisition_->total_frames_ = totalFrames;
    next_acquisition_->frames_to_write_ = calc_num_frames(totalFrames);

    LOG4CXX_INFO(logger_, "Expecting " << next_acquisition_->frames_to_write_ << " frames (total " << totalFrames << ")");
  }

  // Check to see if the master dataset is being set
  if (config.has_param(FileWriterPlugin::CONFIG_MASTER_DATASET)) {
    next_acquisition_->master_frame_ = config.get_param<std::string>(FileWriterPlugin::CONFIG_MASTER_DATASET);
  }

  // Check if we are setting the frame offset adjustment
  if (config.has_param(FileWriterPlugin::CONFIG_OFFSET_ADJUSTMENT)) {
    LOG4CXX_INFO(logger_, "Setting frame offset adjustment to "
        << config.get_param<int>(FileWriterPlugin::CONFIG_OFFSET_ADJUSTMENT));
    frame_offset_adjustment_ = (size_t)config.get_param<int>(FileWriterPlugin::CONFIG_OFFSET_ADJUSTMENT);
  }

  // Check to see if the acquisition id is being set
  if (config.has_param(FileWriterPlugin::ACQUISITION_ID)) {
    next_acquisition_->acquisition_id_ = config.get_param<std::string>(FileWriterPlugin::ACQUISITION_ID);
    LOG4CXX_INFO(logger_, "Setting next Acquisition ID to " << next_acquisition_->acquisition_id_);
  }

  // Check to see if the close file timeout period is being set
  if (config.has_param(FileWriterPlugin::CLOSE_TIMEOUT_PERIOD)) {
    timeout_period_ = config.get_param<size_t>(FileWriterPlugin::CLOSE_TIMEOUT_PERIOD);
    LOG4CXX_INFO(logger_, "Setting close file timeout to " << timeout_period_);
  }

  // Check to see if the close file timeout is being started
  if (config.has_param(FileWriterPlugin::START_CLOSE_TIMEOUT)) {
    if (config.get_param<bool>(FileWriterPlugin::START_CLOSE_TIMEOUT) == true) {
      LOG4CXX_INFO(logger_, "Configure call to start close file timeout");
      if (writing_) {
        start_close_file_timeout();
      } else {
        LOG4CXX_INFO(logger_, "Not starting timeout as not currently writing");
      }
    }
  }

  // Final check is to start or stop writing
  if (config.has_param(FileWriterPlugin::CONFIG_WRITE)) {
    if (config.get_param<bool>(FileWriterPlugin::CONFIG_WRITE) == true) {
      // Only start writing if we have frames to write, or if the total number of frames is 0 (free running mode)
      if (next_acquisition_->total_frames_ > 0 && next_acquisition_->frames_to_write_ == 0) {
        // We're not expecting any frames, so just clear out the nextAcquisition for the next one and don't start writing
        this->next_acquisition_ = boost::shared_ptr<Acquisition>(new Acquisition());
        this->current_acquisition_ = boost::shared_ptr<Acquisition>(new Acquisition());
        LOG4CXX_INFO(logger_, "FrameProcessor will not receive any frames from this acquisition and so no output file will be created");
      } else {
        this->start_writing();
      }
    } else {
      this->stop_writing();
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
void FileWriterPlugin::configure_process(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
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

  // Check to see if the frames per block is being set
  if (config.has_param(FileWriterPlugin::CONFIG_PROCESS_BLOCKS_PER_FILE)) {
    size_t blocks_per_file = config.get_param<size_t>(FileWriterPlugin::CONFIG_PROCESS_BLOCKS_PER_FILE);
    if (this->blocks_per_file_ != blocks_per_file) {
      // If we are writing a file then we cannot change block size
      if (this->writing_) {
        LOG4CXX_ERROR(logger_, "Cannot change blocks per file whilst writing");
        throw std::runtime_error("Cannot change blocks per file whilst writing");
      }
      this->blocks_per_file_ = blocks_per_file;
      LOG4CXX_INFO(logger_, "Setting number of blocks per file to " << blocks_per_file_);
    }
    else {
      LOG4CXX_DEBUG(logger_, "Blocks per file is already " << this->blocks_per_file_);
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
void FileWriterPlugin::configure_file(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  LOG4CXX_DEBUG(logger_, "Configure file name and path");
  // Check for file path and file name
  if (config.has_param(FileWriterPlugin::CONFIG_FILE_PATH)) {
    this->next_acquisition_->file_path_ = config.get_param<std::string>(FileWriterPlugin::CONFIG_FILE_PATH);
    LOG4CXX_DEBUG(logger_, "Next file path changed to " << this->next_acquisition_->file_path_);
  }
  if (config.has_param(FileWriterPlugin::CONFIG_FILE_NAME)) {
    this->next_acquisition_->filename_ = config.get_param<std::string>(FileWriterPlugin::CONFIG_FILE_NAME);
    LOG4CXX_DEBUG(logger_, "Next file name changed to " << this->next_acquisition_->filename_);
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
void FileWriterPlugin::configure_dataset(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  LOG4CXX_DEBUG(logger_, "Configure dataset");
  // Read the dataset command
  if (config.has_param(FileWriterPlugin::CONFIG_DATASET_CMD)) {
    std::string cmd = config.get_param<std::string>(FileWriterPlugin::CONFIG_DATASET_CMD);

    // Command for creating a new dataset description
    if (cmd == "create") {
      DatasetDefinition dset_def;
      // There must be a name present for the dataset
      if (config.has_param(FileWriterPlugin::CONFIG_DATASET_NAME)) {
        dset_def.name = config.get_param<std::string>(FileWriterPlugin::CONFIG_DATASET_NAME);
      } else {
        LOG4CXX_ERROR(logger_, "Cannot create a dataset without a name");
        throw std::runtime_error("Cannot create a dataset without a name");
      }

      // There must be a type present for the dataset
      if (config.has_param(FileWriterPlugin::CONFIG_DATASET_TYPE)) {
        dset_def.pixel = (PixelType)config.get_param<int>(FileWriterPlugin::CONFIG_DATASET_TYPE);
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
        dset_def.compression = (CompressionType)config.get_param<int>(FileWriterPlugin::CONFIG_DATASET_COMPRESSION);
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
      this->next_acquisition_->dataset_defs_[dset_def.name] = dset_def;
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
  status.set_param(get_name() + "/writing", this->writing_);
  status.set_param(get_name() + "/frames_max", (int)this->current_acquisition_->frames_to_write_);
  status.set_param(get_name() + "/frames_written", (int)this->current_acquisition_->frames_written_);
  status.set_param(get_name() + "/file_path", this->current_acquisition_->file_path_);
  status.set_param(get_name() + "/file_name", this->current_acquisition_->filename_);
  status.set_param(get_name() + "/acquisition_id", this->current_acquisition_->acquisition_id_);
  status.set_param(get_name() + "/processes", (int)this->concurrent_processes_);
  status.set_param(get_name() + "/rank", (int)this->concurrent_rank_);

  // Check for datasets
  std::map<std::string, DatasetDefinition>::iterator iter;
  for (iter = this->current_acquisition_->dataset_defs_.begin(); iter != this->current_acquisition_->dataset_defs_.end(); ++iter) {
    // Add the dataset type
    status.set_param(get_name() + "/datasets/" + iter->first + "/type", (int)iter->second.pixel);

    // Check for and add dimensions
    if (iter->second.frame_dimensions.size() > 0) {
      std::string dimParamName = get_name() + "/datasets/" + iter->first + "/dimensions[]";
      for (int index = 0; index < iter->second.frame_dimensions.size(); index++) {
        status.set_param(dimParamName, (int)iter->second.frame_dimensions[index]);
      }
    }
    // Check for and add chunking dimensions
    if (iter->second.chunks.size() > 0) {
      std::string chunkParamName = get_name() + "/datasets/" + iter->first + "/chunks[]";
      for (int index = 0; index < iter->second.chunks.size(); index++) {
        status.set_param(chunkParamName, (int)iter->second.chunks[index]);
      }
    }
  }
}

/** Check if the frame contains an acquisition ID and start a new file if it does and it's different from the current one
 *
 * If the frame object contains an acquisition ID, then we need to check if the current acquisition we are writing has
 * the same ID. If it is different, then we close the current file and create a new one and start writing.
 * If we are not currently writing then we just create a new file and start writing.
 *
 * \param[in] frame - Pointer to the Frame object.
 */
bool FileWriterPlugin::check_acquisition_id(boost::shared_ptr<Frame> frame) {
  if (!frame->get_acquisition_id().empty()) {
    if (writing_) {
      if (frame->get_acquisition_id() == current_acquisition_->acquisition_id_) {
        // On same file, take no action
        return true;
      }
    }

    if (frame->get_acquisition_id() == next_acquisition_->acquisition_id_) {
      LOG4CXX_DEBUG(logger_, "Acquisition ID sent in frame matches next acquisition ID. Closing current file and starting next");
      stop_writing();
      start_writing();
    } else {
      LOG4CXX_WARN(logger_, "Unexpected acquisition ID on frame [" << frame->get_acquisition_id() << "] for frame " << frame->get_frame_number());
      // TODO set status? (There's currently no mechanism to report this in the status message)
      return false;
    }
  }
  return true;
}

/**
 * Stops the current acquisition and starts the next if it is configured
 *
 */
void FileWriterPlugin::stop_acquisition() {
  this->stop_writing();
  // Start next acquisition if we have a filename or acquisition ID to use
  if (!next_acquisition_->filename_.empty() || !next_acquisition_->acquisition_id_.empty()) {
    if (next_acquisition_->total_frames_ > 0 && next_acquisition_->frames_to_write_ == 0) {
      // We're not expecting any frames, so just clear out the nextAcquisition for the next one and don't start writing
      this->next_acquisition_ = boost::shared_ptr<Acquisition>(new Acquisition());
      LOG4CXX_INFO(logger_, "FrameProcessor will not receive any frames from this acquisition and so no output file will be created");
    } else {
      this->start_writing();
    }
  }
}

/**
 * Starts the close file timeout
 *
 */
void FileWriterPlugin::start_close_file_timeout()
{
  if (timeout_active_ == false) {
    LOG4CXX_INFO(logger_, "Starting close file timeout");
    boost::mutex::scoped_lock lock(start_timeout_mutex_);
    start_condition_.notify_all();
  } else {
    LOG4CXX_INFO(logger_, "Close file timeout already active");
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
void FileWriterPlugin::run_close_file_timeout()
{
  OdinData::configure_logging_mdc(OdinData::app_path.c_str());
  boost::mutex::scoped_lock startLock(start_timeout_mutex_);
  while (timeout_thread_running_) {
    start_condition_.wait(startLock);
    if (timeout_thread_running_) {
      timeout_active_ = true;
      boost::mutex::scoped_lock lock(close_file_mutex_);
      while (timeout_active_) {
        if (!timeout_condition_.timed_wait(lock, boost::posix_time::milliseconds(timeout_period_))) {
          // Timeout
          LOG4CXX_DEBUG(logger_, "Close file Timeout timed out");
          boost::lock_guard<boost::recursive_mutex> lock(mutex_);
          if (writing_ && timeout_active_) {
            LOG4CXX_INFO(logger_, "Timed out waiting for frames, stopping writing");
            stop_acquisition();
          }
          timeout_active_ = false;
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
size_t FileWriterPlugin::calc_num_frames(size_t totalFrames)
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

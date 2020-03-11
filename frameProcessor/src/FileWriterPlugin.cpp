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
#include "DebugLevelLogger.h"
#include "version.h"

namespace FrameProcessor
{

const std::string FileWriterPlugin::CONFIG_PROCESS                     = "process";
const std::string FileWriterPlugin::CONFIG_PROCESS_NUMBER              = "number";
const std::string FileWriterPlugin::CONFIG_PROCESS_RANK                = "rank";
const std::string FileWriterPlugin::CONFIG_PROCESS_BLOCKSIZE           = "frames_per_block";
const std::string FileWriterPlugin::CONFIG_PROCESS_BLOCKS_PER_FILE     = "blocks_per_file";
const std::string FileWriterPlugin::CONFIG_PROCESS_EARLIEST_VERSION    = "earliest_version";
const std::string FileWriterPlugin::CONFIG_PROCESS_ALIGNMENT_THRESHOLD = "alignment_threshold";
const std::string FileWriterPlugin::CONFIG_PROCESS_ALIGNMENT_VALUE     = "alignment_value";

const std::string FileWriterPlugin::CONFIG_FILE                        = "file";
const std::string FileWriterPlugin::CONFIG_FILE_NAME                   = "name";
const std::string FileWriterPlugin::CONFIG_FILE_PATH                   = "path";
const std::string FileWriterPlugin::CONFIG_FILE_EXTENSION              = "extension";

const std::string FileWriterPlugin::CONFIG_DATASET                     = "dataset";
const std::string FileWriterPlugin::CONFIG_DATASET_TYPE                = "datatype";
const std::string FileWriterPlugin::CONFIG_DATASET_DIMS                = "dims";
const std::string FileWriterPlugin::CONFIG_DATASET_CHUNKS              = "chunks";
const std::string FileWriterPlugin::CONFIG_DATASET_COMPRESSION         = "compression";
const std::string FileWriterPlugin::CONFIG_DATASET_INDEXES             = "indexes";
const std::string FileWriterPlugin::CONFIG_DATASET_BLOSC_COMPRESSOR    = "blosc_compressor";
const std::string FileWriterPlugin::CONFIG_DATASET_BLOSC_LEVEL         = "blosc_level";
const std::string FileWriterPlugin::CONFIG_DATASET_BLOSC_SHUFFLE       = "blosc_shuffle";

const std::string FileWriterPlugin::CONFIG_FRAMES                      = "frames";
const std::string FileWriterPlugin::CONFIG_MASTER_DATASET              = "master";
const std::string FileWriterPlugin::CONFIG_WRITE                       = "write";
const std::string FileWriterPlugin::ACQUISITION_ID                     = "acquisition_id";
const std::string FileWriterPlugin::CLOSE_TIMEOUT_PERIOD               = "timeout_timer_period";
const std::string FileWriterPlugin::START_CLOSE_TIMEOUT                = "start_timeout_timer";

const std::string FileWriterPlugin::CREATE_ERROR_DURATION = "create_error_duration";
const std::string FileWriterPlugin::WRITE_ERROR_DURATION = "write_error_duration";
const std::string FileWriterPlugin::FLUSH_ERROR_DURATION = "flush_error_duration";
const std::string FileWriterPlugin::CLOSE_ERROR_DURATION = "close_error_duration";


/**
 * Create a FileWriterPlugin with default values.
 * File path is set to default of current directory, and the
 * filename is set to a default.
 *
 * The writer plugin is also configured to be a single
 * process writer (no other expected writers).
 */
FileWriterPlugin::FileWriterPlugin() :
        writing_(false),
        concurrent_processes_(1),
        concurrent_rank_(0),
        frames_per_block_(1),
        blocks_per_file_(0),
        file_extension_("h5"),
        use_earliest_hdf5_(false),
        alignment_threshold_(1),
        alignment_value_(1),
        timeout_period_(0),
        timeout_thread_running_(true),
        timeout_thread_(boost::bind(&FileWriterPlugin::run_close_file_timeout, this))
{
  this->logger_ = Logger::getLogger("FP.FileWriterPlugin");
  this->logger_->setLevel(Level::getTrace());
  LOG4CXX_INFO(logger_, "FileWriterPlugin version " << this->get_version_long() << " loaded");
  this->current_acquisition_ = boost::shared_ptr<Acquisition>(new Acquisition(hdf5_error_definition_));
  this->next_acquisition_ = boost::shared_ptr<Acquisition>(new Acquisition(hdf5_error_definition_));
  hdf5_error_definition_.create_duration = 0;
  hdf5_error_definition_.write_duration = 0;
  hdf5_error_definition_.flush_duration = 0;
  hdf5_error_definition_.close_duration = 0;
  hdf5_error_definition_.callback = boost::bind(&FileWriterPlugin::set_error, this, _1);
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
  boost::mutex::scoped_lock cflock(close_file_mutex_);
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);

  // Check the frame against the current acquisition and start a new one if
  // the frame object contains a different acquisition ID from the current one
  if (frame_in_acquisition(frame)) {

    if (writing_) {

      ProcessFrameStatus status = current_acquisition_->process_frame(frame, hdf5_call_durations_);

      // Check if this acquisition is complete and stop
      if (status == status_complete) {
        stop_acquisition();
        // Prevent the timeout from closing the file as it's just been closed. This will also stop the timer if it's running
        timeout_active_ = false;
      } else if (status == status_complete_missing_frames) {
        LOG4CXX_INFO(logger_, "Starting close file timeout as received last frame but missing some frames");
        start_close_file_timeout();
      } else if (status == status_invalid) {
        this->set_error("Frame invalid");
        this->set_error(current_acquisition_->get_last_error());
      }

      // Push frame to any registered callbacks
      this->push(frame);

      // Notify the close timeout thread that a frame has been processed
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
    // Re-calculate the number of frames to write in case the process and
    // rank has been changed since the frame count was set
    next_acquisition_->frames_to_write_ = calc_num_frames(this->next_acquisition_->total_frames_);
    this->current_acquisition_ = next_acquisition_;
    this->next_acquisition_ = boost::shared_ptr<Acquisition>(new Acquisition(hdf5_error_definition_));

    // Set up datasets within the current acquisition
    std::map<std::string, DatasetDefinition>::iterator iter;
    for (iter = this->dataset_defs_.begin(); iter != this->dataset_defs_.end(); ++iter){
      this->current_acquisition_->dataset_defs_[iter->first] = iter->second;
    }

    // Start the acquisition and set writing flag to true if it started successfully
    writing_ = this->current_acquisition_->start_acquisition(
        concurrent_rank_,
        concurrent_processes_,
        frames_per_block_,
        blocks_per_file_,
        file_extension_,
        use_earliest_hdf5_,
        alignment_threshold_,
        alignment_value_,
        master_frame_,
        hdf5_call_durations_);
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
    this->current_acquisition_->stop_acquisition(hdf5_call_durations_);
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

  try {
    // Check to see if we are configuring the process number and rank
    if (config.has_param(FileWriterPlugin::CONFIG_PROCESS)) {
      OdinData::IpcMessage processConfig(config.get_param<const rapidjson::Value &>(FileWriterPlugin::CONFIG_PROCESS));
      this->configure_process(processConfig, reply);
    }

    // Check to see if we are configuring the file path and name
    if (config.has_param(FileWriterPlugin::CONFIG_FILE)) {
      OdinData::IpcMessage fileConfig(config.get_param<const rapidjson::Value &>(FileWriterPlugin::CONFIG_FILE));
      this->configure_file(fileConfig, reply);
    }

    // Check to see if we are configuring a dataset
    if (config.has_param(FileWriterPlugin::CONFIG_DATASET)) {
      // Attempt to retrieve the value as a string parameter
      try {
        LOG4CXX_INFO(logger_, "Checking for string name of dataset");
        std::string dataset_name = config.get_param<std::string>(FileWriterPlugin::CONFIG_DATASET);
        LOG4CXX_INFO(logger_, "Dataset name " << dataset_name << " found, creating...");
        // If we can retrieve a single string parameter then we are being asked to create
        // a new dataset.  Only create it if it doesn't already exist.
        create_new_dataset(dataset_name);
      } catch (OdinData::IpcMessageException &e) {
        // The object passed to us is a dataset description so pass to the configure_dataset method.
        OdinData::IpcMessage dataset_config(
            config.get_param<const rapidjson::Value &>(FileWriterPlugin::CONFIG_DATASET));
        std::vector <std::string> dataset_names = dataset_config.get_param_names();
        for (std::vector<std::string>::iterator iter = dataset_names.begin();
             iter != dataset_names.end(); ++iter) {
          std::string dataset_name = *iter;
          LOG4CXX_INFO(logger_, "Dataset name " << dataset_name << " found, creating...");
          create_new_dataset(dataset_name);
          OdinData::IpcMessage dsetConfig(dataset_config.get_param<const rapidjson::Value &>(dataset_name));
          this->configure_dataset(dataset_name, dsetConfig, reply);
        }
      }
    }

    // Check to see if we are being told how many frames to write
    if (config.has_param(FileWriterPlugin::CONFIG_FRAMES) &&
        config.get_param<size_t>(FileWriterPlugin::CONFIG_FRAMES) > 0) {
      size_t totalFrames = config.get_param<size_t>(FileWriterPlugin::CONFIG_FRAMES);
      next_acquisition_->total_frames_ = totalFrames;
      next_acquisition_->frames_to_write_ = calc_num_frames(totalFrames);

      LOG4CXX_INFO(logger_,
                   "Expecting " << next_acquisition_->frames_to_write_ << " frames (total " << totalFrames << ")");
    }

    // Check to see if the master dataset is being set
    if (config.has_param(FileWriterPlugin::CONFIG_MASTER_DATASET)) {
      master_frame_ = config.get_param<std::string>(FileWriterPlugin::CONFIG_MASTER_DATASET);
      LOG4CXX_INFO(logger_, "Setting master frame dataset to " << master_frame_);
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
          this->next_acquisition_ = boost::shared_ptr<Acquisition>(new Acquisition(hdf5_error_definition_));
          if (!writing_) {
            this->current_acquisition_ = boost::shared_ptr<Acquisition>(new Acquisition(hdf5_error_definition_));
          }
          LOG4CXX_INFO(logger_,
                       "FrameProcessor will not receive any frames from this acquisition and so no output file will be created");
        } else {
          this->start_writing();
        }
      } else {
        this->stop_writing();
      }
    }
  }
  catch (std::runtime_error& e)
  {
    this->set_error(e.what());
    throw;
  }
}

void FileWriterPlugin::requestConfiguration(OdinData::IpcMessage& reply)
{
  // Return the configuration of the file writer plugin
  std::string process_str = get_name() + "/" + FileWriterPlugin::CONFIG_PROCESS + "/";
  reply.set_param(process_str + FileWriterPlugin::CONFIG_PROCESS_NUMBER, concurrent_processes_);
  reply.set_param(process_str + FileWriterPlugin::CONFIG_PROCESS_RANK, concurrent_rank_);
  reply.set_param(process_str + FileWriterPlugin::CONFIG_PROCESS_BLOCKSIZE, frames_per_block_);
  reply.set_param(process_str + FileWriterPlugin::CONFIG_PROCESS_BLOCKS_PER_FILE, blocks_per_file_);
  reply.set_param(process_str + FileWriterPlugin::CONFIG_PROCESS_EARLIEST_VERSION, use_earliest_hdf5_);
  reply.set_param(process_str + FileWriterPlugin::CONFIG_PROCESS_ALIGNMENT_THRESHOLD, alignment_threshold_);
  reply.set_param(process_str + FileWriterPlugin::CONFIG_PROCESS_ALIGNMENT_VALUE, alignment_value_);

  std::string file_str = get_name() + "/" + FileWriterPlugin::CONFIG_FILE + "/";
  reply.set_param(file_str + FileWriterPlugin::CONFIG_FILE_PATH, next_acquisition_->file_path_);
  reply.set_param(file_str + FileWriterPlugin::CONFIG_FILE_NAME, next_acquisition_->configured_filename_);
  reply.set_param(file_str + FileWriterPlugin::CONFIG_FILE_EXTENSION, file_extension_);
  // Configure HDF5 call error durations
  reply.set_param(file_str + FileWriterPlugin::CREATE_ERROR_DURATION, hdf5_error_definition_.create_duration);
  reply.set_param(file_str + FileWriterPlugin::WRITE_ERROR_DURATION, hdf5_error_definition_.write_duration);
  reply.set_param(file_str + FileWriterPlugin::FLUSH_ERROR_DURATION, hdf5_error_definition_.flush_duration);
  reply.set_param(file_str + FileWriterPlugin::CLOSE_ERROR_DURATION, hdf5_error_definition_.close_duration);

  reply.set_param(get_name() + "/" + FileWriterPlugin::CONFIG_FRAMES, next_acquisition_->total_frames_);
  reply.set_param(get_name() + "/" + FileWriterPlugin::CONFIG_MASTER_DATASET, master_frame_);
  reply.set_param(get_name() + "/" + FileWriterPlugin::ACQUISITION_ID, next_acquisition_->acquisition_id_);
  reply.set_param(get_name() + "/" + FileWriterPlugin::CLOSE_TIMEOUT_PERIOD, timeout_period_);

  // Check for datasets
  std::map<std::string, DatasetDefinition>::iterator iter;
  for (iter = this->dataset_defs_.begin(); iter != this->dataset_defs_.end(); ++iter) {
    // Add the dataset type
    reply.set_param(get_name() + "/dataset/" + iter->first + "/" + FileWriterPlugin::CONFIG_DATASET_TYPE, get_type_from_enum(iter->second.data_type));

    // Add the dataset compression
    reply.set_param(get_name() + "/dataset/" + iter->first + "/" + FileWriterPlugin::CONFIG_DATASET_COMPRESSION, get_compress_from_enum(iter->second.compression));
    reply.set_param(get_name() + "/dataset/" + iter->first + "/" + FileWriterPlugin::CONFIG_DATASET_BLOSC_COMPRESSOR, (int)iter->second.blosc_compressor);
    reply.set_param(get_name() + "/dataset/" + iter->first + "/" + FileWriterPlugin::CONFIG_DATASET_BLOSC_LEVEL, (int)iter->second.blosc_level);
    reply.set_param(get_name() + "/dataset/" + iter->first + "/" + FileWriterPlugin::CONFIG_DATASET_BLOSC_SHUFFLE, (int)iter->second.blosc_shuffle);

    // Check for and add dimensions
    if (iter->second.frame_dimensions.size() > 0) {
      std::string dimParamName = get_name() + "/dataset/" + iter->first + "/" + FileWriterPlugin::CONFIG_DATASET_DIMS + "[]";
      for (int index = 0; index < iter->second.frame_dimensions.size(); index++) {
        reply.set_param(dimParamName, (int)iter->second.frame_dimensions[index]);
      }
    }
    // Check for and add chunking dimensions
    if (iter->second.chunks.size() > 0) {
      std::string chunkParamName = get_name() + "/dataset/" + iter->first + "/" + FileWriterPlugin::CONFIG_DATASET_CHUNKS + "[]";
      for (int index = 0; index < iter->second.chunks.size(); index++) {
        reply.set_param(chunkParamName, (int)iter->second.chunks[index]);
      }
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
        std::string message = "Cannot change concurrent processes whilst writing";
        set_error(message);
        throw std::runtime_error(message);
      }
      this->concurrent_processes_ = processes;
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Concurrent processes changed to " << this->concurrent_processes_);
    }
    else {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Concurrent processes is already " << this->concurrent_processes_);
    }
  }
  // Check for rank number
  if (config.has_param(FileWriterPlugin::CONFIG_PROCESS_RANK)) {
    size_t rank = config.get_param<size_t>(FileWriterPlugin::CONFIG_PROCESS_RANK);
    if (this->concurrent_rank_ != rank) {
      // If we are writing a file then we cannot change concurrent rank
      if (this->writing_) {
        std::string message = "Cannot change process rank whilst writing";
        set_error(message);
        throw std::runtime_error(message);
      }
      this->concurrent_rank_ = rank;
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Process rank changed to " << this->concurrent_rank_);
    }
    else {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Process rank is already " << this->concurrent_rank_);
    }
  }

  // Check to see if the frames per block is being set
  if (config.has_param(FileWriterPlugin::CONFIG_PROCESS_BLOCKSIZE)) {
    size_t block_size = config.get_param<size_t>(FileWriterPlugin::CONFIG_PROCESS_BLOCKSIZE);
    if (this->frames_per_block_ != block_size) {
      if (block_size < 1) {
        std::string message = "Must have at least one frame per block";
        set_error(message);
        throw std::runtime_error(message);
      }
      // If we are writing a file then we cannot change block size
      if (this->writing_) {
        std::string message = "Cannot change block size whilst writing";
        set_error(message);
        throw std::runtime_error(message);
      }
      this->frames_per_block_ = block_size;
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting number of frames per block to " << frames_per_block_);
    }
    else {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Block size is already " << this->frames_per_block_);
    }
  }

  // Check to see if the frames per block is being set
  if (config.has_param(FileWriterPlugin::CONFIG_PROCESS_BLOCKS_PER_FILE)) {
    size_t blocks_per_file = config.get_param<size_t>(FileWriterPlugin::CONFIG_PROCESS_BLOCKS_PER_FILE);
    if (this->blocks_per_file_ != blocks_per_file) {
      // If we are writing a file then we cannot change block size
      if (this->writing_) {
        std::string message = "Cannot change blocks per file whilst writing";
        set_error(message);
        throw std::runtime_error(message);
      }
      this->blocks_per_file_ = blocks_per_file;
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting number of blocks per file to " << blocks_per_file_);
    }
    else {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Blocks per file is already " << this->blocks_per_file_);
    }
  }

  // Check for hdf5 version
  if (config.has_param(FileWriterPlugin::CONFIG_PROCESS_EARLIEST_VERSION)) {
    this->use_earliest_hdf5_ = config.get_param<bool>(FileWriterPlugin::CONFIG_PROCESS_EARLIEST_VERSION);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Use earliest version of HDF5 library to write file set to " << this->use_earliest_hdf5_);
  }

  // Check for alignment value and threshold
  if (config.has_param(FileWriterPlugin::CONFIG_PROCESS_ALIGNMENT_THRESHOLD)) {
    this->alignment_threshold_ = config.get_param<size_t>(FileWriterPlugin::CONFIG_PROCESS_ALIGNMENT_THRESHOLD);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Chunk alignment threshold set to " << this->alignment_threshold_);
  }
  if (config.has_param(FileWriterPlugin::CONFIG_PROCESS_ALIGNMENT_VALUE)) {
    this->alignment_value_ = config.get_param<size_t>(FileWriterPlugin::CONFIG_PROCESS_ALIGNMENT_VALUE);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Chunk alignment value set to " << this->alignment_value_);
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
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Configure file name and path");
  // Check for file path and file name
  if (config.has_param(FileWriterPlugin::CONFIG_FILE_PATH)) {
    this->next_acquisition_->file_path_ = config.get_param<std::string>(FileWriterPlugin::CONFIG_FILE_PATH);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Next file path changed to " << this->next_acquisition_->file_path_);
  }
  if (config.has_param(FileWriterPlugin::CONFIG_FILE_NAME)) {
    this->next_acquisition_->configured_filename_ = config.get_param<std::string>(FileWriterPlugin::CONFIG_FILE_NAME);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Next file name changed to " << this->next_acquisition_->configured_filename_);
  }
  if (config.has_param(FileWriterPlugin::CONFIG_FILE_EXTENSION)) {
    this->file_extension_ = config.get_param<std::string>(FileWriterPlugin::CONFIG_FILE_EXTENSION);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "File extension changed to " << this->file_extension_);
  }
  // Check for HDF5 call error durations
  if (config.has_param(FileWriterPlugin::CREATE_ERROR_DURATION)) {
    this->hdf5_error_definition_.create_duration = config.get_param<unsigned int>(FileWriterPlugin::CREATE_ERROR_DURATION);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Open error duration changed to " << this->hdf5_error_definition_.create_duration);
  }
  if (config.has_param(FileWriterPlugin::WRITE_ERROR_DURATION)) {
    this->hdf5_error_definition_.write_duration = config.get_param<unsigned int>(FileWriterPlugin::WRITE_ERROR_DURATION);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Write error duration changed to " << this->hdf5_error_definition_.write_duration);
  }
  if (config.has_param(FileWriterPlugin::FLUSH_ERROR_DURATION)) {
    this->hdf5_error_definition_.flush_duration = config.get_param<unsigned int>(FileWriterPlugin::FLUSH_ERROR_DURATION);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Flush error duration changed to " << this->hdf5_error_definition_.flush_duration);
  }
  if (config.has_param(FileWriterPlugin::CLOSE_ERROR_DURATION)) {
    this->hdf5_error_definition_.close_duration = config.get_param<unsigned int>(FileWriterPlugin::CLOSE_ERROR_DURATION);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Close error duration changed to " << this->hdf5_error_definition_.close_duration);
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
void FileWriterPlugin::configure_dataset(const std::string& dataset_name, OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  LOG4CXX_DEBUG_LEVEL(1, logger_, "Configuring dataset [" << dataset_name << "]");

  DatasetDefinition dset = dataset_defs_[dataset_name];

  // If there is a type present then set it
  if (config.has_param(FileWriterPlugin::CONFIG_DATASET_TYPE)) {
    dset.data_type = get_type_from_string(config.get_param<std::string>(FileWriterPlugin::CONFIG_DATASET_TYPE));
  }

  // If there are dimensions present for the dataset then set them
  if (config.has_param(FileWriterPlugin::CONFIG_DATASET_DIMS)) {
    const rapidjson::Value& val = config.get_param<const rapidjson::Value&>(FileWriterPlugin::CONFIG_DATASET_DIMS);
    // Loop over the dimension values
    dimensions_t dims(val.Size());
    for (rapidjson::SizeType i = 0; i < val.Size(); i++) {
      const rapidjson::Value& dim = val[i];
      dims[i] = dim.GetUint64();
    }
    dset.frame_dimensions = dims;
    // Create default chunking for the dataset (to include n dimension)
    dimensions_t chunks(dset.frame_dimensions.size()+1);
    // Set first chunk dimension (n dimension) to a single frame or item
    chunks[0] = 1;
    // Set the remaining chunk dimensions to the same as the dataset dimensions
    for (int index = 0; index < dset.frame_dimensions.size(); index++){
      chunks[index+1] = dset.frame_dimensions[index];
    }
    dset.chunks = chunks;
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
    dset.chunks = chunks;
  }

  // Check if compression has been specified for the raw data
  if (config.has_param(FileWriterPlugin::CONFIG_DATASET_COMPRESSION)) {
    dset.compression = get_compression_from_string(config.get_param<std::string>(FileWriterPlugin::CONFIG_DATASET_COMPRESSION));
    LOG4CXX_INFO(logger_, "Enabling compression: " << dset.compression);
  }
  // Blosc compression require a set of parameters to be defined
  if (config.has_param(FileWriterPlugin::CONFIG_DATASET_BLOSC_COMPRESSOR)) {
    dset.blosc_compressor = config.get_param<unsigned int>(FileWriterPlugin::CONFIG_DATASET_BLOSC_COMPRESSOR);
    if (dset.blosc_compressor<0 || dset.blosc_compressor>5) {
      LOG4CXX_ERROR(logger_, "Invalid blosc compression setting " << dset.blosc_compressor);
    }
  }
  if (config.has_param(FileWriterPlugin::CONFIG_DATASET_BLOSC_LEVEL)) {
    dset.blosc_level = config.get_param<unsigned int>(FileWriterPlugin::CONFIG_DATASET_BLOSC_LEVEL);
    if (dset.blosc_level<0 || dset.blosc_level>9) {
      LOG4CXX_ERROR(logger_, "Invalid blosc level setting " << dset.blosc_level);
    }
  }
  if (config.has_param(FileWriterPlugin::CONFIG_DATASET_BLOSC_SHUFFLE)) {
    dset.blosc_shuffle = config.get_param<unsigned int>(FileWriterPlugin::CONFIG_DATASET_BLOSC_SHUFFLE);
    if (dset.blosc_shuffle<-1 || dset.blosc_shuffle>2) {
      LOG4CXX_ERROR(logger_, "Invalid blosc shuffle setting " << dset.blosc_shuffle);
    }
  }
  if (dset.compression == blosc) {
    LOG4CXX_INFO(logger_, "Blosc compression settings: compressor=" << dset.blosc_compressor
                           << " level=" << dset.blosc_level << " shuffle=" << dset.blosc_shuffle);
  }

  // Check if creating the high/low indexes has been specified
  if (config.has_param(FileWriterPlugin::CONFIG_DATASET_INDEXES)) {
    dset.create_low_high_indexes = config.get_param<bool>(FileWriterPlugin::CONFIG_DATASET_INDEXES);
  }

  // Add the dataset definition to the store
  dataset_defs_[dataset_name] = dset;
}

/**
 * Checks to see if a dataset with the supplied name already exists.  If it doesn't then
 * the dataset definition is created and then added to the store.
 *
 * \param[in] dset_name - Name of the dataset to create.
 */
void FileWriterPlugin::create_new_dataset(const std::string& dset_name)
{
  if (dataset_defs_.count(dset_name) < 1){
    DatasetDefinition dset_def;
    // Provide default values for the dataset
    dset_def.name = dset_name;
    dset_def.data_type = raw_8bit;
    dset_def.compression = no_compression;
    dset_def.blosc_compressor = 0;
    dset_def.blosc_level = 0;
    dset_def.blosc_shuffle = 0;
    dset_def.num_frames = 1;
    std::vector<long long unsigned int> dims(0);
    dset_def.frame_dimensions = dims;
    dset_def.chunks = dims;
    dset_def.create_low_high_indexes = false;
    // Record the dataset in the definitions
    dataset_defs_[dset_def.name] = dset_def;
  }
}

/**
 * Collate status information for the plugin. The status is added to the status IpcMessage object.
 *
 * \param[out] status - Reference to an IpcMessage value to store the status.
 */
void FileWriterPlugin::status(OdinData::IpcMessage& status)
{
  // Record the plugin's status items
  status.set_param(get_name() + "/writing", this->writing_);
  status.set_param(get_name() + "/frames_max", (int)this->current_acquisition_->frames_to_write_);
  status.set_param(get_name() + "/frames_written", (int)this->current_acquisition_->frames_written_);
  status.set_param(get_name() + "/frames_processed", (int)this->current_acquisition_->frames_processed_);
  status.set_param(get_name() + "/file_path", this->current_acquisition_->file_path_);
  status.set_param(get_name() + "/file_name", this->current_acquisition_->filename_);
  status.set_param(get_name() + "/acquisition_id", this->current_acquisition_->acquisition_id_);
  status.set_param(get_name() + "/processes", (int)this->concurrent_processes_);
  status.set_param(get_name() + "/rank", (int)this->concurrent_rank_);
  status.set_param(get_name() + "/timeout_active", this->timeout_active_);
  add_file_writing_stats(status);
}

/**
 * Collate file writing statistics for the plugin.
 *
 * The metrics are added to the status IpcMessage object.
 *
 * \param[out] status - Reference to an IpcMessage value to store the file writing stats.
 */
void FileWriterPlugin::add_file_writing_stats(OdinData::IpcMessage& status)
{
  status.set_param(get_name() + "/timing/last_create", (int) hdf5_call_durations_.create.last_);
  status.set_param(get_name() + "/timing/max_create", (int) hdf5_call_durations_.create.max_);
  status.set_param(get_name() + "/timing/mean_create", (int) hdf5_call_durations_.create.mean_);
  status.set_param(get_name() + "/timing/last_write", (int) hdf5_call_durations_.write.last_);
  status.set_param(get_name() + "/timing/max_write", (int) hdf5_call_durations_.write.max_);
  status.set_param(get_name() + "/timing/mean_write", (int) hdf5_call_durations_.write.mean_);
  status.set_param(get_name() + "/timing/last_flush", (int) hdf5_call_durations_.flush.last_);
  status.set_param(get_name() + "/timing/max_flush", (int) hdf5_call_durations_.flush.max_);
  status.set_param(get_name() + "/timing/mean_flush", (int) hdf5_call_durations_.flush.mean_);
  status.set_param(get_name() + "/timing/last_close", (int) hdf5_call_durations_.close.last_);
  status.set_param(get_name() + "/timing/max_close", (int) hdf5_call_durations_.close.max_);
  status.set_param(get_name() + "/timing/mean_close", (int) hdf5_call_durations_.close.mean_);
}

/**
 * Reset file writing statistics
 */
bool FileWriterPlugin::reset_statistics()
{
  hdf5_call_durations_.create.reset();
  hdf5_call_durations_.write.reset();
  hdf5_call_durations_.flush.reset();
  hdf5_call_durations_.close.reset();
  return true;
}

/** Check if the frame contains an acquisition ID and start a new file if it does and it's different from the current one
 *
 * If the frame object contains an acquisition ID, then we need to check if the current acquisition we are writing has
 * the same ID. If it is different, then we close the current file and create a new one and start writing.
 * If we are not currently writing then we just create a new file and start writing.
 *
 * \param[in] frame - Pointer to the Frame object.
 */
bool FileWriterPlugin::frame_in_acquisition(boost::shared_ptr<Frame> frame) {

  std::string frame_acquisition_ID = frame->get_meta_data().get_acquisition_ID();

  if (!frame_acquisition_ID.empty()) {
    if (writing_) {
      if (frame_acquisition_ID == current_acquisition_->acquisition_id_) {
        // On same file, take no action
        return true;
      }
    }

    if (frame_acquisition_ID == next_acquisition_->acquisition_id_) {
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Acquisition ID sent in frame matches next acquisition ID. "
                                      "Closing current file and starting next");
      stop_writing();
      start_writing();
    } else {
      std::stringstream ss;
      ss << "Unexpected acquisition ID on acquisition (" << frame_acquisition_ID << ")";
      if (writing_)
      {
        set_error(ss.str());
        ss << " for frame " << frame->get_frame_number();
        LOG4CXX_WARN(logger_, ss.str());
      }
      else
      {
        ss << " for frame " << frame->get_frame_number();
        LOG4CXX_DEBUG(logger_, ss.str());
      }
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
  // Before stopping the current acquisition, check for identical paths, filenames or
  // acquisition IDs.  If these are found do not automatically start the next acquisition
  bool restart = true;
  if (writing_){
    if (!next_acquisition_->file_path_.empty()){
      if (next_acquisition_->file_path_ == current_acquisition_->file_path_){
        if (!next_acquisition_->configured_filename_.empty()){
          if (next_acquisition_->configured_filename_ == current_acquisition_->configured_filename_){
            // Identical path and filenames so do not re-start
            restart = false;
            LOG4CXX_INFO(logger_, "FrameProcessor will not auto-restart acquisition due to identical filename and path");
          }
        }
        if (!next_acquisition_->acquisition_id_.empty()){
          if (next_acquisition_->acquisition_id_ == current_acquisition_->acquisition_id_){
            // Identical path and acquisition IDs so do not re-start
            restart = false;
            LOG4CXX_INFO(logger_, "FrameProcessor will not auto-restart acquisition due to identical file path and acquisition ID");
          }
        }
      }
    }
  }
  this->stop_writing();
  if (restart){
    // Start next acquisition if we have a filename or acquisition ID to use
    if (!next_acquisition_->configured_filename_.empty() || !next_acquisition_->acquisition_id_.empty()) {
      if (next_acquisition_->total_frames_ > 0 && next_acquisition_->frames_to_write_ == 0) {
        // We're not expecting any frames, so just clear out the nextAcquisition for the next one and don't start writing
        this->next_acquisition_ = boost::shared_ptr<Acquisition>(new Acquisition(hdf5_error_definition_));
        LOG4CXX_INFO(logger_, "FrameProcessor will not receive any frames from this acquisition and so no output file will be created");
      } else {
        this->start_writing();
      }
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
          LOG4CXX_DEBUG_LEVEL(1, logger_, "Close file Timeout timed out");
          boost::lock_guard<boost::recursive_mutex> lock(mutex_);
          if (writing_ && timeout_active_) {
            set_error("Timed out waiting for frames, stopping writing");
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

int FileWriterPlugin::get_version_major()
{
  return ODIN_DATA_VERSION_MAJOR;
}

int FileWriterPlugin::get_version_minor()
{
  return ODIN_DATA_VERSION_MINOR;
}

int FileWriterPlugin::get_version_patch()
{
  return ODIN_DATA_VERSION_PATCH;
}

std::string FileWriterPlugin::get_version_short()
{
  return ODIN_DATA_VERSION_STR_SHORT;
}

std::string FileWriterPlugin::get_version_long()
{
  return ODIN_DATA_VERSION_STR;
}

} /* namespace FrameProcessor */

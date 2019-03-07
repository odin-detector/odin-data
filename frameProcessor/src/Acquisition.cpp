/*
 * Acquisition.cpp
 *
 *  Created on: 31 Oct 2017
 *      Author: vtu42223
 */

#include <boost/filesystem.hpp>
#include <boost/any.hpp>

#include "IFrame.h"
#include "Acquisition.h"
#include "DebugLevelLogger.h"


namespace FrameProcessor {

const std::string META_NAME              = "Acquisition";

const std::string META_FRAME_KEY         = "frame";
const std::string META_OFFSET_KEY        = "offset";
const std::string META_RANK_KEY          = "rank";
const std::string META_NUM_PROCESSES_KEY = "proc";
const std::string META_ACQID_KEY         = "acqID";
const std::string META_NUM_FRAMES_KEY    = "totalFrames";

const std::string META_WRITE_ITEM        = "writeframe";
const std::string META_CREATE_ITEM       = "createfile";
const std::string META_CLOSE_ITEM        = "closefile";
const std::string META_START_ITEM        = "startacquisition";
const std::string META_STOP_ITEM         = "stopacquisition";

Acquisition::Acquisition() :
        concurrent_rank_(0),
        concurrent_processes_(1),
        frames_per_block_(1),
        blocks_per_file_(0),
        frames_written_(0),
        frames_processed_(0),
        total_frames_(0),
        frames_to_write_(0),
        use_earliest_hdf5_(false),
        alignment_threshold_(1),
        alignment_value_(1),
        last_error_("")
{
  this->logger_ = Logger::getLogger("FP.Acquisition");
  this->logger_->setLevel(Level::getTrace());
  LOG4CXX_TRACE(logger_, "Acquisition constructor.");
  connect_meta_channel();
}

Acquisition::~Acquisition() {
}

/**
 * Returns the last error message that was generated
 *
 * \return - The most recently generated error message.
 */
std::string Acquisition::get_last_error()
{
  return last_error_;
}

/**
 * Processes a frame
 *
 * This method checks that the frame is valid before using an HDF5File to write the frame to file
 *
 * \param[in] frame - The frame to process
 * \return - The Status of the processing.
 */
ProcessFrameStatus Acquisition::process_frame(boost::shared_ptr<IFrame> frame) {
  ProcessFrameStatus return_status = status_ok;

  try {
    if (check_frame_valid(frame)) {

      hsize_t frame_no = frame->get_frame_number();
      std::string frame_dataset_name = frame->get_meta_data().get_dataset_name();

      size_t frame_offset = this->adjust_frame_offset(frame);

      if (this->concurrent_processes_ > 1) {
        // Check whether this frame should really be in this process
        if ((frame_offset / frames_per_block_) % this->concurrent_processes_ != this->concurrent_rank_) {
          std::stringstream ss;
          ss << "Unexpected frame: " << frame_no << " in this process rank: " << this->concurrent_rank_;
          last_error_ = ss.str();
          LOG4CXX_ERROR(logger_, last_error_);
          return status_invalid;
        }
      }

      boost::shared_ptr<HDF5File> file = this->get_file(frame_offset);

      if (file == 0) {
        last_error_ = "Unable to get file for this frame";
        LOG4CXX_ERROR(logger_,last_error_);
        return status_invalid;
      }

      size_t frame_offset_in_file = this->get_frame_offset_in_file(frame_offset);

      uint64_t outer_chunk_dimension = 1;
      if (dataset_defs_.size() != 0) {
        outer_chunk_dimension = dataset_defs_.at(frame_dataset_name).chunks[0];
      }

      file->write_frame(*frame, frame_offset_in_file, outer_chunk_dimension);

      // Loops over all parameters, checking if there is a matching dataset and write to it if so
      const std::map<std::string, boost::any> &frame_parameters = frame->get_meta_data().get_parameters();
      std::map<std::string, boost::any>::const_iterator param_iter;
      for (param_iter = frame_parameters.begin(); param_iter != frame_parameters.end(); ++param_iter) {
        std::map<std::string, DatasetDefinition>::iterator dset_iter;
        dset_iter = dataset_defs_.find(param_iter->first);
        if (dset_iter != dataset_defs_.end())
        {
          file->write_parameter(*frame, dset_iter->second, frame_offset_in_file);
        }
      }

      // Send the meta message containing the frame written and the offset written to
      rapidjson::Document document;
      document.SetObject();

      // Add Frame number
      rapidjson::Value key_frame(META_FRAME_KEY.c_str(), document.GetAllocator());
      rapidjson::Value value_frame;
      value_frame.SetUint64(frame_no);
      document.AddMember(key_frame, value_frame, document.GetAllocator());

      // Add offset
      rapidjson::Value key_offset(META_OFFSET_KEY.c_str(), document.GetAllocator());
      rapidjson::Value value_offset;
      value_offset.SetUint64(frame_offset);
      document.AddMember(key_offset, value_offset, document.GetAllocator());

      // Add rank
      rapidjson::Value key_rank(META_RANK_KEY.c_str(), document.GetAllocator());
      rapidjson::Value value_rank;
      value_rank.SetUint64(concurrent_rank_);
      document.AddMember(key_rank, value_rank, document.GetAllocator());

      // Add num consumers
      rapidjson::Value key_num_processes(META_NUM_PROCESSES_KEY.c_str(), document.GetAllocator());
      rapidjson::Value value_num_processes;
      value_num_processes.SetUint64(concurrent_processes_);
      document.AddMember(key_num_processes, value_num_processes, document.GetAllocator());

      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> docWriter(buffer);
      document.Accept(docWriter);

      publish_meta(META_NAME, META_WRITE_ITEM, buffer.GetString(), get_meta_header());

      // Check if this is a master frame (for multi dataset acquisitions)
      // or if no master frame has been defined. If either of these conditions
      // are true then increment the number of frames written.
      if (master_frame_ == "" || master_frame_ == frame_dataset_name) {
        size_t dataset_frames = current_file->get_dataset_frames(frame_dataset_name);
        frames_processed_++;
        LOG4CXX_TRACE(logger_, "Master frame processed");
        size_t current_file_index = current_file->get_file_index() / concurrent_processes_;
        size_t frames_written_to_previous_files = current_file_index * frames_per_block_ * blocks_per_file_;
        size_t total_frames_written = frames_written_to_previous_files + dataset_frames;
        if (total_frames_written == frames_written_) {
          LOG4CXX_TRACE(logger_, "Frame rewritten");
        } else if (total_frames_written > frames_written_) {
          frames_written_ = total_frames_written;
        }
      }
      else {
        LOG4CXX_TRACE(logger_, "Non-master frame processed");
      }

      // If this frame is the final one in the series we are expecting to process, set the return state
      if (frames_to_write_ > 0 && frames_written_ == frames_to_write_) {
        if (frames_processed_ >= frames_to_write_) {
          return_status = status_complete;
        } else {
          LOG4CXX_INFO(logger_, "Number of frames processed (" << frames_processed_ << ") doesn't match expected (" << frames_to_write_ << ")");
          return_status = status_complete_missing_frames;
        }
      }

    } else {
      return_status = status_invalid;
    }

  }
  catch (const std::out_of_range& e) {
    std::stringstream ss;
    ss << "Out of Range exception: " << e.what();
    last_error_ = ss.str();
    LOG4CXX_ERROR(logger_, last_error_);
    return status_invalid;
  }
  catch (const std::range_error& e)
  {
    std::stringstream ss;
    ss << "Range exception: " << e.what();
    last_error_ = ss.str();
    LOG4CXX_ERROR(logger_, last_error_);
    return status_invalid;
  }
  catch (const std::exception& e) {
    std::stringstream ss;
    ss << "Unexpected exception: " << e.what();
    last_error_ = ss.str();
    LOG4CXX_ERROR(logger_, last_error_);
    return status_invalid;
  }
  return return_status;
}

/**
 * Creates a file
 *
 * This method creates a new HDF5File object with the given file_number.
 * The file will be created, the datasets populated within the file, and a meta message sent
 *
 * \param[in] file_number - The file_number to create a file for
 */
void Acquisition::create_file(size_t file_number) {
  // Set previous file to current file, closing off the file for the previous file first
  close_file(previous_file);
  previous_file = current_file;

  current_file = boost::shared_ptr<HDF5File>(new HDF5File());

  // Create the file
  boost::filesystem::path full_path = boost::filesystem::path(file_path_) / boost::filesystem::path(filename_);
  current_file->create_file(full_path.string(), file_number, use_earliest_hdf5_, alignment_threshold_, alignment_value_);

  // Send meta data message to notify of file creation
  publish_meta(META_NAME, META_CREATE_ITEM, full_path.string(), get_create_meta_header());

  // Create the datasets from the definitions
  std::map<std::string, DatasetDefinition>::iterator iter;
  for (iter = dataset_defs_.begin(); iter != dataset_defs_.end(); ++iter) {
    DatasetDefinition dset_def = iter->second;
    dset_def.num_frames = frames_to_write_;

    // Calculate low and high index for this dataset - needed to be able to open datasets in Albula
    int low_index = -1;
    int high_index = -1;

    if (dset_def.create_low_high_indexes && frames_per_block_ > 1)
    {
      low_index = file_number * frames_per_block_ + 1;
      high_index = low_index + frames_per_block_ - 1;
      if (blocks_per_file_ == 0 || high_index > total_frames_)
      {
        high_index = total_frames_;
      }
    }

    validate_dataset_definition(dset_def);
    current_file->create_dataset(dset_def, low_index, high_index);
  }

  current_file->start_swmr();
}

/**
 * Closes a file
 *
 * This method closes the file that is currently being written to by the specified HDF5File object
 * and sends off meta data for this event
 *
 * \param[in] file - The HDF5File to call to close its file
 */
void Acquisition::close_file(boost::shared_ptr<HDF5File> file) {
  if (file != 0) {
    LOG4CXX_INFO(logger_, "Closing file " << file->get_filename());
    file->close_file();
    // Send meta data message to notify of file close
    publish_meta(META_NAME, META_CLOSE_ITEM, file->get_filename(), get_meta_header());
  }
}

/**
 * Validate dataset definition
 *
 * Perform checks on the given dataset definition to ensure it is valid before creation
 *
 * \param[in] definition - The DatasetDefinition to validate
 */
void Acquisition::validate_dataset_definition(DatasetDefinition definition) {
  // Check image dimensions
  std::vector<long long unsigned int>::iterator iter;
  for (iter = definition.frame_dimensions.begin(); iter != definition.frame_dimensions.end(); ++iter) {
    if (*iter == 0) {
      throw std::runtime_error("Image dimensions must be non-zero");
    }
  }
  // Check chunk dimensions
  for (iter = definition.chunks.begin(); iter != definition.chunks.end(); ++iter) {
    if (*iter == 0) {
      throw std::runtime_error("Chunk dimensions must be non-zero");
    }
  }
}

/**
 * Starts this acquisition, creating the acquisition file, or first file in a series, and publishes meta
 *
 * \param[in] concurrent_rank - The rank of the processor
 * \param[in] concurrent_processes - The number of processors
 * \param[in] frames_per_block - The number of frames per block
 * \param[in] blocks_per_file - The number of blocks per file
 * \param[in] file_extension - The file extension to use
 * \param[in] use_earliest_hdf5 - Whether to use an early version of hdf5 library
 * \param[in] alignment_threshold - Alignment threshold for hdf5 chunking
 * \param[in] alignment_value - Alignment value for hdf5 chunking
 * \param[in] master_frame - The master frame dataset name
 * \return - true if the acquisition was started successfully
 */
bool Acquisition::start_acquisition(
    size_t concurrent_rank,
    size_t concurrent_processes,
    size_t frames_per_block,
    size_t blocks_per_file,
    std::string file_extension,
    bool use_earliest_hdf5,
    size_t alignment_threshold,
    size_t alignment_value,
    std::string master_frame) {

  concurrent_rank_ = concurrent_rank;
  concurrent_processes_ = concurrent_processes;
  frames_per_block_ = frames_per_block;
  blocks_per_file_ = blocks_per_file;
  use_earliest_hdf5_ = use_earliest_hdf5;
  alignment_threshold_ = alignment_threshold;
  alignment_value_ = alignment_value;
  file_extension_ = file_extension;
  master_frame_ = master_frame;

  // Sanitise the file extension, ensuring there is a . at the start if the extension is not empty
  if (!file_extension_.empty())
  {
    if (file_extension_.at(0) != '.')
    {
      file_extension_.insert(0, ".");
    }
  }

  // Generate the filename
  filename_ = generate_filename(concurrent_rank_);

  if (filename_.empty()) {
    last_error_ = "Unable to start writing - no filename to write to";
    LOG4CXX_ERROR(logger_, last_error_);
    return false;
  }

  publish_meta(META_NAME, META_START_ITEM, "", get_create_meta_header());

  create_file(concurrent_rank_);

  return true;
}

/**
 * Stops this acquisition, closing off any open files
 */
void Acquisition::stop_acquisition() {
  close_file(previous_file);
  close_file(current_file);
  publish_meta(META_NAME, META_STOP_ITEM, "", get_meta_header());
}

/** Check incoming frame is valid for its target dataset.
 *
 * Check the dimensions, data type and compression of the frame data.
 *
 * \param[in] frame - Pointer to the Frame object.
 * \return - true if the frame was valid
 */
bool Acquisition::check_frame_valid(boost::shared_ptr<IFrame> frame)
{
  bool invalid = false;
  const IFrameMetaData frame_meta_data = frame->get_meta_data();
  DatasetDefinition dataset = dataset_defs_.at(frame_meta_data.get_dataset_name());
  CompressionType frame_compression_type = frame_meta_data.get_compression_type();
  if (frame_compression_type >= 0 && frame_compression_type != dataset.compression) {
    std::stringstream ss;
    ss << "Invalid frame: Frame has compression " << frame_compression_type <<
          ", expected " << dataset.compression <<
          " for dataset " << dataset.name <<
          " (0: None, 1: LZ4, 2: BSLZ4, 3: Blosc)";
    last_error_ = ss.str();
    LOG4CXX_ERROR(logger_, last_error_);
    invalid = true;
  }
  DataType frame_data_type = frame_meta_data.get_data_type();
  if (frame_data_type >= 0 && frame_data_type != dataset.data_type) {
    std::stringstream ss;
    ss << "Invalid frame: Frame has data type " << frame_data_type <<
       ", expected " << dataset.data_type <<
       " for dataset " << dataset.name <<
       " (0: UINT8, 1: UINT16, 2: UINT32, 3: UINT64, 4: FLOAT)";
    last_error_ = ss.str();
    LOG4CXX_ERROR(logger_, last_error_);
    invalid = true;
  }
  dimensions_t frame_dimensions = frame_meta_data.get_dimensions();
  if (frame_dimensions != dataset.frame_dimensions) {
    if (frame_dimensions.size() >= 2 && dataset.frame_dimensions.size() >= 2) {
      std::stringstream ss;
      ss << "Invalid frame: Frame has dimensions [" << frame_dimensions[0] << ", " << frame_dimensions[1] <<
         "], expected [" << dataset.frame_dimensions[0] << ", " << dataset.frame_dimensions[1] <<
         "] for dataset " << dataset.name;
      last_error_ = ss.str();
      LOG4CXX_ERROR(logger_, last_error_);
    } else if (frame_dimensions.size() >= 1 && dataset.frame_dimensions.size() >= 1) {
      std::stringstream ss;
      ss << "Invalid frame: Frame has dimensions [" << frame_dimensions[0]  <<
         "], expected [" << dataset.frame_dimensions[0] << "] for dataset " << dataset.name;
      last_error_ = ss.str();
      LOG4CXX_ERROR(logger_, last_error_);
    } else {
      last_error_ = "Invalid frame: Frame dimensions do not match";
      LOG4CXX_ERROR(logger_, last_error_);
    }
    invalid = true;
  }
  return !invalid;
}

/**
 * Return the dataset offset for the supplied global offset
 *
 * This method calculates the dataset offset for this frame, within
 * the final file that it will be written to
 *
 * \param[in] frame_offset - Frame number of the frame.
 * \return - the dataset offset for the frame number.
 */
size_t Acquisition::get_frame_offset_in_file(size_t frame_offset) const {
  // Calculate the new offset based on how many concurrent processes are running
  size_t block_index = frame_offset / (frames_per_block_ * this->concurrent_processes_);
  size_t first_frame_offset_of_block = block_index * frames_per_block_;
  if (blocks_per_file_ != 0) {
    first_frame_offset_of_block = first_frame_offset_of_block % (blocks_per_file_ * frames_per_block_);
  }
  size_t offset_within_block = frame_offset % frames_per_block_;
  frame_offset = first_frame_offset_of_block + offset_within_block;

  return frame_offset;
}

/**
 * Return the file index for the supplied global offset
 *
 * This method calculates the file index that this frame should be written to
 *
 * \param[in] frame_offset - Frame number of the frame.
 * \return - the file index.
 */
size_t Acquisition::get_file_index(size_t frame_offset) const {
  size_t block_number = frame_offset / frames_per_block_;
  size_t block_row = block_number / concurrent_processes_;
  size_t file_row = block_row / blocks_per_file_;
  size_t file_index = (file_row * concurrent_processes_) + concurrent_rank_;
  return file_index;
}

/**
 * Gets the HDF5File object for the given frame
 *
 * This will depending on variables like the number of frames per block and blocks per file.
 * If the required HDF5File doesn't currently exist, one will be created.
 * If it's detected that there are frames which have been missed which would have required
 * a file before this one, those files are created and their files written with blank data
 *
 * \param[in] frame_offset - The frame offset to get the file for
 * \return - The file that should be used to write this frame
 */
boost::shared_ptr<HDF5File> Acquisition::get_file(size_t frame_offset) {
  if (blocks_per_file_ == 0) {
    return this->current_file;
  }

  // Get the file index this frame should go into
  size_t file_index = get_file_index(frame_offset);

  // Get the file for this frame index
  if (file_index == current_file->get_file_index()) {
    return this->current_file;
  } else if (previous_file != 0 && file_index == previous_file->get_file_index()) {
    return this->previous_file;
  } else if (file_index > current_file->get_file_index()) {
    LOG4CXX_TRACE(logger_,"Creating new file as frame " << frame_offset <<
        " won't go into file index " << current_file->get_file_index() << " as it requires " << file_index);

    // Check for missing files and create them if they have been missed
    size_t next_expected_file_index = current_file->get_file_index() + concurrent_processes_;
    while (next_expected_file_index < file_index) {
      LOG4CXX_DEBUG_LEVEL(1, logger_,"Creating missing file " << next_expected_file_index);
      filename_ = generate_filename(next_expected_file_index);
      create_file(next_expected_file_index);
      next_expected_file_index = current_file->get_file_index() + concurrent_processes_;
    }

    filename_ = generate_filename(file_index);

    create_file(file_index);

    return this->current_file;
  } else {
    LOG4CXX_WARN(logger_,"Unable to write frame offset " << frame_offset << " as no suitable file found");
    return boost::shared_ptr<HDF5File>();
  }

}

/** Returns the adjusted offset (index in file) for the Frame
 *
 * Combines the frame number with the frame offset stored on the frame
 * object to calculate an the adjusted frame offset in the file for this
 * frame
 *
 * Throws a std::range_error if the applied offset would cause the frame
 * offset to be calculated as a negative number
 *
 * Returns the dataset offset for frame number (frame_no)
 */
size_t Acquisition::adjust_frame_offset(boost::shared_ptr<IFrame> frame) const {
  size_t frame_no = frame->get_frame_number();
  int64_t frame_offset_adjustment = frame->get_meta_data().get_frame_offset();
  size_t frame_offset = 0;

  LOG4CXX_DEBUG_LEVEL(2, logger_, "Raw frame number: " << frame_no << ", Frame offset adjustment: " << frame_offset_adjustment);

  if ((int) frame_no + frame_offset_adjustment < 0 ) {
    // Throw a range error if the offset would cause the adjusted offset to be negative
    throw std::range_error("Frame offset causes negative file offset");
  }

  frame_offset = frame_no + frame_offset_adjustment;
  LOG4CXX_DEBUG_LEVEL(2, logger_, "Adjusted frame offset: " << frame_offset);
  return frame_offset;
}

/** Creates and returns the Meta Header json string to be sent out over the meta data channel when a file is created
 *
 * This will typically include details about the current acquisition (e.g. the ID)
 *
 * \return - a string containing the json meta data header
 */
std::string Acquisition::get_create_meta_header() {
  rapidjson::Document meta_document;
  meta_document.SetObject();

  // Add Acquisition ID
  rapidjson::Value key_acq_id(META_ACQID_KEY.c_str(), meta_document.GetAllocator());
  rapidjson::Value value_acq_id;
  value_acq_id.SetString(acquisition_id_.c_str(), acquisition_id_.length(), meta_document.GetAllocator());
  meta_document.AddMember(key_acq_id, value_acq_id, meta_document.GetAllocator());

  // Add Number of Frames
  rapidjson::Value key_num_frames(META_NUM_FRAMES_KEY.c_str(), meta_document.GetAllocator());
  rapidjson::Value value_num_frames;
  value_num_frames.SetUint64(total_frames_);
  meta_document.AddMember(key_num_frames, value_num_frames, meta_document.GetAllocator());

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  meta_document.Accept(writer);

  return buffer.GetString();
}

/** Creates and returns the Meta Header json string to be sent out over the meta data channel
 *
 * This will typically include details about the current acquisition (e.g. the ID)
 *
 * \return - a string containing the json meta data header
 */
std::string Acquisition::get_meta_header() {
  rapidjson::Document meta_document;
  meta_document.SetObject();

  // Add Acquisition ID
  rapidjson::Value key_acq_id(META_ACQID_KEY.c_str(), meta_document.GetAllocator());
  rapidjson::Value value_acq_id;
  value_acq_id.SetString(acquisition_id_.c_str(), acquisition_id_.length(), meta_document.GetAllocator());
  meta_document.AddMember(key_acq_id, value_acq_id, meta_document.GetAllocator());

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  meta_document.Accept(writer);

  return buffer.GetString();
}

/**
 * Generates the filename for the given file number
 *
 * Appends a 6 digit file number to the configured file name.
 * File numbers are 0 indexed, but filenames are 1 indexed
 *
 * If no file name is configured, it uses the acquisition ID and if this
 * is not configured, then the generated filename returned is empty.
 *
 * \param[in] file_number - The file number to generate the filename for
 * \return - The name of the file including extension
 */
std::string Acquisition::generate_filename(size_t file_number) {

  std::stringstream generated_filename;

  char number_string[7];
  snprintf(number_string, 7, "%06d", file_number + 1);
  if (!configured_filename_.empty())
  {
    generated_filename << configured_filename_ << "_" << number_string << file_extension_;
  }
  else if (!acquisition_id_.empty())
  {
    generated_filename << acquisition_id_ << "_" << number_string << file_extension_;
  }

  return generated_filename.str();
}

} /* namespace FrameProcessor */

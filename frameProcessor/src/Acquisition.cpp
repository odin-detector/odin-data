/*
 * Acquisition.cpp
 *
 *  Created on: 31 Oct 2017
 *      Author: vtu42223
 */

#include <boost/filesystem.hpp>

#include "Frame.h"
#include "Acquisition.h"


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
        frame_offset_adjustment_(0),
        frames_per_block_(1),
        blocks_per_file_(0),
        frames_written_(0),
        frames_processed_(0),
        total_frames_(0),
        frames_to_write_(0),
        use_earliest_hdf5_(false),
        alignment_threshold_(1),
        alignment_value_(1)
{
  this->logger_ = Logger::getLogger("FP.Acquisition");
  this->logger_->setLevel(Level::getTrace());
  LOG4CXX_TRACE(logger_, "Acquisition constructor.");
  connect_meta_channel();
}

Acquisition::~Acquisition() {
}

/**
 * Processes a frame
 *
 * This method checks that the frame is valid before using an HDF5File to write the frame to file
 *
 * \param[in] frame - The frame to process
 * \return - The Status of the processing.
 */
ProcessFrameStatus Acquisition::process_frame(boost::shared_ptr<Frame> frame) {
  ProcessFrameStatus return_status = status_ok;

  try {
    if (check_frame_valid(frame)) {

      hsize_t frame_no = frame->get_frame_number();

      size_t frame_offset = this->adjust_frame_offset(frame_no);

      if (this->concurrent_processes_ > 1) {
        // Check whether this frame should really be in this process
        if ((frame_offset / frames_per_block_) % this->concurrent_processes_ != this->concurrent_rank_) {
          LOG4CXX_ERROR(logger_,"Unexpected frame: " << frame_no << " in this process rank: " << this->concurrent_rank_);
          return status_invalid;
        }
      }

      boost::shared_ptr<HDF5File> file = this->get_file(frame_offset);

      if (file == 0) {
        LOG4CXX_ERROR(logger_,"Unable to get file for this frame");
        return status_invalid;
      }

      size_t frame_offset_in_file = this->get_frame_offset_in_file(frame_offset);

      uint64_t outer_chunk_dimension = 1;
      if (dataset_defs_.size() != 0) {
        outer_chunk_dimension = dataset_defs_.at(frame->get_dataset_name()).chunks[0];
      }

      file->write_frame(*frame, frame_offset_in_file, outer_chunk_dimension);

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
      if (master_frame_ == "" || master_frame_ == frame->get_dataset_name()) {
        size_t dataset_frames = file->get_dataset_frames(frame->get_dataset_name());
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
        if (frames_processed_ == frames_to_write_) {
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
    LOG4CXX_ERROR(logger_, "Out of Range exception: " << e.what());
    return status_invalid;
  }
  catch (const std::range_error& e)
  {
    LOG4CXX_ERROR(logger_, "Range exception: " << e.what());
    return status_invalid;
  }
  catch (const std::exception& e) {
    LOG4CXX_ERROR(logger_, "Unexpected exception: " << e.what());
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
    current_file->create_dataset(dset_def);
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
 * Starts this acquisition, creating the acquisition file, or first file in a series, and publishes meta
 *
 * \param[in] concurrent_rank - The rank of the processor
 * \param[in] concurrent_processes - The number of processors
 * \param[in] frame_offset_adjustment - The starting frame offset adjustment
 * \param[in] frames_per_block - The number of frames per block
 * \param[in] blocks_per_file - The number of blocks per file
 */
void Acquisition::start_acquisition(
    size_t concurrent_rank,
    size_t concurrent_processes,
    size_t frame_offset_adjustment,
    size_t frames_per_block,
    size_t blocks_per_file,
    bool use_earliest_hdf5,
    size_t alignment_threshold,
    size_t alignment_value) {

  concurrent_rank_ = concurrent_rank;
  concurrent_processes_ = concurrent_processes;
  frame_offset_adjustment_ = frame_offset_adjustment;
  frames_per_block_ = frames_per_block;
  blocks_per_file_ = blocks_per_file;
  use_earliest_hdf5_ = use_earliest_hdf5;
  alignment_threshold_ = alignment_threshold;
  alignment_value_ = alignment_value;

  // If filename hasn't been explicitly specified or we are in multi-file mode, generate the filename
  if ((filename_.empty() && !acquisition_id_.empty()) || blocks_per_file_ != 0) {
    filename_ = generate_filename(concurrent_rank_);
  }

  if (filename_.empty()) {
    LOG4CXX_ERROR(logger_, "Unable to start writing - no filename to write to");
    return;
  }

  publish_meta(META_NAME, META_START_ITEM, "", get_create_meta_header());

  create_file(concurrent_rank_);
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
bool Acquisition::check_frame_valid(boost::shared_ptr<Frame> frame)
{
  bool invalid = false;
  DatasetDefinition dataset = dataset_defs_.at(frame->get_dataset_name());
  if (frame->get_compression() >= 0 && frame->get_compression() != dataset.compression) {
    LOG4CXX_ERROR(logger_, "Invalid frame: Frame has compression " << frame->get_compression() <<
        ", expected " << dataset.compression <<
        " for dataset " << dataset.name <<
        " (0: None, 1: LZ4, 2: BSLZ4)");
    invalid = true;
  }
  if (frame->get_data_type() >= 0 && frame->get_data_type() != dataset.pixel) {
    LOG4CXX_ERROR(logger_, "Invalid frame: Frame has data type " << frame->get_data_type() <<
        ", expected " << dataset.pixel <<
        " for dataset " << dataset.name <<
        " (0: UINT8, 1: UINT16, 2: UINT32, 3: UINT64)");
    invalid = true;
  }
  if (frame->get_dimensions(frame->get_dataset_name()) != dataset.frame_dimensions) {
    std::vector<unsigned long long> dimensions = frame->get_dimensions(frame->get_dataset_name());
    if (dimensions.size() >= 2 && dataset.frame_dimensions.size() >= 2) {
      LOG4CXX_ERROR(logger_, "Invalid frame: Frame has dimensions [" << dimensions[0] << ", " << dimensions[1] <<
          "], expected [" << dataset.frame_dimensions[0] << ", " << dataset.frame_dimensions[1] <<
          "] for dataset " << dataset.name);
    } else if (dimensions.size() >= 1 && dataset.frame_dimensions.size() >= 1) {
      LOG4CXX_ERROR(logger_, "Invalid frame: Frame has dimensions [" << dimensions[0]  <<
          "], expected [" << dataset.frame_dimensions[0] << "] for dataset " << dataset.name);
    } else {
      LOG4CXX_ERROR(logger_, "Invalid frame: Frame dimensions do not match");
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
      LOG4CXX_DEBUG(logger_,"Creating missing file " << next_expected_file_index);
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
size_t Acquisition::adjust_frame_offset(size_t frame_no) const {
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
 * If there is only one file (blocks per file is 0), then the rank is used.
 *
 * If there are multiple files (blocks per file is bigger than 0), then
 * the file number is used. File numbers are 0 indexed, but filenames are 1 indexed
 *
 * \param[in] file_number - The file number to generate the filename for
 * \return - The name of the file including extension
 */
std::string Acquisition::generate_filename(size_t file_number) {

  std::stringstream generated_filename;

  if (blocks_per_file_ == 0) {
    generated_filename << acquisition_id_ << "_r" << concurrent_rank_ << ".h5";
  } else {
    char number_string[7];
    snprintf(number_string, 7, "%06d", file_number + 1);
    generated_filename << acquisition_id_ << "_data_" << number_string << ".h5";
  }

  return generated_filename.str();
}

} /* namespace FrameProcessor */

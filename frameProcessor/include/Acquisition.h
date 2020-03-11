/*
 * Acquisition.h
 *
 *  Created on: 31 Oct 2017
 *      Author: vtu42223
 */

#ifndef FRAMEPROCESSOR_SRC_ACQUISITION_H_
#define FRAMEPROCESSOR_SRC_ACQUISITION_H_

#include <string>
#include <vector>
#include <map>


#include <boost/shared_ptr.hpp>

#include <log4cxx/logger.h>
using namespace log4cxx;

#include "FrameProcessorDefinitions.h"
#include "HDF5File.h"
#include "MetaMessagePublisher.h"

namespace FrameProcessor {

class Frame;

class Acquisition : public MetaMessagePublisher
{
public:
  Acquisition(const HDF5ErrorDefinition_t& hdf5_error_definition);
  ~Acquisition();
  std::string get_last_error();
  ProcessFrameStatus process_frame(boost::shared_ptr<Frame> frame, HDF5CallDurations_t& call_durations);
  void create_file(size_t file_number, HDF5CallDurations_t& call_durations);
  void close_file(boost::shared_ptr<HDF5File> file, HDF5CallDurations_t& call_durations);
  void validate_dataset_definition(DatasetDefinition definition);
  bool start_acquisition(
    size_t concurrent_rank,
    size_t concurrent_processes,
    size_t frames_per_block,
    size_t blocks_per_file,
    std::string file_extension,
    bool use_earliest_hdf5,
    size_t alignment_threshold,
    size_t alignment_value,
    std::string master_frame,
    HDF5CallDurations_t& call_durations
  );
  void stop_acquisition(HDF5CallDurations_t& call_durations);
  bool check_frame_valid(boost::shared_ptr<Frame> frame);
  size_t get_frame_offset_in_file(size_t frame_offset) const;
  size_t get_file_index(size_t frame_offset) const;
  size_t adjust_frame_offset(boost::shared_ptr<Frame> frame) const;
  boost::shared_ptr<HDF5File> get_file(size_t frame_offset, HDF5CallDurations_t& call_durations);
  std::string get_create_meta_header();
  std::string get_meta_header();
  std::string generate_filename(size_t file_number=0);

  LoggerPtr logger_;
  /** Name of master frame. When a master frame is received frame numbers increment */
  std::string master_frame_;
  /** Number of frames to write to file */
  size_t frames_to_write_;
  /** Total number of frames in acquisition */
  size_t total_frames_;
  /** Path of the file to write to */
  std::string file_path_;
  /** Name of the file to write to */
  std::string filename_;
  /** Configured value to be used as the prefix to generate the filename. */
  std::string configured_filename_;
  /** File extension to use */
  std::string file_extension_;
  /** Use the earliest version of hdf5 */
  bool use_earliest_hdf5_;
  /** HDF5 file chunk alignment threshold */
  size_t alignment_threshold_;
  /** HDF5 file chunk alignment value */
  size_t alignment_value_;
  /** Identifier for the acquisition - value sent from a detector/control to be used to
   * identify frames, config or anything else to this acquisition. Used to name the file */
  std::string acquisition_id_;
  /** Map of dataset definitions for this acquisition */
  std::map<std::string, DatasetDefinition> dataset_defs_;
  /** Number of frames that have been written to file */
  size_t frames_written_;
  /** Number of frames that have been processed */
  size_t frames_processed_;
  /** Number of concurrent file writers executing */
  size_t concurrent_processes_;
  /** Rank of this file writer */
  size_t concurrent_rank_;
  /** Number of frames to write consecutively in a file */
  size_t frames_per_block_;
  /** Number of blocks to write in a file; 0=put all blocks in same file  */
  size_t blocks_per_file_;
  /** HDF5 call error definitions */
  const HDF5ErrorDefinition_t& hdf5_error_definition_;

private:
  void add_uint64_to_document(const std::string& key, size_t value, rapidjson::Document* document) const;
  void add_string_to_document(const std::string& key, const std::string& value, rapidjson::Document* document) const;
  std::string document_to_string(rapidjson::Document& document) const;

  /** The current file that frames are being written to */
  boost::shared_ptr<HDF5File> current_file_;
  /** The previous file that frames were written to, held in case of late frames  */
  boost::shared_ptr<HDF5File> previous_file_;
  /** Most recently generated error message */
  std::string last_error_;
};

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_SRC_ACQUISITION_H_ */

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
#include "HDF5FileWriter.h"
#include "MetaMessagePublisher.h"

namespace FrameProcessor {

class Frame;

class Acquisition : public MetaMessagePublisher
{
public:
  Acquisition();
  ~Acquisition();
  ProcessFrameStatus process_frame(boost::shared_ptr<Frame> frame);
  void create_file(size_t file_number=0);
  void close_file(boost::shared_ptr<HDF5FileWriter> writer);
  void start_acquisition(size_t concurrent_rank, size_t concurrent_processes, size_t frame_offset_adjustment, size_t frames_per_block, size_t blocks_per_file);
  void stop_acquisition();
  bool check_frame_valid(boost::shared_ptr<Frame> frame);
  size_t get_frame_offset_in_file(size_t frame_offset) const;
  size_t adjust_frame_offset(size_t frame_no) const;
  boost::shared_ptr<HDF5FileWriter> get_file_writer(size_t frame_offset);
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
  /** Offset between raw frame ID and position in dataset */
  size_t frame_offset_adjustment_;
  /** Number of frames to write consecutively in a file */
  size_t frames_per_block_;
  /** Number of blocks to write in a file  */
  size_t blocks_per_file_;
private:
  boost::shared_ptr<HDF5FileWriter> file_writer;
  boost::shared_ptr<HDF5FileWriter> previous_file_writer;
};

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_SRC_ACQUISITION_H_ */

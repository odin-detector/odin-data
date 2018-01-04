/*
 * FileWriterPlugin.h
 *
 */

#ifndef TOOLS_FileWriterPlugin_FileWriterPlugin_H_
#define TOOLS_FileWriterPlugin_FileWriterPlugin_H_

#include <string>
#include <vector>
#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <log4cxx/logger.h>
using namespace log4cxx;

#include "FrameProcessorPlugin.h"
#include "Acquisition.h"
#include "ClassLoader.h"

namespace FrameProcessor
{

class Frame;

/** Plugin that writes Frame objects to HDF5 files.
 *
 * This plugin processes Frame objects and can write them to HDF5 files.
 * The plugin can be configured through the Ipc control interface defined
 * in the FileWriterPluginController class. Currently only the raw data is written
 * into datasets. Multiple datasets can be created and the raw data is stored
 * according to the Frame index (or name).
 */
class FileWriterPlugin : public FrameProcessorPlugin
{
public:

  explicit FileWriterPlugin();
  virtual ~FileWriterPlugin();

  void set_frame_offset_adjustment(size_t frame_no);

  void start_writing();
  void stop_writing();
  void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
  void requestConfiguration(OdinData::IpcMessage& reply);
  void configure_process(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
  void configure_file(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
  void configure_dataset(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
  void status(OdinData::IpcMessage& status);
  void stop_acquisition();
  void start_close_file_timeout();
  void run_close_file_timeout();
  size_t calc_num_frames(size_t total_frames);

private:
  /** Configuration constant for process related items */
  static const std::string CONFIG_PROCESS;
  /** Configuration constant for number of processes */
  static const std::string CONFIG_PROCESS_NUMBER;
  /** Configuration constant for this process rank */
  static const std::string CONFIG_PROCESS_RANK;
  /** Configuration constant for the number of frames per block */
  static const std::string CONFIG_PROCESS_BLOCKSIZE;
  /** Configuration constant for the number of blocks per file */
  static const std::string CONFIG_PROCESS_BLOCKS_PER_FILE;
  /** Configuration constant for using earliest file version */
  static const std::string CONFIG_PROCESS_EARLIEST_VERSION;
  /** Configuration constant for chunk alignment threshold */
  static const std::string CONFIG_PROCESS_ALIGNMENT_THRESHOLD;
  /** Configuration constant for chunk alignment value */
  static const std::string CONFIG_PROCESS_ALIGNMENT_VALUE;

  /** Configuration constant for file related items */
  static const std::string CONFIG_FILE;
  /** Configuration constant for file name */
  static const std::string CONFIG_FILE_NAME;
  /** Configuration constant for file path */
  static const std::string CONFIG_FILE_PATH;

  /** Configuration constant for dataset related items */
  static const std::string CONFIG_DATASET;
  /** Configuration constant for dataset command */
  static const std::string CONFIG_DATASET_CMD;
  /** Configuration constant for dataset name */
  static const std::string CONFIG_DATASET_NAME;
  /** Configuration constant for dataset datatype */
  static const std::string CONFIG_DATASET_TYPE;
  /** Configuration constant for dataset dimensions */
  static const std::string CONFIG_DATASET_DIMS;
  /** Configuration constant for chunking dimensions */
  static const std::string CONFIG_DATASET_CHUNKS;
  /** Configuration constant for data compression */
  static const std::string CONFIG_DATASET_COMPRESSION;

  /** Configuration constant for number of frames to write */
  static const std::string CONFIG_FRAMES;
  /** Configuration constant for master dataset name */
  static const std::string CONFIG_MASTER_DATASET;
  /** Configuration constant for starting and stopping writing of frames */
  static const std::string CONFIG_WRITE;
  /** Configuration constant for the frame offset */
  static const std::string CONFIG_OFFSET_ADJUSTMENT;
  /** Configuration constant for the acquisition id */
  static const std::string ACQUISITION_ID;
  /** Configuration constant for the close file timeout */
  static const std::string CLOSE_TIMEOUT_PERIOD;
  /** Configuration constant for starting the close file timeout */
  static const std::string START_CLOSE_TIMEOUT;

  /**
   * Prevent a copy of the FileWriterPlugin plugin.
   *
   * \param[in] src
   */
  FileWriterPlugin(const FileWriterPlugin& src); // prevent copying one of these

  void process_frame(boost::shared_ptr<Frame> frame);
  bool frame_in_acquisition(boost::shared_ptr<Frame> frame);

  /** Pointer to logger */
  LoggerPtr logger_;
  /** Mutex used to make this class thread safe */
  boost::recursive_mutex mutex_;
  /** Is this plugin writing frames to file? */
  bool writing_;
  /** Number of concurrent file writers executing */
  size_t concurrent_processes_;
  /** Rank of this file writer */
  size_t concurrent_rank_;
  /** Offset between raw frame ID and position in dataset */
  size_t frame_offset_adjustment_;
  /** Details of the acquisition currently being written */
  boost::shared_ptr<Acquisition> current_acquisition_;
  /** Details of the next acquisition to be written */
  boost::shared_ptr<Acquisition> next_acquisition_;
  /** Number of frames to write consecutively in a file */
  size_t frames_per_block_;
  /** Number of blocks to write in a file  */
  size_t blocks_per_file_;
  /** Use the earliest version of hdf5 */
  bool use_earliest_hdf5_;
  /** HDF5 file chunk alignment threshold */
  size_t alignment_threshold_;
  /** HDF5 file chunk alignment value */
  size_t alignment_value_;
  /** Timeout for closing the file after receiving no data */
  size_t timeout_period_;
  /** Mutex used to make starting the close file timeout thread safe */
  boost::mutex start_timeout_mutex_;
  /** Mutex used to make running the close file timeout thread safe */
  boost::mutex close_file_mutex_;
  /** Condition variable used to start the close file timeout */
  boost::condition_variable start_condition_;
  /** Condition variable used to run the close file timeout */
  boost::condition_variable timeout_condition_;
  /** Close file timeout active switch */
  bool timeout_active_;
  /** Close file timeout thread running */
  bool timeout_thread_running_;
  /** The close file timeout thread */
  boost::thread timeout_thread_;
};

} /* namespace FrameProcessor */

#endif /* TOOLS_FileWriterPlugin_FileWriterPlugin_H_ */

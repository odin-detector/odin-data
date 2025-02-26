/*
 * HDF5File.h
 *
 *  Created on: 31 Oct 2017
 *      Author: vtu42223
 */

#ifndef FRAMEPROCESSOR_SRC_HDF5FILE_H_
#define FRAMEPROCESSOR_SRC_HDF5FILE_H_


#include <string>
#include <vector>
#include <map>

#include <log4cxx/logger.h>
using namespace log4cxx;

#include <hdf5.h>
#include "H5Zpublic.h"

#include "Frame.h"
#include "FrameProcessorDefinitions.h"
#include "MetaMessagePublisher.h"
#include "WatchdogTimer.h"
#include "CallDuration.h"

namespace FrameProcessor {

/**
 * A collection of CallDurations to pass around and update with HDF5 call metrics
 */
struct HDF5CallDurations_t
{
  CallDuration create;
  CallDuration write;
  CallDuration flush;
  CallDuration close;
};

/**
 * Definitions of what constitutes an error from the HDF5 library
 *
 * - Durations (milliseconds) of HDF5 calls over which an error message will be logged
 */
struct HDF5ErrorDefinition_t
{
  unsigned int create_duration;
  unsigned int write_duration;
  unsigned int flush_duration;
  unsigned int close_duration;
  std::function<void(const std::string&)> callback;
};

class HDF5File
{
public:

  /**
   * Struct to keep track of an HDF5 dataset handle and dimensions.
   */
  struct HDF5Dataset_t
  {
    /** Handle of the dataset **/
    hid_t dataset_id;
    /** Array of dimensions of the dataset **/
    std::vector<hsize_t> dataset_dimensions;
    /** Array of offsets of the dataset **/
    std::vector<hsize_t> dataset_offsets;
    /** Extent of the (outermost dimension of the) dataset that has had frames written to, including any gaps
     * i.e. the highest offset that has been written to + 1 */
    size_t actual_dataset_size_;
  };

  HDF5File(const HDF5ErrorDefinition_t& hdf5_error_definition);
  ~HDF5File();
  void hdf_error_handler(unsigned n, const H5E_error2_t* err_desc);
  void clear_hdf_errors();
  void handle_h5_error(const std::string& message, const std::string& function, const std::string& filename, int line);
  size_t create_file(std::string file_name, size_t file_index, bool use_earliest_version, size_t alignment_threshold, size_t alignment_value);
  size_t close_file();
  void create_dataset(const DatasetDefinition& definition, int low_index, int high_index);
  void write_frame(
    const Frame& frame,
    hsize_t frame_offset,
    uint64_t outer_chunk_dimension,
    HDF5CallDurations_t& call_durations
  );
  void write_parameter(const Frame& frame, DatasetDefinition dataset_definition, hsize_t frame_offset);
  size_t get_dataset_frames(const std::string& dset_name);
  size_t get_dataset_max_size(const std::string& dset_name);
  void start_swmr();
  size_t get_file_index();
  std::string get_filename();
  void set_unlimited();

private:

  /** Filter definition to write datasets with LZ4 compressed data */
  static const H5Z_filter_t LZ4_FILTER = (H5Z_filter_t)32004;
  /** Filter definition to write datasets with bitshuffle processed data */
  static const H5Z_filter_t BSLZ4_FILTER = (H5Z_filter_t)32008;
  /** Filter definition to write datasets with Blosc processed data */
  static const H5Z_filter_t BLOSC_FILTER = (H5Z_filter_t)32001;

  /** Flush rate for parameter datasets in miliseconds */
  static const int PARAM_FLUSH_RATE = 1000;

  HDF5Dataset_t& get_hdf5_dataset(const std::string& dset_name);
  void extend_dataset(HDF5File::HDF5Dataset_t& dset, size_t frame_no);
  hid_t datatype_to_hdf_type(DataType data_type) const;

  LoggerPtr logger_;
  /** Internal ID of the file being written to */
  hid_t hdf5_file_id_;
  /** Internal HDF5 error flag */
  bool hdf5_error_flag_;
  /** Internal HDF5 error recording */
  std::vector<H5E_error2_t> hdf5_errors_;
  /** Map of datasets that are being written to */
  std::map<std::string, HDF5Dataset_t> hdf5_datasets_;
  /** The index of this file across all processors in the acquisition, 0 indexed */
  size_t file_index_;
  /** Full name and path of the file to write to */
  std::string filename_;
  /** Whether to use the earliest version of the hdf5 library */
  bool use_earliest_version_;
  /** Whether datasets use H5S_UNLIMITED as the outermost dimension extent */
  bool unlimited_;
  /** Mutex used to make this class thread safe */
  std::recursive_mutex mutex_;
  /* Parameters memspace */
  hid_t param_memspace_;
  /* Map containing time each dataset was last flushed*/
  std::map<std::string, boost::posix_time::ptime> last_flushed;
  /* Watchdog timer for monitoring function call durations */
  WatchdogTimer watchdog_timer_;
  /** HDF5 call error definitions */
  const HDF5ErrorDefinition_t& hdf5_error_definition_;
};

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_SRC_HDF5FILE_H_ */

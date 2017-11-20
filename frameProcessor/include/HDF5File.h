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

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <log4cxx/logger.h>
using namespace log4cxx;

#include <hdf5.h>
#include "H5Zpublic.h"

#include "Frame.h"
#include "FrameProcessorDefinitions.h"
#include "MetaMessagePublisher.h"

namespace FrameProcessor {

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
  };


  HDF5File();
  ~HDF5File();
  void hdf_error_handler(unsigned n, const H5E_error2_t *err_desc);
  bool check_for_hdf_errors();
  std::vector<std::string> read_hdf_errors();
  void clear_hdf_errors();
  void handle_h5_error(std::string message, std::string function, std::string filename, int line) const;
  void create_file(std::string file_name, size_t file_index, size_t chunk_align=1024 * 1024);
  void close_file();
  void create_dataset(const DatasetDefinition& definition);
  void write_frame(const Frame& frame, hsize_t frame_offset, uint64_t outer_chunk_dimension);
  size_t get_dataset_frames(const std::string dset_name);
  void start_swmr();
  size_t get_file_index();
  std::string get_filename();

private:

  /** Filter definition to write datasets with LZ4 compressed data */
  static const H5Z_filter_t LZ4_FILTER = (H5Z_filter_t)32004;
  /** Filter definition to write datasets with bitshuffle processed data */
  static const H5Z_filter_t BSLZ4_FILTER = (H5Z_filter_t)32008;

  HDF5Dataset_t& get_hdf5_dataset(const std::string dset_name);
  void extend_dataset(HDF5File::HDF5Dataset_t& dset, size_t frame_no) const;
  hid_t pixel_to_hdf_type(PixelType pixel) const;

  LoggerPtr logger_;
  /** Internal ID of the file being written to */
  hid_t hdf5_file_id_;
  /** Internal HDF5 error flag */
  bool hdf5_error_flag_;
  /** Internal HDF5 error recording */
  std::vector<std::string> hdf5_errors_;
  /** Map of datasets that are being written to */
  std::map<std::string, HDF5Dataset_t> hdf5_datasets_;
  /** The index of this file across all processors in the acquisition, 0 indexed */
  size_t file_index_;
  /** Full name and path of the file to write to */
  std::string filename_;
  /** Mutex used to make this class thread safe */
  boost::recursive_mutex mutex_;
};

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_SRC_HDF5FILE_H_ */

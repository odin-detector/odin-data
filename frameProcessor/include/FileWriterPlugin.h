/*
 * FileWriterPlugin.h
 *
 */

#ifndef TOOLS_FileWriterPlugin_FileWriterPlugin_H_
#define TOOLS_FileWriterPlugin_FileWriterPlugin_H_

#include <string>
#include <vector>
#include <queue>
#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <log4cxx/logger.h>
#include <hdf5.h>
#include "H5Zpublic.h"

using namespace log4cxx;

#include "FrameProcessorPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{

// Forward declarations
class Frame;

/** Plugin that writes Frame objects to HDF5 files.
 *
 * This plugin processes Frame objects and can write them to HDF5 files.
 * The plugin can be configured through the Ipc control interface defined
 * in the FileWriterPluginController class.  Currently only the raw data is written
 * into datasets.  Multiple datasets can be created and the raw data is stored
 * according to the Frame index (or name).
 */
class FileWriterPlugin : public FrameProcessorPlugin
{
  public:
    /**
     * Enumeration to store the pixel type of the incoming image
     */
    enum PixelType { pixel_raw_8bit, pixel_raw_16bit, pixel_float32 };

    /**
     * Defines a dataset to be saved in HDF5 format.
     */
    struct DatasetDefinition
    {
      /** Name of the dataset **/
      std::string name;
      /** Data type for the dataset **/
      FileWriterPlugin::PixelType pixel;
      /** Numer of frames expected to capture **/
      size_t num_frames;
      /** Array of dimensions of the dataset **/
      std::vector<long long unsigned int> frame_dimensions;
      /** Array of chunking dimensions of the dataset **/
      std::vector<long long unsigned int> chunks;
      /** Compression state of data **/
      std::string compression;
    };

    /**
     * Struct to keep track of an HDF5 dataset handle and dimensions.
     */
    struct HDF5Dataset_t
    {
      /** Handle of the dataset **/
      hid_t datasetid;
      /** Array of dimensions of the dataset **/
      std::vector<hsize_t> dataset_dimensions;
      /** Array of offsets of the dataset **/
      std::vector<hsize_t> dataset_offsets;
    };

    explicit FileWriterPlugin();
    virtual ~FileWriterPlugin();

    void createFile(std::string filename, size_t chunk_align=1024 * 1024);
    void createDataset(const FileWriterPlugin::DatasetDefinition & definition);
    void writeFrame(const Frame& frame);
    void writeSubFrames(const Frame& frame);
    void closeFile();

    size_t calculateFrameOffset(size_t frame_no);
    void queueFrameOffsetAdjustment(size_t active_frame_no, size_t offset);

    void startWriting();
    void stopWriting();
    void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
    void configureProcess(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
    void configureFile(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
    void configureDataset(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
    void status(OdinData::IpcMessage& status);
    void hdfErrorHandler(unsigned n, const H5E_error2_t *err_desc);
    bool checkForHdfErrors();
    std::vector<std::string> readHdfErrors();
    void clearHdfErrors();

  private:
    /** Configuration constant for process related items */
    static const std::string CONFIG_PROCESS;
    /** Configuration constant for number of processes */
    static const std::string CONFIG_PROCESS_NUMBER;
    /** Configuration constant for this process rank */
    static const std::string CONFIG_PROCESS_RANK;

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
    /** Configuration constant for the frame offset active frame*/
    static const std::string CONFIG_OFFSET_ADJUSTMENT_ACTIVE_FRAME;

    /** Filter definition to write datasets with LZ4 compressed data */
    static const H5Z_filter_t LZ4_FILTER = (H5Z_filter_t)32004;
    /** Filter definition to write datasets with bitshuffle processed data */
    static const H5Z_filter_t BS_FILTER = (H5Z_filter_t)32008;

    /**
     * Prevent a copy of the FileWriterPlugin plugin.
     *
     * \param[in] src
     */
    FileWriterPlugin(const FileWriterPlugin& src); // prevent copying one of these
    hid_t pixelToHdfType(FileWriterPlugin::PixelType pixel) const;
    HDF5Dataset_t& get_hdf5_dataset(const std::string dset_name);
    void extend_dataset(FileWriterPlugin::HDF5Dataset_t& dset, size_t frame_no) const;
    size_t adjustFrameOffset(size_t frame_no);
    void applyFrameOffsetAdjustment();

    void processFrame(boost::shared_ptr<Frame> frame);
    size_t getDatasetFrames(const std::string dset_name);

    /** Pointer to logger */
    LoggerPtr logger_;
    /** Mutex used to make this class thread safe */
    boost::recursive_mutex mutex_;
    /** Is this plugin writing frames to file? */
    bool writing_;
    /** Name of master frame.  When a master frame is received frame numbers increment */
    std::string masterFrame_;
    /** Number of frames to write to file */
    size_t framesToWrite_;
    /** Number of frames that have been written to file */
    size_t framesWritten_;
    /** Path of the file to write to */
    std::string filePath_;
    /** Name of the file to write to */
    std::string fileName_;
    /** Number of concurrent file writers executing */
    size_t concurrent_processes_;
    /** Rank of this file writer */
    size_t concurrent_rank_;
    /** Offset between raw frame ID and position in dataset */
    size_t frame_offset_adjustment_;
    /** Offset adjustment queue - pairs of active frame id and offset value */
    std::queue<std::pair<size_t, size_t> > offset_queue_;
    /** Internal ID of the file being written to */
    hid_t hdf5_fileid_;
    /** Internal HDF5 error flag */
    bool hdf5ErrorFlag_;
    /** Internal HDF5 error recording */
    std::vector<std::string> hdf5Errors_;
    /** Map of datasets that are being written to */
    std::map<std::string, FileWriterPlugin::HDF5Dataset_t> hdf5_datasets_;
    /** Map of dataset definitions for this file writer instance */
    std::map<std::string, FileWriterPlugin::DatasetDefinition> dataset_defs_;
};

/**
 * Registration of this plugin through the ClassLoader.  This macro
 * registers the class without needing to worry about name mangling
 */
REGISTER(FrameProcessorPlugin, FileWriterPlugin, "FileWriterPlugin");

}
#endif /* TOOLS_FileWriterPlugin_FileWriterPlugin_H_ */

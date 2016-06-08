/*
 * FileWriter.cpp
 *
 */
#include <assert.h>

#include "FileWriter.h"
#include <hdf5_hl.h>
#include "Frame.h"

namespace filewriter
{

FileWriter::FileWriter() :
  writing_(false),
  framesToWrite_(3),
  framesWritten_(0),
  subFramesWritten_(0),
  filePath_("./"),
  fileName_("test_file.h5"),
  concurrent_processes_(1),
  concurrent_rank_(0),
  hdf5_fileid_(0),
  start_frame_offset_(0)
{
    this->log_ = Logger::getLogger("FW.FileWriter");
    this->log_->setLevel(Level::getTrace());
    LOG4CXX_TRACE(log_, "FileWriter constructor.");

    this->hdf5_fileid_ = 0;
    this->start_frame_offset_ = 0;
}

FileWriter::FileWriter(size_t num_processes, size_t process_rank) :
  writing_(false),
  framesToWrite_(3),
  framesWritten_(0),
  subFramesWritten_(0),
  filePath_("./"),
  fileName_("test_file.h5"),
  concurrent_processes_(num_processes),
  concurrent_rank_(process_rank),
  hdf5_fileid_(0),
  start_frame_offset_(0)
{
    this->log_ = Logger::getLogger("FW.FileWriter");
    this->log_->setLevel(Level::getTrace());
    LOG4CXX_TRACE(log_, "FileWriter constructor.");
}

FileWriter::~FileWriter() {
    if (this->hdf5_fileid_ != 0) {
        LOG4CXX_TRACE(log_, "destructor closing file");
        H5Fclose(this->hdf5_fileid_);
        this->hdf5_fileid_ = 0;
    }
}

void FileWriter::createFile(std::string filename, size_t chunk_align) {
    hid_t fapl; /* File access property list */
    hid_t fcpl;

    /* Create file access property list */
    fapl = H5Pcreate(H5P_FILE_ACCESS);
    assert(fapl >= 0);

    assert(H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG) >= 0);

    /* Set chunk boundary alignment to 4MB */
    assert( H5Pset_alignment( fapl, 65536, 4*1024*1024 ) >= 0);

    /* Set to use the latest library format */
    assert(H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST) >= 0);

    /* Create file creation property list */
    if ((fcpl = H5Pcreate(H5P_FILE_CREATE)) < 0)
    assert(fcpl >= 0);

    /* Creating the file with SWMR write access*/
    LOG4CXX_INFO(log_, "Creating file: " << filename);
    unsigned int flags = H5F_ACC_TRUNC;
    this->hdf5_fileid_ = H5Fcreate(filename.c_str(), flags, fcpl, fapl);
    assert(this->hdf5_fileid_ >= 0);

    /* Close file access property list */
    assert(H5Pclose(fapl) >= 0);
}

void FileWriter::writeFrame(const Frame& frame) {
    herr_t status;
    hsize_t frame_no = frame.get_frame_number();

    HDF5Dataset_t& dset = this->get_hdf5_dataset(frame.get_dataset_name());

    hsize_t frame_offset = 0;
    frame_offset = this->getFrameOffset(frame_no);
    this->extend_dataset(dset, frame_offset + 1);

    LOG4CXX_DEBUG(log_, "Writing frame offset=" << frame_no  << " (" << frame_offset << ")"
    		             << " dset=" << frame.get_dataset_name());

    // Set the offset
    std::vector<hsize_t>offset(dset.dataset_dimensions.size());
    offset[0] = frame_offset;

    uint32_t filter_mask = 0x0;
    status = H5DOwrite_chunk(dset.datasetid, H5P_DEFAULT,
                             filter_mask, &offset.front(),
                             frame.get_data_size(), frame.get_data());
    assert(status >= 0);
}

/** Write horizontal subframes direct to dataset chunk
 *
 * Requirement: the chunking must have been configured exactly to the subframe size
 * and dimensions at dataset creation time.
 */
void FileWriter::writeSubFrames(const Frame& frame) {
    herr_t status;
    uint32_t filter_mask = 0x0;
    hsize_t frame_no = frame.get_frame_number();

    HDF5Dataset_t& dset = this->get_hdf5_dataset(frame.get_dataset_name());

    hsize_t frame_offset = 0;
    frame_offset = this->getFrameOffset(frame_no);

    this->extend_dataset(dset, frame_offset + 1);

    LOG4CXX_DEBUG(log_, "Writing frame=" << frame_no << " (" << frame_offset << ")"
    					<< " dset=" << frame.get_dataset_name());

    // Set the offset
    std::vector<hsize_t>offset(dset.dataset_dimensions.size(), 0);
    offset[0] = frame_offset;

    for (int i = 0; i < frame.get_parameter("subframe_count"); i++)
    {
      offset[2] = i * frame.get_dimensions("subframe")[1]; // For P2M: subframe is 704 pixels
        LOG4CXX_DEBUG(log_, "    offset=" << offset[0]
                  << "," << offset[1] << "," << offset[2]);

        LOG4CXX_DEBUG(log_, "    subframe_size=" << frame.get_parameter("subframe_size"));

        status = H5DOwrite_chunk(dset.datasetid, H5P_DEFAULT,
                                 filter_mask, &offset.front(),
                                 frame.get_parameter("subframe_size"),
                                 (static_cast<const char*>(frame.get_data())+(i*frame.get_parameter("subframe_size"))));
        assert(status >= 0);
    }
}

void FileWriter::createDataset(
        const FileWriter::DatasetDefinition& definition) {
    // Handles all at the top so we can remember to close them
    hid_t dataspace = 0;
    hid_t prop = 0;
    hid_t dapl = 0;
    herr_t status;
    hid_t dtype = pixelToHdfType(definition.pixel);

    std::vector<hsize_t> frame_dims = definition.frame_dimensions;

    // Dataset dims: {1, <image size Y>, <image size X>}
    std::vector<hsize_t> dset_dims(1,1);
    dset_dims.insert(dset_dims.end(), frame_dims.begin(), frame_dims.end());

    // If chunking has not been defined it defaults to a single full frame
    std::vector<hsize_t> chunk_dims(1, 1);
    if (definition.chunks.size() != dset_dims.size()) {
    	chunk_dims = dset_dims;
    } else {
    	chunk_dims = definition.chunks;
    }

    std::vector<hsize_t> max_dims = dset_dims;
    max_dims[0] = H5S_UNLIMITED;

    /* Create the dataspace with the given dimensions - and max dimensions */
    dataspace = H5Screate_simple(dset_dims.size(), &dset_dims.front(), &max_dims.front());
    assert(dataspace >= 0);

    /* Enable chunking  */
    LOG4CXX_DEBUG(log_, "Chunking=" << chunk_dims[0] << ","
                       << chunk_dims[1] << ","
                       << chunk_dims[2]);
    prop = H5Pcreate(H5P_DATASET_CREATE);
    status = H5Pset_chunk(prop, dset_dims.size(), &chunk_dims.front());
    assert(status >= 0);

    char fill_value[8] = {0,0,0,0,0,0,0,0};
    status = H5Pset_fill_value(prop, dtype, fill_value);
    assert(status >= 0);

    dapl = H5Pcreate(H5P_DATASET_ACCESS);

    /* Create dataset  */
    LOG4CXX_DEBUG(log_, "Creating dataset: " << definition.name);
    FileWriter::HDF5Dataset_t dset;
    dset.datasetid = H5Dcreate2(this->hdf5_fileid_, definition.name.c_str(),
                                        dtype, dataspace,
                                        H5P_DEFAULT, prop, dapl);
    assert(dset.datasetid >= 0);
    dset.dataset_dimensions = dset_dims;
    dset.dataset_offsets = std::vector<hsize_t>(3);
    this->hdf5_datasets_[definition.name] = dset;

    LOG4CXX_DEBUG(log_, "Closing intermediate open HDF objects");
    assert( H5Pclose(prop) >= 0);
    assert( H5Pclose(dapl) >= 0);
    assert( H5Sclose(dataspace) >= 0);
}


void FileWriter::closeFile() {
    LOG4CXX_TRACE(log_, "FileWriter closeFile");
    if (this->hdf5_fileid_ >= 0) {
        assert(H5Fclose(this->hdf5_fileid_) >= 0);
        this->hdf5_fileid_ = 0;
    }
}

hid_t FileWriter::pixelToHdfType(FileWriter::PixelType pixel) const {
    hid_t dtype = 0;
    switch(pixel)
    {
    case pixel_float32:
        dtype = H5T_NATIVE_UINT32;
        break;
    case pixel_raw_16bit:
        dtype = H5T_NATIVE_UINT16;
        break;
    case pixel_raw_8bit:
        dtype = H5T_NATIVE_UINT8;
        break;
    default:
        dtype = H5T_NATIVE_UINT16;
    }
    return dtype;
}

FileWriter::HDF5Dataset_t& FileWriter::get_hdf5_dataset(const std::string dset_name) {
    // Check if the frame destination dataset has been created
    if (this->hdf5_datasets_.find(dset_name) == this->hdf5_datasets_.end())
    {
        // no dataset of this name exist
        LOG4CXX_ERROR(log_, "Attempted to access non-existent dataset: \"" << dset_name << "\"\n");
        throw std::runtime_error("Attempted to access non-existent dataset");
    }
    return this->hdf5_datasets_.at(dset_name);
}

size_t FileWriter::getFrameOffset(size_t frame_no) const {
    size_t frame_offset = this->adjustFrameOffset(frame_no);

    if (this->concurrent_processes_ > 1) {
        // Check whether this frame should really be in this process
        // Note: this expects the frame numbering from HW/FW to start at 1, not 0!
        if ( (((frame_no-1) % this->concurrent_processes_) - this->concurrent_rank_) != 0) {
            LOG4CXX_WARN(log_, "Unexpected frame: " << frame_no
                                << " in this process rank: "
                                << this->concurrent_rank_);
            throw std::runtime_error("Unexpected frame in this process rank");
        }

        // Calculate the new offset based on how many concurrent processes are running
        frame_offset = frame_offset / this->concurrent_processes_;
    }
    return frame_offset;
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
 *   frame number than the initial frame used to set the offset.
 *
 * Returns the dataset offset for frame number <frame_no>
 */
size_t FileWriter::adjustFrameOffset(size_t frame_no) const {
    size_t frame_offset = 0;
    if (frame_no < this->start_frame_offset_) {
        // Deal with a frame arriving after the very first frame
        // which was used to set the offset: throw a range error
        throw std::range_error("Frame out of order at start causing negative file offset");
    }

    // Normal case: apply offset
    frame_offset = frame_no - this->start_frame_offset_;
    return frame_offset;
}

/** Part of big nasty work-around for the missing frame counter reset in FW
 *
 */
void FileWriter::setStartFrameOffset(size_t frame_no) {
    this->start_frame_offset_ = frame_no;
}

void FileWriter::extend_dataset(HDF5Dataset_t& dset, size_t frame_no) const {
	herr_t status;
    if (frame_no > dset.dataset_dimensions[0]) {
        // Extend the dataset
        LOG4CXX_DEBUG(log_, "Extending dataset_dimensions[0] = " << frame_no);
        dset.dataset_dimensions[0] = frame_no;
        status = H5Dset_extent( dset.datasetid,
                                &dset.dataset_dimensions.front());
        assert(status >= 0);
    }
}

void FileWriter::processFrame(boost::shared_ptr<Frame> frame)
{
  if (writing_){
    this->writeSubFrames(*frame);
    subFramesWritten_++;
    if (subFramesWritten_ == 2){
      subFramesWritten_ = 0;
      framesWritten_++;
    }
    // Check if we need to stop writing frames
    if (framesWritten_ == framesToWrite_){
      this->stopWriting();
    }
  }
}

void FileWriter::startWriting()
{
  if (!writing_){
    // TODO: HARDCODED
    // Assuming this is a P2M, our image dimensions are:
    size_t bytes_per_pixel = 2;
    dimensions_t p2m_dims(2); p2m_dims[0] = 1484; p2m_dims[1] = 1408;
    dimensions_t p2m_subframe_dims = p2m_dims; p2m_subframe_dims[1] = p2m_subframe_dims[1] >> 1;
    this->createFile(filePath_ + fileName_);
    FileWriter::DatasetDefinition dset_def;
    dset_def.name = "data";
    dset_def.frame_dimensions = p2m_dims;
    dset_def.chunks = dimensions_t(3,1);
    dset_def.chunks[1] = p2m_subframe_dims[0]; // Chunk the frame in half (horizontal split) so we can write subframes
    dset_def.chunks[2] = p2m_subframe_dims[1];
    dset_def.pixel = FileWriter::pixel_raw_16bit;
    dset_def.num_frames = framesToWrite_;
    this->createDataset(dset_def);
    dset_def.name = "reset";
    this->createDataset(dset_def);

    // Reset counters
    framesWritten_ = 0;
    subFramesWritten_ = 0;

    // Set writing flag to true
    writing_ = true;
  }
}

void FileWriter::stopWriting()
{
  if (writing_){
    writing_ = false;
    this->closeFile();
  }
}

boost::shared_ptr<filewriter::JSONMessage> FileWriter::configure(boost::shared_ptr<filewriter::JSONMessage> config)
{
  LOG4CXX_DEBUG(log_, config->toString());
  // Check config for items
  if (config->HasMember("filepath")){
    if ((*config)["filepath"].IsString()){
      filePath_ = (*config)["filepath"].GetString();
    }
  }
  if (config->HasMember("filename")){
    if ((*config)["filename"].IsString()){
      fileName_ = (*config)["filename"].GetString();
    }
  }
  if (config->HasMember("frames")){
    if ((*config)["frames"].IsInt()){
      framesToWrite_ = (*config)["frames"].GetInt();
    }
  }
  // Final check is to start or stop writing
  if (config->HasMember("write")){
    if ((*config)["write"].IsBool()){
      if ((*config)["write"].GetBool() == true){
        this->startWriting();
      } else {
        this->stopWriting();
      }
    }
  }

  return config;
}

}

/*
 * FileWriter.h
 *
 */

#ifndef TOOLS_FILEWRITER_FILEWRITER_H_
#define TOOLS_FILEWRITER_FILEWRITER_H_

#include <string>
#include <vector>
#include <map>
#include <log4cxx/logger.h>
#include <hdf5.h>

using namespace log4cxx;

// Forward declarations
class Frame;

class FileWriter {
public:
    enum PixelType { pixel_raw_16bit, pixel_float32 };
    struct DatasetDefinition {
        std::string name;
        FileWriter::PixelType pixel;
        size_t num_frames;
        std::vector<long long unsigned int> frame_dimensions;
    };

    struct HDF5Dataset_t {
        hid_t datasetid;
        std::vector<hsize_t> dataset_dimensions;
        std::vector<hsize_t> dataset_offsets;
    };

    explicit FileWriter();
    virtual ~FileWriter();

    void createFile(std::string filename, size_t chunk_align=1024 * 1024);
    void createDataset(const FileWriter::DatasetDefinition & definition);
    void writeFrame(const Frame& frame);
    void closeFile();

private:
    FileWriter(const FileWriter& src); // prevent copying one of these
    LoggerPtr log_;
    hid_t pixelToHdfType(FileWriter::PixelType pixel) const;

    hid_t hdf5_fileid_;
    std::map<std::string, FileWriter::HDF5Dataset_t> hdf5_datasets_;
};

#endif /* TOOLS_FILEWRITER_FILEWRITER_H_ */

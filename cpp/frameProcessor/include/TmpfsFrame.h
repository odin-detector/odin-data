#ifndef FRAMEPROCESSOR_TMPFSFRAME_H
#define FRAMEPROCESSOR_TMPFSFRAME_H

#include <boost/filesystem.hpp> // boost::filesystem::path, boost::filesystem::remove

#include "Frame.h"

namespace FrameProcessor {

/** A Frame backed by a memory mapped temporary file on disk, for example a `tmpfs` mount such as `/dev/shm`.
 *
 * \param[in] file_path - The full path to the file to create
 * \param[in] meta_data - The metadata of the `Frame`
 * \param[in] data_src - Pointer to the data to copy into the file
 * \param[in] data_size - Size of the data to copy
 * \param[in] image_offset - Offset from `data_src` to the image data (passed to parent class)
 *
 * \throws std::runtime_error if the underlying file could not be opened
*/
class TmpfsFrame : public Frame {

 public:

  /** Constructor */
  TmpfsFrame(
    std::string& file_path,
    const FrameMetaData &meta_data,
    const void* data_src,
    size_t data_size,
    const int& image_offset=0
  );

  /** Destructor */
  ~TmpfsFrame();

  /** Return a void pointer to the raw data */
  virtual void* get_data_ptr() const;

private:
  /** Pointer to memory mapped file **/
  void* data_ptr_;
  boost::filesystem::path full_file_path_;

};

}

#endif

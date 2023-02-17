#include <sys/mman.h> // mmap, PROT_WRITE, MAP_SHARED
#include <fcntl.h> // open, O_CREAT, O_RDWR

#include "TmpfsFrame.h"

namespace FrameProcessor {

TmpfsFrame::TmpfsFrame(
  std::string& file_name,
  const FrameMetaData &meta_data,
  const void* data_src,
  size_t data_size,
  const int& image_offset
) :
  Frame(meta_data, data_size, image_offset)
{
  this->full_file_path_ = boost::filesystem::path(file_name);
  try {
    boost::filesystem::create_directories(this->full_file_path_.parent_path());
  } catch (boost::filesystem::filesystem_error& e) {
    std::stringstream error;
    error << "Failed to create directory: " << e.what();
    throw std::runtime_error(error.str());
  }

  int fd = open(this->full_file_path_.c_str(), O_CREAT | O_RDWR, 0666);
  if (fd == -1) {
    throw std::runtime_error("Failed to open " + this->full_file_path_.string());
  }

  int result = ftruncate(fd, data_size);
  if (result == -1) {
    std::stringstream error;
    error << "Failed to truncate " << this->full_file_path_.string() << " to size " << data_size;
    throw std::runtime_error(error.str());
  }

  this->data_ptr_ = mmap(NULL, data_size, PROT_WRITE, MAP_SHARED, fd, 0);
  memcpy(this->data_ptr_, data_src, data_size);

  close(fd);
}

TmpfsFrame::~TmpfsFrame () {
  munmap(this->data_ptr_, this->get_data_size());
  boost::filesystem::remove(this->full_file_path_);
}

/** Return a void pointer to the raw data.
 *
 * \return Pointer to the raw data.
 */
void* TmpfsFrame::get_data_ptr() const {
  return data_ptr_;
}

} // namespace FrameProcessor

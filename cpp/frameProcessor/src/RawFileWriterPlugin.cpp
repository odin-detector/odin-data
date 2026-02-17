
#include <sys/mman.h> // PROT_WRITE, MAP_SHARED
#include <fcntl.h> // open, O_CREAT, O_RDWR

#include <version.h>
#include <DebugLevelLogger.h>

#include "RawFileWriterPlugin.h"

namespace FrameProcessor
{

const std::string RawFileWriterPlugin::CONFIG_FILE_PATH = "file_path";
const std::string RawFileWriterPlugin::CONFIG_ENABLED = "writing_enabled";


RawFileWriterPlugin::RawFileWriterPlugin() : file_path_{""}, dropped_frames_{0}, enabled_{false} {
  // Setup logging for the class
  logger_ = Logger::getLogger("FP.TempfsPlugin");
  LOG4CXX_TRACE(logger_, "RawFileWriterPlugin constructor.");
}

void RawFileWriterPlugin::process_frame(boost::shared_ptr<Frame> frame) {
  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);
  std::array<char, 128> msg{};
  if(!this->enabled_){
    ++this->dropped_frames_;
    this->push(frame);
    return;
  }

  const std::string& acq_id = frame->get_meta_data().get_acquisition_ID();
  long long fr_num = frame->get_frame_number();
  std::string f_path = this->file_path_.string() + '/' + acq_id + '/' + std::to_string(fr_num);
  size_t dsize = frame->get_data_size();
  int fd = open(f_path.c_str(), O_CREAT | O_RDWR, 0666);
  if (fd == -1) {
    ++this->dropped_frames_;
    strerror_r(errno, msg.data(), msg.max_size());
    LOG4CXX_ERROR(logger_, "Failed to open: " << this->file_path_.string() << " errno: " << msg.data());
  }else if (ftruncate(fd, dsize) == -1) {
    ++this->dropped_frames_;
    strerror_r(errno, msg.data(), msg.max_size());
    LOG4CXX_ERROR(logger_, "Failed to truncate: " << this->file_path_.string() << " to size " << dsize << " errno: " << msg.data());
  }else  if(write(fd, frame->get_data_ptr(), dsize) == -1){
    ++this->dropped_frames_;
    strerror_r(errno, msg.data(), msg.max_size());
    LOG4CXX_ERROR(logger_, "Failed to Write to file - " << this->file_path_.string() << ": " << msg.data());
  }

  fd != -1 && close(fd);
  this->push(frame);
}

/** Configure
 * @param config
 * @param reply
 */
void RawFileWriterPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply) {
  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);
  if(config.has_param(RawFileWriterPlugin::CONFIG_FILE_PATH)){
    std::string path_str = config.get_param<std::string>(RawFileWriterPlugin::CONFIG_FILE_PATH);
    this->file_path_ = std::move(path_str);
    try {
      boost::filesystem::create_directories(this->file_path_.parent_path());
    } catch (boost::filesystem::filesystem_error& e) {
      this->enabled_ = false;
      std::stringstream error;
      error << "Failed to create directory: " << file_path_.c_str() << ", Error:" << e.what();
      this->set_error(error.str());
      file_path_.clear();
      throw std::runtime_error(error.str());
    }
  }
}

/** Get configuration settings for the RawFileWriterPlugin
 *
 * @param reply - Response IpcMessage.
 */
void RawFileWriterPlugin::requestConfiguration(OdinData::IpcMessage& reply)
{
  reply.set_param(this->get_name() + '/' + RawFileWriterPlugin::CONFIG_FILE_PATH, this->file_path_);
  reply.set_param(this->get_name() + '/' + RawFileWriterPlugin::CONFIG_ENABLED, this->enabled_);
}

/** Get status of the RawFileWriterPlugin
 *
 * @param reply - Response IpcMessage.
 */
void RawFileWriterPlugin::status(OdinData::IpcMessage& status)
{
  status.set_param(this->get_name() + '/' + RawFileWriterPlugin::CONFIG_FILE_PATH, this->file_path_);
  status.set_param(this->get_name() + '/' + RawFileWriterPlugin::CONFIG_ENABLED, this->enabled_);
  status.set_param(this->get_name() + '/' + "dropped_frames", this->dropped_frames_);
}


int RawFileWriterPlugin::get_version_major() {
  return ODIN_DATA_VERSION_MAJOR;
}

int RawFileWriterPlugin::get_version_minor() {
  return ODIN_DATA_VERSION_MINOR;
}

int RawFileWriterPlugin::get_version_patch() {
  return ODIN_DATA_VERSION_PATCH;
}

std::string RawFileWriterPlugin::get_version_short() {
  return ODIN_DATA_VERSION_STR_SHORT;
}

std::string RawFileWriterPlugin::get_version_long() {
  return ODIN_DATA_VERSION_STR;
}

}

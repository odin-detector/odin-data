
#include <sys/mman.h> // PROT_WRITE, MAP_SHARED
#include <sys/stat.h> // mkdir
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
  if(!this->enabled_) {
    this->push(frame);
    return;
  }
  
  const std::string& acq_id = frame->get_meta_data().get_acquisition_ID();
  std::string&& f_path = this->file_path_.string() + '/' + acq_id + '/';
  if(mkdir(f_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IWOTH) == -1 && (errno != EEXIST)) {
    this->enabled_ = false;
    ++this->dropped_frames_;
    strerror_r(errno, msg.data(), msg.max_size());
    LOG4CXX_ERROR(logger_, "Failed to create directory: " << f_path << " errno: " << msg.data());
    this->push(frame);
    return;
  }

  long long fr_num = frame->get_frame_number();
  size_t data_size = frame->get_data_size();
  int fd = open((f_path += std::to_string(fr_num)).c_str(), O_CREAT | O_RDWR, 0666);
  if(fd == -1) {
    ++this->dropped_frames_;
    strerror_r(errno, msg.data(), msg.max_size());
    LOG4CXX_ERROR(logger_, "Failed to open: " << f_path << " errno: " << msg.data());
  } else if(ftruncate(fd, data_size) == -1) {
    ++this->dropped_frames_;
    strerror_r(errno, msg.data(), msg.max_size());
    LOG4CXX_ERROR(logger_, "Failed to truncate: " << f_path << " to size " << data_size << " errno: " << msg.data());
  } else if(write(fd, frame->get_data_ptr(), data_size) == -1) {
    ++this->dropped_frames_;
    strerror_r(errno, msg.data(), msg.max_size());
    LOG4CXX_ERROR(logger_, "Failed to write to file - " << f_path << ": " << msg.data());
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
  if(config.has_param(RawFileWriterPlugin::CONFIG_ENABLED)) {
    this->enabled_ = config.get_param<bool>(RawFileWriterPlugin::CONFIG_ENABLED);
  }
  if(config.has_param(RawFileWriterPlugin::CONFIG_FILE_PATH)) {
    std::string path_str = config.get_param<std::string>(RawFileWriterPlugin::CONFIG_FILE_PATH);
    this->file_path_ = std::move(path_str);
    try {
      boost::filesystem::create_directories(this->file_path_.parent_path());
    } catch (boost::filesystem::filesystem_error& e) {
      this->enabled_ = false;
      std::stringstream error;
      error << "Failed to create directory: " << this->file_path_.c_str() << ", Error:" << e.what();
      this->set_error(error.str());
      this->file_path_.clear();
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

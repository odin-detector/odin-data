
#include <sys/mman.h> // PROT_WRITE, MAP_SHARED
#include <fcntl.h> // open, O_CREAT, O_RDWR

#include <version.h>
#include <DebugLevelLogger.h>

#include "TmpFSPlugin.h"

namespace FrameProcessor
{

const std::string TmpFSPlugin::CONFIG_FILE_PATH = "file_path";
// const std::string TmpFSPlugin::CONFIG_REMOVE_FILE = "remove_file";

TmpFSPlugin::TmpFSPlugin() : full_file_path_{""} {
  // Setup logging for the class
  logger_ = Logger::getLogger("FP.BloscPlugin");
  LOG4CXX_TRACE(logger_, "TmpFSPlugin constructor.");
}

void TmpFSPlugin::process_frame(boost::shared_ptr<Frame> frame) {
  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);
  LOG4CXX_DEBUG_LEVEL(3, logger_, "TmpFS received a new frame...");

  int fd = open(this->full_file_path_.c_str(), O_CREAT | O_RDWR, 0666);
  if (fd == -1) {
    throw std::runtime_error("Failed to open " + this->full_file_path_.string());
  }
  size_t dsize = frame->get_data_size();
  int result = ftruncate(fd, dsize);
  if (result == -1) {
    std::stringstream error;
    error << "Failed to truncate " << this->full_file_path_.string() << " to size " << dsize;
    throw std::runtime_error(error.str());
  }

  close(fd);
}

/** Configure
 * @param config
 * @param reply
 */
void TmpFSPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply) {
  if(config.has_param(TmpFSPlugin::CONFIG_FILE_PATH)){
    std::string path_str = config.get_param<std::string>(TmpFSPlugin::CONFIG_FILE_PATH);
    if(path_str.empty()){
      reply.set_param<std::string>("warningboost::filesystem::create_directories: level", "File Path name is EMPTY/INVALID");
      LOG4CXX_WARN(logger_, "Empty string for File Path");
    }
    this->full_file_path_ = std::move(path_str);
  }

  // if(config.has_param(TmpFSPlugin::CONFIG_REMOVE_FILE)){
  //   this->remove_file_ = config.get_param<bool>(TmpFSPlugin::CONFIG_REMOVE_FILE);
  // }

  try {
    boost::filesystem::create_directories(this->full_file_path_.parent_path());
  } catch (boost::filesystem::filesystem_error& e) {
    std::stringstream error;
    error << "Failed to create directory: " << e.what();
    throw std::runtime_error(error.str());
  }
}

/** Get configuration settings for the BloscPlugin
 *
 * @param reply - Response IpcMessage.
 */
void TmpFSPlugin::requestConfiguration(OdinData::IpcMessage& reply)
{
  reply.set_param(this->get_name() + '/' + TmpFSPlugin::CONFIG_FILE_PATH, this->full_file_path_);
}

/** Collate status information for the plugin
 *
 * @param status - Reference to an IpcMessage value to store the status
 */
void TmpFSPlugin::status(OdinData::IpcMessage& status)
{
  status.set_param(this->get_name() + '/' + TmpFSPlugin::CONFIG_FILE_PATH, this->full_file_path_);
}


int TmpFSPlugin::get_version_major() {
  return ODIN_DATA_VERSION_MAJOR;
}

int TmpFSPlugin::get_version_minor() {
  return ODIN_DATA_VERSION_MINOR;
}

int TmpFSPlugin::get_version_patch() {
  return ODIN_DATA_VERSION_PATCH;
}

std::string TmpFSPlugin::get_version_short() {
  return ODIN_DATA_VERSION_STR_SHORT;
}

std::string TmpFSPlugin::get_version_long() {
  return ODIN_DATA_VERSION_STR;
}

}

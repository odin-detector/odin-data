
#include <fcntl.h> // open, O_CREAT, O_RDWR
#include <sys/mman.h> // PROT_WRITE, MAP_SHARED
#include <sys/stat.h> // mkdir

#include "logging.h"
#include <DebugLevelLogger.h>
#include <version.h>

#include "RawFileWriterPlugin.h"

namespace FrameProcessor {

const std::string RawFileWriterPlugin::CONFIG_FILE_PATH = "file_path";
const std::string RawFileWriterPlugin::CONFIG_ENABLED = "writing_enabled";
const std::string RawFileWriterPlugin::STATUS_DROPPED_FRAMES = "dropped_frames";

RawFileWriterPlugin::RawFileWriterPlugin() :
    file_path_ { "" },
    dropped_frames_ { 0 },
    enabled_ { false }
{
    // Setup logging for the class
    logger_ = Logger::getLogger("FP.RawFileWriterPlugin");
    LOG4CXX_TRACE(logger_, "RawFileWriterPlugin constructor.");
}

void RawFileWriterPlugin::process_frame(boost::shared_ptr<Frame> frame)
{
    // Protect this method
    boost::lock_guard<boost::recursive_mutex> lock(mutex_);
    this->push(frame);
    if (!this->enabled_) {
        return;
    }

    const std::string& acq_id = frame->get_meta_data().get_acquisition_ID();
    std::string&& full_file_path = this->file_path_.string() + acq_id + '/';
    boost::system::error_code ec;
    boost::filesystem::create_directory(full_file_path, ec);
    if (ec && (ec.value() != boost::system::errc::errc_t::file_exists)) {
        this->enabled_ = false;
        ++this->dropped_frames_;
        LOG_WITH_ERRNO(logger_, "Failed to create directory: " << full_file_path);
        return;
    }

    long long frame_number = frame->get_frame_number();
    size_t data_size = frame->get_data_size();
    int fd = open((full_file_path += std::to_string(frame_number)).c_str(), O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        ++this->dropped_frames_;
        LOG_WITH_ERRNO(logger_, "Failed to open: " << full_file_path);
    } else if (ftruncate(fd, data_size) == -1) {
        ++this->dropped_frames_;
        LOG_WITH_ERRNO(logger_, "Failed to truncate: " << full_file_path);
    } else if (write(fd, frame->get_data_ptr(), data_size) == -1) {
        ++this->dropped_frames_;
        LOG_WITH_ERRNO(logger_, "Failed to write: " << full_file_path);
    }
    fd != -1 && close(fd);
}

/** Configure
 * @param config
 * @param reply
 */
void RawFileWriterPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
    // Protect this method
    boost::lock_guard<boost::recursive_mutex> lock(mutex_);
    if (config.has_param(RawFileWriterPlugin::CONFIG_ENABLED)) {
        this->enabled_ = config.get_param<bool>(RawFileWriterPlugin::CONFIG_ENABLED);
    }
    if (config.has_param(RawFileWriterPlugin::CONFIG_FILE_PATH)) {
        std::string path_str = config.get_param<std::string>(RawFileWriterPlugin::CONFIG_FILE_PATH);
        if (path_str.back() != '/')
            path_str += '/';
        this->file_path_ = std::move(path_str);
        try {
            boost::filesystem::create_directories(this->file_path_.parent_path());
        } catch (boost::filesystem::filesystem_error& e) {
            this->enabled_ = false;
            std::stringstream error;
            error << "Failed to create directory: " << this->file_path_.c_str() << ", Error: " << e.what();
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
    reply.set_param(this->get_name() + '/' + RawFileWriterPlugin::CONFIG_FILE_PATH, this->file_path_.string());
    reply.set_param(this->get_name() + '/' + RawFileWriterPlugin::CONFIG_ENABLED, this->enabled_);
}

/** Get status of the RawFileWriterPlugin
 *
 * @param reply - Response IpcMessage.
 */
void RawFileWriterPlugin::status(OdinData::IpcMessage& status)
{
    status.set_param(this->get_name() + '/' + RawFileWriterPlugin::STATUS_DROPPED_FRAMES, this->dropped_frames_);
}

int RawFileWriterPlugin::get_version_major()
{
    return ODIN_DATA_VERSION_MAJOR;
}

int RawFileWriterPlugin::get_version_minor()
{
    return ODIN_DATA_VERSION_MINOR;
}

int RawFileWriterPlugin::get_version_patch()
{
    return ODIN_DATA_VERSION_PATCH;
}

std::string RawFileWriterPlugin::get_version_short()
{
    return ODIN_DATA_VERSION_STR_SHORT;
}

std::string RawFileWriterPlugin::get_version_long()
{
    return ODIN_DATA_VERSION_STR;
}

}

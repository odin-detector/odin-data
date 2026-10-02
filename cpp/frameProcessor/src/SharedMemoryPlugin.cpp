

#include "SharedMemoryPlugin.h"
#include "version.h"

namespace FrameProcessor {

const std::string SharedMemoryPlugin::CONFIG_FR_RELEASE = "fr_release_cnxn";
const std::string SharedMemoryPlugin::CONFIG_FR_READY = "fr_ready_cnxn";
const std::string SharedMemoryPlugin::STATUS_SHB_NAME = "shared_buf_name";
const std::string SharedMemoryPlugin::STATUS_SHB_CONFIGURED = "shared_buf_configured";
const std::string SharedMemoryPlugin::STATUS_FR_RECV = "frames_recv";
/**
 * The constructor sets up logging used within the class.
 */
SharedMemoryPlugin::SharedMemoryPlugin() :
    frames_recv_ { 0 }
{
    LOG4CXX_TRACE(Logger::getLogger("FP.SharedMemoryPlugin"), "SharedMemoryPlugin constructor.");
    m_thread_ = boost::thread { boost::bind(&SharedMemoryPlugin::start_reactor, this) };
    add_config_param_metadata(CONFIG_FR_RELEASE, PMDD::STRING_T, PMDA::READ_WRITE);
    add_config_param_metadata(CONFIG_FR_READY, PMDD::STRING_T, PMDA::READ_WRITE);
    add_status_param_metadata(STATUS_SHB_NAME, PMDD::STRING_T, PMDA::READ_ONLY);
    add_status_param_metadata(STATUS_SHB_CONFIGURED, PMDD::BOOL_T, PMDA::READ_ONLY);
}

SharedMemoryPlugin::~SharedMemoryPlugin()
{
    LOG4CXX_TRACE(Logger::getLogger("FP.SharedMemoryPlugin"), "SharedMemoryPlugin destructor.");
    reactor_.stop();
    m_thread_.join();
}

void SharedMemoryPlugin::start_reactor()
{
    reactor_.register_timer(1000, 0, &tick_timer);
    reactor_.run();
}

void SharedMemoryPlugin::process_frame(boost::shared_ptr<Frame> ptr)
{
    ++frames_recv_;
    this->push(ptr);
}

void SharedMemoryPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
    if (config.has_param(SharedMemoryPlugin::CONFIG_FR_RELEASE)
        && config.has_param(SharedMemoryPlugin::CONFIG_FR_READY)) {
        std::string pubString = config.get_param<std::string>(SharedMemoryPlugin::CONFIG_FR_RELEASE);
        std::string subString = config.get_param<std::string>(SharedMemoryPlugin::CONFIG_FR_READY);
        this->setupFrameReceiverInterface(pubString, subString);
    }
}

/** Set up the frame receiver interface.
 *
 * This method creates new SharedMemoryController and SharedMemoryParser objects,
 * which manage the receipt of frame ready notifications and construction of
 * Frame objects from shared memory.
 * Pointers to the two objects are kept by this class.
 *
 * \param[in] sharedMemName - Name of the shared memory block opened by the frame receiver.
 * \param[in] frReleaseString - Endpoint for sending frame release notifications.
 * \param[in] frReadyString - Endpoint for receiving frame ready notifications.
 */
void SharedMemoryPlugin::setupFrameReceiverInterface(std::string& frReleaseString, std::string& frReadyString)
{
    LOG4CXX_DEBUG(
        Logger::getLogger("FP.SharedMemoryPlugin"),
        "Shared Memory Config: Publisher=" << frReleaseString << " Subscriber=" << frReadyString
    );

    // Only reconstruct the shared memory controller if it has never been created or either
    // of the endpoints has been changed
    if (!shmctrlr_handle_ || frReleaseString != frReleaseEndpoint_ || frReadyString != frReadyEndpoint_) {
        try {
            // destroy the current shared memory controller if one exists
            if (shmctrlr_handle_) {
                shmctrlr_handle_.reset();
            }
            // Create the new shared memory controller and give it the parser and publisher
            shmctrlr_handle_.emplace(reactor_, frReadyString, frReleaseString);
            shmctrlr_handle_->inject_process_frame_cb(
                boost::bind(&SharedMemoryPlugin::process_frame, this, boost::placeholders::_1)
            );
            frReadyEndpoint_ = std::move(frReadyString);
            frReleaseEndpoint_ = std::move(frReleaseString);
        } catch (const boost::interprocess::interprocess_exception& e) {
            LOG4CXX_ERROR(Logger::getLogger("FP.SharedMemoryPlugin"), "Unable to access shared memory: \n" << e.what());
        }
    } else {
        LOG4CXX_TRACE(
            Logger::getLogger("FP.SharedMemoryPlugin"), "Not updating shared memory, endpoints were not changed"
        );
    }
}

/** Get configuration settings for the SharedMemoryPlugin
 *
 * @param reply - Response IpcMessage.
 */
void SharedMemoryPlugin::requestConfiguration(OdinData::IpcMessage& reply)
{
    reply.set_param(this->get_name() + '/' + SharedMemoryPlugin::CONFIG_FR_READY, this->frReadyEndpoint_);
    reply.set_param(this->get_name() + '/' + SharedMemoryPlugin::CONFIG_FR_RELEASE, this->frReleaseEndpoint_);
}

void SharedMemoryPlugin::status(OdinData::IpcMessage& reply)
{
    if (shmctrlr_handle_) {
        reply.set_param(this->get_name() + '/' + SharedMemoryPlugin::STATUS_SHB_NAME, shmctrlr_handle_->getName());
        reply.set_param(
            this->get_name() + '/' + SharedMemoryPlugin::STATUS_SHB_CONFIGURED, shmctrlr_handle_->isConfigured()
        );
    }
    reply.set_param(this->get_name() + '/' + SharedMemoryPlugin::STATUS_FR_RECV, frames_recv_);
}

int SharedMemoryPlugin::get_version_major()
{
    return ODIN_DATA_VERSION_MAJOR;
}

int SharedMemoryPlugin::get_version_minor()
{
    return ODIN_DATA_VERSION_MINOR;
}

int SharedMemoryPlugin::get_version_patch()
{
    return ODIN_DATA_VERSION_PATCH;
}

std::string SharedMemoryPlugin::get_version_short()
{
    return ODIN_DATA_VERSION_STR_SHORT;
}

std::string SharedMemoryPlugin::get_version_long()
{
    return ODIN_DATA_VERSION_STR;
}

} // namespace FrameProcessor

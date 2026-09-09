

#include "SharedMemoryPlugin.h"
#include "version.h"

namespace FrameProcessor {

const std::string SharedMemoryPlugin::CONFIG_FR_RELEASE = "fr_release_cnxn";
const std::string SharedMemoryPlugin::CONFIG_FR_READY = "fr_ready_cnxn";
const std::string SharedMemoryPlugin::STATUS_SHB_NAME = "shared_buf_name";
const std::string SharedMemoryPlugin::STATUS_SHB_CONFIGURED = "shared_buf_configured";
/**
 * The constructor sets up logging used within the class.
 */
SharedMemoryPlugin::SharedMemoryPlugin()
{
    // Setup logging for the class
    logger_ = Logger::getLogger("FP.SharedMemoryPlugin");
    LOG4CXX_TRACE(logger_, "SharedMemoryPlugin constructor.");
    add_config_param_metadata(CONFIG_FR_RELEASE, PMDD::STRING_T, PMDA::READ_WRITE);
    add_config_param_metadata(CONFIG_FR_READY, PMDD::STRING_T, PMDA::READ_WRITE);
    add_status_param_metadata(STATUS_SHB_NAME, PMDD::STRING_T, PMDA::READ_ONLY);
    add_status_param_metadata(STATUS_SHB_CONFIGURED, PMDD::BOOL_T, PMDA::READ_ONLY);
    reactor_ = boost::make_shared<OdinData::IpcReactor>();
    reactor_->register_timer(5000, 0, &dummy_timer);
    // boost::bind
    boost::thread m_thread_ { &OdinData::IpcReactor::run, reactor_.get() };
}

SharedMemoryPlugin::~SharedMemoryPlugin()
{
    LOG4CXX_TRACE(logger_, "SharedMemoryPlugin destructor.");
    reactor_->stop();
    m_thread_.join();
}

void SharedMemoryPlugin::process_frame(boost::shared_ptr<Frame> ptr)
{
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
 * \param[in] frPublisherString - Endpoint for sending frame release notifications.
 * \param[in] frSubscriberString - Endpoint for receiving frame ready notifications.
 */
void SharedMemoryPlugin::setupFrameReceiverInterface(
    const std::string& frPublisherString,
    const std::string& frSubscriberString
)
{
    LOG4CXX_DEBUG(
        logger_, "Shared Memory Config: Publisher=" << frPublisherString << " Subscriber=" << frSubscriberString
    );

    // Only reconstruct the shared memory controller if it has never been created or either
    // of the endpoints has been changed
    if (!shmctrlr_handle_ || frPublisherString != frReleaseEndpoint_ || frSubscriberString != frReadyEndpoint_) {
        try {
            // Release the current shared memory controller if one exists
            if (shmctrlr_handle_) {
                shmctrlr_handle_.reset();
            }
            // Create the new shared memory controller and give it the parser and publisher
            shmctrlr_handle_
                = boost::make_shared<SharedMemoryController>(reactor_, frSubscriberString, frPublisherString);
            shmctrlr_handle_->inject_process_frame_cb(
                boost::bind(&SharedMemoryPlugin::process_frame, this, boost::placeholders::_1)
            );
            frReadyEndpoint_ = frSubscriberString;
            frReleaseEndpoint_ = frPublisherString;
        } catch (const boost::interprocess::interprocess_exception& e) {
            LOG4CXX_ERROR(logger_, "Unable to access shared memory: \n"); // << e.what());
        }
    } else {
        LOG4CXX_ERROR(logger_, "*** Not updating shared memory, endpoints were not changed");
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
    reply.set_param(this->get_name() + '/' + SharedMemoryPlugin::STATUS_SHB_NAME, shmctrlr_handle_->getName());
    reply.set_param(
        this->get_name() + '/' + SharedMemoryPlugin::STATUS_SHB_CONFIGURED, shmctrlr_handle_->isConfigured()
    );
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

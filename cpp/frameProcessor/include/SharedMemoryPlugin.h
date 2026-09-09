/*
 * SharedMemoryPlugin.h
 *
 *  Created on: 08 Sep. 2026
 *      Author: Famous Alele
 */

#ifndef SHAREDMEMORYPLUGIN_H_
#define SHAREDMEMORYPLUGIN_H_
#include <log4cxx/logger.h>

using namespace log4cxx;

#include "ClassLoader.h"
#include "FrameProcessorPlugin.h"
#include "IpcReactor.h"
#include "SharedMemoryController.h"

namespace FrameProcessor {
class SharedMemoryPlugin : public FrameProcessorPlugin {
public:
    SharedMemoryPlugin();
    void process_frame(boost::shared_ptr<Frame> frame);
    void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
    void requestConfiguration(OdinData::IpcMessage& reply);
    void status(OdinData::IpcMessage& reply);
    int get_version_major() override;
    int get_version_minor() override;
    int get_version_patch() override;
    std::string get_version_short() override;
    std::string get_version_long() override;

    const static std::string CONFIG_FR_RELEASE;
    const static std::string CONFIG_FR_READY;
    const static std::string STATUS_SHB_NAME;
    const static std::string STATUS_SHB_CONFIGURED;

private:
    std::string frReleaseEndpoint_;
    std::string frReadyEndpoint_;
    boost::shared_ptr<OdinData::IpcReactor> reactor_;

    /** IpcReactor thread */
    boost::thread m_thread_;
    /** The shared memory controller object */
    boost::shared_ptr<SharedMemoryController> shmctrlr_handle_;
    /** Pointer to logger */
    LoggerPtr logger_;

    static void dummy_timer()
    {
    }
    void setupFrameReceiverInterface(const std::string&, const std::string&);
};
}

#endif // end SHAREDMEMORYPLUGIN_H_

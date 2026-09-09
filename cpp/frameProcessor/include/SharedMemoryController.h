/*
 * SharedMemoryController.h
 *
 *  Created on: 31 May 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_SHAREDMEMORYCONTROLLER_H_
#define TOOLS_FILEWRITER_SHAREDMEMORYCONTROLLER_H_

#include <log4cxx/basicconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/logger.h>
#include <log4cxx/propertyconfigurator.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "boost/date_time/posix_time/posix_time.hpp"

#include "IFrameCallback.h"
#include "IpcChannel.h"
#include "IpcMessage.h"
#include "IpcReactor.h"
#include "SharedBufferManager.h"

namespace FrameProcessor {

/**
 * The SharedMemoryController class uses an IpcReactor object which is used
 * to notify this class when new data is available from the
 * frame receiver service. This class also owns an instance of the
 * SharedMemoryParser class, which extracts the data from the shared memory
 * location specified by the incoming IpcMessage objects, constructs a
 * Frame to contain the data and meta data, and then notifies any listening
 * plugins. This class also notifies the frame receiver service once the
 * shared memory location is available for re-use.
 */
class SharedMemoryController {
    typedef boost::function<void(boost::shared_ptr<Frame> frame)> TProcess_frame_cb;

public:
    SharedMemoryController(
        boost::shared_ptr<OdinData::IpcReactor> reactor,
        const std::string& rxEndPoint,
        const std::string& txEndPoint
    );
    virtual ~SharedMemoryController();
    void setSharedBufferManager(std::string& shared_buffer_name);
    void requestSharedBufferConfig(const bool deferred = false);
    void registerCallback(const std::string& name, boost::shared_ptr<IFrameCallback> cb);
    void removeCallback(const std::string& name);
    void handleRxChannel();
    void status(OdinData::IpcMessage& status);
    void injectEOA();
    const std::string& getName() const
    {
        return shbName_;
    }
    bool isConfigured() const
    {
        return sharedBufferConfigured_;
    }
    void inject_process_frame_cb(TProcess_frame_cb callback)
    {
        callback_ = callback;
    }

private:
    /** Pointer to logger */
    LoggerPtr logger_;
    /** Pointer to SharedBufferManager object */
    boost::shared_ptr<OdinData::SharedBufferManager> sbm_;
    /** Map of IFrameCallback pointers, indexed by name */
    std::map<std::string, boost::shared_ptr<IFrameCallback>> callbacks_;
    /** IpcReactor pointer, for managing IpcMessage objects */
    boost::shared_ptr<OdinData::IpcReactor> reactor_;
    /** IpcChannel for receiving notifications of new frames */
    OdinData::IpcChannel rxChannel_;
    /** IpcChannel for sending notifications of frame release */
    OdinData::IpcChannel txChannel_;
    /** Shared buffer name */
    std::string shbName_;
    /** Shared buffer configured status flag */
    bool sharedBufferConfigured_;
    /** Shared buffer config request deferred flag */
    bool sharedBufferConfigRequestDeferred_;

    TProcess_frame_cb callback_;

    /** Name of class used in status messages */
    static const std::string SHARED_MEMORY_CONTROLLER_NAME;
};

} /* namespace FrameProcessor */

#endif /* TOOLS_FILEWRITER_SHAREDMEMORYCONTROLLER_H_ */

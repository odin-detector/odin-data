/*
 * RawFileWriterPlugin.h
 *
 *  Created on: 10 Feb 2026
 *      Author: Famous Alele
 */

#ifndef RAWFILEWRITERPLUGIN_H_
#define RAWFILEWRITERPLUGIN_H_
#include <log4cxx/logger.h>
using namespace log4cxx;

#include <boost/filesystem.hpp> // boost::filesystem::path

#include "ClassLoader.h"
#include "FrameProcessorPlugin.h"

namespace FrameProcessor {

class RawFileWriterPlugin : public FrameProcessorPlugin {
public:
    RawFileWriterPlugin();
    void process_frame(std::shared_ptr<Frame> frame);
    void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
    void requestConfiguration(OdinData::IpcMessage& reply);
    void status(OdinData::IpcMessage& reply);
    int get_version_major() override;
    int get_version_minor() override;
    int get_version_patch() override;
    std::string get_version_short() override;
    std::string get_version_long() override;

    const static std::string CONFIG_FILE_PATH;
    const static std::string CONFIG_ENABLED;
    const static std::string STATUS_DROPPED_FRAMES;

private:
    /** Mutex used to make this class thread safe */
    std::recursive_mutex mutex_;
    /** Pointer to logger */
    LoggerPtr logger_;
    /** Flag to enable or disable file writing */
    bool enabled_;
    /** Root path to write files to - files will be created in this directory or a nested directory, depending on Frame
     * properties */
    boost::filesystem::path file_path_;
    /** Number of dropped frames */
    std::size_t dropped_frames_;
};

};

#endif

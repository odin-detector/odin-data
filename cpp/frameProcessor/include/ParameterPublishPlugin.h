/*
 *  Created on: 22 Nov 2021
 *      Author: Gary Yendell
 */

#ifndef PARAMETERPUBLISHPLUGIN_H
#define PARAMETERPUBLISHPLUGIN_H

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

#include "IpcChannel.h"

#include "FrameProcessorPlugin.h"

/**
 * This plugin class looks for the configured parameters on a Frame and publishes them over ZMQ
 */
namespace FrameProcessor {

  class ParameterPublishPlugin : public FrameProcessorPlugin {
    public:
      ParameterPublishPlugin();
      ~ParameterPublishPlugin();

      void process_frame(boost::shared_ptr<Frame> frame);
      void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);

      int get_version_major();
      int get_version_minor();
      int get_version_patch();
      std::string get_version_short();
      std::string get_version_long();

      // Config message keys
      static const std::string CONFIG_ENDPOINT;
      static const std::string CONFIG_ADD_PARAMETER;
      // Data message keys
      static const std::string DATA_FRAME_NUMBER;
      static const std::string DATA_PARAMETERS;

    private:
      /** Pointer to logger */
      LoggerPtr logger_;
      /** Parameters to publish */
      std::set<std::string> parameters_;
      /** Configured endpoint messages are published on */
      std::string channel_endpoint_;
      /** IpcChannel for publishing messages */
      OdinData::IpcChannel publish_channel_;

      void setup_publish_channel(const std::string& endpoint);
      void requestConfiguration(OdinData::IpcMessage& reply);
  };

} /* namespace FrameProcessor */

#endif //PARAMETERPUBLISHPLUGIN_H

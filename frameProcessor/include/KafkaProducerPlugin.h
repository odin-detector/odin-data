//
// Created by hir12111 on 25/03/19.
//

#ifndef KAFKAPRODUCERPLUGIN_H
#define KAFKAPRODUCERPLUGIN_H

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameProcessorPlugin.h"

/**
 * This plugin class calculates the sum of each pixel and
 * adds it as a parameter
 *
 */
namespace FrameProcessor {

  class KafkaProducer : public FrameProcessorPlugin {
  public:
    KafkaProducer();

    ~KafkaProducer();

    void process_frame(boost::shared_ptr <Frame> frame);

    int get_version_major();

    int get_version_minor();

    int get_version_patch();

    std::string get_version_short();

    std::string get_version_long();

  private:
    /** Pointer to logger */
    LoggerPtr logger_;
  };

} /* namespace FrameProcessor */

#endif //KAFKAPRODUCERPLUGIN_H

/*
 * PercivalProcessPlugin.h
 *
 *  Created on: 7 Jun 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_PERCIVALPROCESSPLUGIN_H_
#define TOOLS_FILEWRITER_PERCIVALPROCESSPLUGIN_H_

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameProcessorPlugin.h"
#include "PercivalEmulatorDefinitions.h"
#include "ClassLoader.h"

namespace FrameProcessor
{

  /** Processing of Percival Frame objects.
   *
   * The PercivalProcessPlugin class is currently responsible for receiving a raw data
   * Frame object and parsing the header information, before splitting the raw data into
   * the two "data" and "reset" Frame objects.
   */
  class PercivalProcessPlugin : public FileWriterPlugin
  {
  public:
    PercivalProcessPlugin();
    virtual ~PercivalProcessPlugin();

  private:
    void processFrame(boost::shared_ptr<Frame> frame);

    /** Pointer to logger */
    LoggerPtr logger_;
  };

  /**
   * Registration of this plugin through the ClassLoader.  This macro
   * registers the class without needing to worry about name mangling
   */
  REGISTER(FileWriterPlugin, PercivalProcessPlugin, "PercivalProcessPlugin");

} /* namespace FrameProcessor */

#endif /* TOOLS_FILEWRITER_PERCIVALPROCESSPLUGIN_H_ */

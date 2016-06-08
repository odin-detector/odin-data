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

#include "FileWriterPlugin.h"
#include "PercivalEmulatorDefinitions.h"
#include "ClassLoader.h"

namespace filewriter
{

  class PercivalProcessPlugin : public FileWriterPlugin
  {
  public:
    PercivalProcessPlugin();
    virtual ~PercivalProcessPlugin();

  private:
    void processFrame(boost::shared_ptr<Frame> frame);

    LoggerPtr logger_;
  };

  /**
   * Registration of this plugin through the ClassLoader.  This macro
   * registers the class without needing to worry about name mangling
   */
  REGISTER(FileWriterPlugin, PercivalProcessPlugin, "PercivalProcessPlugin");

} /* namespace filewriter */

#endif /* TOOLS_FILEWRITER_PERCIVALPROCESSPLUGIN_H_ */

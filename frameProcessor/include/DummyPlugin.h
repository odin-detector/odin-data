/*
 * DummyPlugin.h
 *
 *  Created on: 2 Jun 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_DUMMYPLUGIN_H_
#define TOOLS_FILEWRITER_DUMMYPLUGIN_H_

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameProcessorPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{

/**
 * This is a very simple example of a plugin class to demonstrate how to
 * create plugins for the frame processor.
 *
 * When this plugin receives a frame, processFrame is called and the class
 * simply logs that a frame has been passed to it.
 */
class DummyPlugin : public FrameProcessorPlugin
{
public:
  DummyPlugin();
  virtual ~DummyPlugin();

private:
  void processFrame(boost::shared_ptr<Frame> frame);

  /** Pointer to logger */
  LoggerPtr logger_;
};

/**
 * Registration of this plugin through the ClassLoader. This macro
 * registers the class without needing to worry about name mangling
 */
REGISTER(FrameProcessorPlugin, DummyPlugin, "DummyPlugin");

} /* namespace FrameProcessor */

#endif /* TOOLS_FILEWRITER_DUMMYPLUGIN_H_ */

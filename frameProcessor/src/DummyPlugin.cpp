/*
 * DummyPlugin.cpp
 *
 *  Created on: 2 Jun 2016
 *      Author: gnx91527
 */

#include "DummyPlugin.h"
#include "version.h"

namespace FrameProcessor
{

/**
 * The constructor sets up logging used within the class.
 */
DummyPlugin::DummyPlugin()
{
  // Setup logging for the class
  logger_ = Logger::getLogger("FW.DummyPlugin");
  logger_->setLevel(Level::getAll());
  LOG4CXX_INFO(logger_, "DummyPlugin version " << this->get_version_long() << " loaded");
}

/**
 * Destructor.
 */
DummyPlugin::~DummyPlugin()
{
  LOG4CXX_TRACE(logger_, "DummyPlugin destructor.");
}

/**
 * Perform processing on the frame. For the DummyPlugin class we are
 * simply going to log that we have received a frame.
 *
 * \param[in] frame - Pointer to a Frame object.
 */
void DummyPlugin::process_frame(boost::shared_ptr<Frame> frame)
{
  LOG4CXX_TRACE(logger_, "Received a new frame...");
}

int DummyPlugin::get_version_major()
{
  return ODIN_DATA_VERSION_MAJOR;
}

int DummyPlugin::get_version_minor()
{
  return ODIN_DATA_VERSION_MINOR;
}

int DummyPlugin::get_version_patch()
{
  return ODIN_DATA_VERSION_PATCH;
}

std::string DummyPlugin::get_version_short()
{
  return ODIN_DATA_VERSION_STR_SHORT;
}

std::string DummyPlugin::get_version_long()
{
  return ODIN_DATA_VERSION_STR;
}

} /* namespace FrameProcessor */

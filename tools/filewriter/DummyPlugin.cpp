/*
 * DummyPlugin.cpp
 *
 *  Created on: 2 Jun 2016
 *      Author: gnx91527
 */

#include "DummyPlugin.h"

namespace filewriter
{

  /** Constructor.
   * The constructor sets up logging used within the class.
   */
  DummyPlugin::DummyPlugin()
  {
    // Setup logging for the class
    logger_ = Logger::getLogger("DummyPlugin");
    logger_->setLevel(Level::getAll());
    LOG4CXX_TRACE(logger_, "DummyPlugin constructor.");
  }

  DummyPlugin::~DummyPlugin()
  {
    LOG4CXX_TRACE(logger_, "DummyPlugin destructor.");
  }

  void DummyPlugin::processFrame(boost::shared_ptr<Frame> frame)
  {

  }

} /* namespace filewriter */

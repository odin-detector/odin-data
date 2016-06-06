/*
 * FileWriterPlugin.h
 *
 *  Created on: 2 Jun 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_FILEWRITERPLUGIN_H_
#define TOOLS_FILEWRITER_FILEWRITERPLUGIN_H_

#include "IFrameCallback.h"
#include "JSONMessage.h"

namespace filewriter
{

  class FileWriterPlugin : public IFrameCallback
  {
  public:
    FileWriterPlugin();
    virtual ~FileWriterPlugin();
    virtual boost::shared_ptr<filewriter::JSONMessage> configure(boost::shared_ptr<filewriter::JSONMessage> config);

  private:
    void callback(boost::shared_ptr<Frame> frame);
    virtual void processFrame(boost::shared_ptr<Frame> frame) = 0;
  };

} /* namespace filewriter */

#endif /* TOOLS_FILEWRITER_FILEWRITERPLUGIN_H_ */

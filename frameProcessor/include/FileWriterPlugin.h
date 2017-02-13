/*
 * FileWriterPlugin.h
 *
 *  Created on: 2 Jun 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_FILEWRITERPLUGIN_H_
#define TOOLS_FILEWRITER_FILEWRITERPLUGIN_H_

#include "IFrameCallback.h"
#include "IpcMessage.h"

namespace filewriter
{

  /** Abstract plugin class, providing the IFrameCallback interface.
   *
   * All file writer plugins must subclass this class.  It provides the
   * IFrameCallback interface and associated WorkQueue for transferring
   * Frame objects between plugins.  It also provides methods for configuring
   * plugins and for retrieving status from plugins.
   */
  class FileWriterPlugin : public IFrameCallback
  {
  public:
    FileWriterPlugin();
    virtual ~FileWriterPlugin();
    void setName(const std::string& name);
    std::string getName();
    virtual void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
    virtual void status(OdinData::IpcMessage& status);
    void registerCallback(const std::string& name, boost::shared_ptr<IFrameCallback> cb);
    void removeCallback(const std::string& name);

  protected:
    void push(boost::shared_ptr<Frame> frame);

  private:
    void callback(boost::shared_ptr<Frame> frame);

    /**
     * This is called by the callback method when any new frames have
     * arrived and must be overridden by child classes.
     *
     * \param[in] frame - Pointer to the frame.
     */
    virtual void processFrame(boost::shared_ptr<Frame> frame) = 0;

    /** Name of this plugin */
    std::string name_;
    /** Map of registered plugins for callbacks, indexed by name */
    std::map<std::string, boost::shared_ptr<IFrameCallback> > callbacks_;
  };

} /* namespace filewriter */

#endif /* TOOLS_FILEWRITER_FILEWRITERPLUGIN_H_ */

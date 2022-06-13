/*
 * MetaMessagePublisher.h
 *
 *  Created on: 2 Nov 2017
 *      Author: vtu42223
 */

#ifndef FRAMEPROCESSOR_SRC_METAMESSAGEPUBLISHER_H_
#define FRAMEPROCESSOR_SRC_METAMESSAGEPUBLISHER_H_

#include "MetaMessage.h"
#include "IpcMessage.h"
#include "IpcChannel.h"

namespace FrameProcessor {

class MetaMessagePublisher {
public:
  MetaMessagePublisher();
  ~MetaMessagePublisher();

  void connect_meta_channel();
  void publish_meta(const std::string name, const std::string& item, int32_t value, const std::string& header = "");
  void publish_meta(const std::string name, const std::string& item, uint64_t value, const std::string &header = "");
  void publish_meta(const std::string name, const std::string& item, double value, const std::string& header = "");
  void publish_meta(const std::string name, const std::string& item, const std::string& value, const std::string& header = "");
  void publish_meta(const std::string name, const std::string& item, const void *pValue, size_t length, const std::string& header = "");

private:

  /** Configuration constant for the meta-data Rx interface **/
  static const std::string META_RX_INTERFACE;
  /** IpcChannel for meta-data messages */
  OdinData::IpcChannel meta_channel_;
};

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_SRC_METAMESSAGEPUBLISHER_H_ */

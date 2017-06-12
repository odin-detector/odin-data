/*
 * MetaMessage.h
 *
 *  Created on: 5 May 2017
 *      Author: gnx91527
 */

#ifndef FRAMEPROCESSOR_SRC_METAMESSAGE_H_
#define FRAMEPROCESSOR_SRC_METAMESSAGE_H_

#include <string>
#include <string.h>
#include <stdlib.h>

namespace FrameProcessor
{

class MetaMessage {
public:
  MetaMessage(const std::string& name, const std::string& item, const std::string& type, const std::string& header, size_t size, const void *dPtr);
  virtual ~MetaMessage();
  std::string getName();
  std::string getItem();
  std::string getType();
  std::string getHeader();
  size_t getSize();
  void *getDataPtr();

private:
  std::string name_;
  std::string item_;
  std::string type_;
  std::string header_;
  size_t size_;
  void *dPtr_;
};

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_SRC_METAMESSAGE_H_ */

/*
 * MetaMessage.cpp
 *
 *  Created on: 5 May 2017
 *      Author: gnx91527
 */

#include "MetaMessage.h"

namespace FrameProcessor {

MetaMessage::MetaMessage(const std::string& name,
                         const std::string& item,
                         const std::string& type,
                         const std::string& header,
                         size_t size,
                         const void *dPtr) :
    name_(name),
    item_(item),
    type_(type),
    header_(header),
    size_(size)
{
  // Allocate the memory for storing the meta data item
  dPtr_ = malloc(size);
  // Copy the meta data item into the allocated memory
  memcpy(dPtr_, dPtr, size);
}

MetaMessage::~MetaMessage()
{
  if (dPtr_) {
    free(dPtr_);
  }
}

std::string MetaMessage::getName()
{
  return name_;
}

std::string MetaMessage::getItem()
{
  return item_;
}

std::string MetaMessage::getType()
{
  return type_;
}

std::string MetaMessage::getHeader()
{
  return header_;
}

size_t MetaMessage::getSize()
{
  return size_;
}

void *MetaMessage::getDataPtr()
{
  return dPtr_;
}

} /* namespace FrameProcessor */

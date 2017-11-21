/*
 * MetaMessagePublisher.cpp
 *
 *  Created on: 2 Nov 2017
 *      Author: vtu42223
 */

#include "MetaMessagePublisher.h"

namespace FrameProcessor {

const std::string MetaMessagePublisher::META_RX_INTERFACE = "inproc://meta_rx";

MetaMessagePublisher::MetaMessagePublisher() :
  meta_channel_(ZMQ_PUSH)
{
}

MetaMessagePublisher::~MetaMessagePublisher() {
}

void MetaMessagePublisher::connect_meta_channel()
{
  meta_channel_.connect(META_RX_INTERFACE.c_str());
}

/** Publish meta data from this plugin.
 *
 * \param[in] item - Name of the meta data item to publish.
 * \param[in] value - The value of the meta data item to publish.
 * \param[in] header - Optional additional header data to publish.
 */
void MetaMessagePublisher::publish_meta(const std::string name, const std::string& item, int32_t value, const std::string &header)
{
  // Create a new MetaMessage object and send to the consumer
  FrameProcessor::MetaMessage *meta = new FrameProcessor::MetaMessage(name, item, "integer", header, sizeof(int32_t), &value);
  // We need the pointer to the object cast to be able to pass it through ZMQ
  uintptr_t addr = reinterpret_cast<uintptr_t>(&(*meta));
  // Send the pointer value to the listener
  meta_channel_.send(sizeof(uintptr_t), &addr, ZMQ_DONTWAIT);
}

void MetaMessagePublisher::publish_meta(const std::string name, const std::string& item, uint64_t value, const std::string &header)
{
  // Create a new MetaMessage object and send to the consumer
  FrameProcessor::MetaMessage *meta = new FrameProcessor::MetaMessage(name, item, "uint64", header, sizeof(uint64_t), &value);
  // We need the pointer to the object cast to be able to pass it through ZMQ
  uintptr_t addr = reinterpret_cast<uintptr_t>(&(*meta));
  // Send the pointer value to the listener
  meta_channel_.send(sizeof(uintptr_t), &addr, ZMQ_DONTWAIT);
}

void MetaMessagePublisher::publish_meta(const std::string name, const std::string& item, double value, const std::string& header)
{
  // Create a new MetaMessage object and send to the consumer
  FrameProcessor::MetaMessage *meta = new FrameProcessor::MetaMessage(name, item, "double", header, sizeof(double), &value);
  // We need the pointer to the object cast to be able to pass it through ZMQ
  uintptr_t addr = reinterpret_cast<uintptr_t>(&(*meta));
  // Send the pointer value to the listener
  meta_channel_.send(sizeof(uintptr_t), &addr, ZMQ_DONTWAIT);
}

void MetaMessagePublisher::publish_meta(const std::string name, const std::string& item, const std::string& value, const std::string& header)
{
  // Create a new MetaMessage object and send to the consumer
  FrameProcessor::MetaMessage *meta = new FrameProcessor::MetaMessage(name, item, "string", header, value.length(), value.c_str());
  // We need the pointer to the object cast to be able to pass it through ZMQ
  uintptr_t addr = reinterpret_cast<uintptr_t>(&(*meta));
  // Send the pointer value to the listener
  meta_channel_.send(sizeof(uintptr_t), &addr, ZMQ_DONTWAIT);
}

/**
 * \param[in] pValue - The pointer to data blob to publish.
 * \param[in] length - Length of data blob.
 */
void MetaMessagePublisher::publish_meta(const std::string name, const std::string& item, const void *pValue, size_t length, const std::string& header)
{
  // Create a new MetaMessage object and send to the consumer
  FrameProcessor::MetaMessage *meta = new FrameProcessor::MetaMessage(name, item, "raw", header, length, pValue);
  // We need the pointer to the object cast to be able to pass it through ZMQ
  uintptr_t addr = reinterpret_cast<uintptr_t>(&(*meta));
  // Send the pointer value to the listener
  meta_channel_.send(sizeof(uintptr_t), &addr, ZMQ_DONTWAIT);
}

} /* namespace FrameProcessor */

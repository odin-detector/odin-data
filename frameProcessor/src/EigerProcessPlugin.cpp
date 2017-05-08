/*
 * EigerProcessPlugin.cpp
 *
 *  Created on: 7 Jun 2016
 *      Author: gnx91527
 */

#include <EigerProcessPlugin.h>

#define RAPIDJSON_HAS_STDSTRING 1

namespace filewriter
{
  void sendMetaMessage() {

  }

  EigerProcessPlugin::EigerProcessPlugin()
  {
    // Setup logging for the class
    logger_ = Logger::getLogger("FW.EigerProcessPlugin");
    logger_->setLevel(Level::getAll());
    LOG4CXX_TRACE(logger_, "EigerProcessPlugin constructor.");
  }

  EigerProcessPlugin::~EigerProcessPlugin()
  {
    // TODO Auto-generated destructor stub
  }

  void EigerProcessPlugin::processFrame(boost::shared_ptr<Frame> frame)
  {
	  const filewriter::EigerFrameHeader* hdrPtr = static_cast<const EigerFrameHeader*>(frame->get_data());
	  LOG4CXX_TRACE(logger_, "aFrameHeader frame currentMessageType: " << hdrPtr->currentMessageType);
	  LOG4CXX_TRACE(logger_, "FrameHeader frame number: " << hdrPtr->frame_number);
	  LOG4CXX_TRACE(logger_, "FrameHeader frame shapeSizeX: " << hdrPtr->shapeSizeX);
	  LOG4CXX_TRACE(logger_, "FrameHeader frame shapeSizeY: " << hdrPtr->shapeSizeY);
	  LOG4CXX_TRACE(logger_, "FrameHeader frame shapeSizeZ: " << hdrPtr->shapeSizeZ);
	  LOG4CXX_TRACE(logger_, "FrameHeader frame startTime: " << hdrPtr->startTime);
	  LOG4CXX_TRACE(logger_, "FrameHeader frame stopTime: " << hdrPtr->stopTime);
	  LOG4CXX_TRACE(logger_, "FrameHeader frame realTime: " << hdrPtr->realTime);
	  LOG4CXX_TRACE(logger_, "FrameHeader frame blob_size: " << hdrPtr->data_size);
	  LOG4CXX_TRACE(logger_, "FrameHeader frame data type: " << hdrPtr->dataType);
	  LOG4CXX_TRACE(logger_, "FrameHeader frame encoding: " << hdrPtr->encoding);

	  if (hdrPtr->currentMessageType == IMAGE_DATA) {

		boost::shared_ptr<Frame> data_frame;
		data_frame = boost::shared_ptr<Frame>(new Frame("data"));
		data_frame->set_frame_number(hdrPtr->frame_number);

		data_frame->copy_data((static_cast<const char*>(frame->get_data())+sizeof(EigerFrameHeader)), hdrPtr->data_size);

		setFrameEncoding(data_frame, hdrPtr);
		setFrameDataType(data_frame, hdrPtr);
		setFrameDimensions(data_frame, hdrPtr);

		this->push(data_frame);

	  } else if (hdrPtr->currentMessageType == END_OF_STREAM) {
		boost::shared_ptr<Frame> meta_frame;
		meta_frame = boost::shared_ptr<Frame>(new Frame("meta"));
		meta_frame->set_parameter("stop", 1);
		this->push(meta_frame);
	  }

  }

  void EigerProcessPlugin::setFrameEncoding(boost::shared_ptr<Frame> frame, const filewriter::EigerFrameHeader* hdrPtr) {

	  std::string encoding(hdrPtr->encoding);

	  // Parse out lz4
	  std::size_t found = encoding.find("lz4");
	  if (found != std::string::npos) {
		  found = encoding.find("bs");
		  if (found != std::string::npos) {
			  frame->set_parameter("compression", LZ4_BITSHUFFLE_COMPRESSION);
		  } else {
			  frame->set_parameter("compression", LZ4_COMPRESSION);
		  }
	  }
  }

  void EigerProcessPlugin::setFrameDataType(boost::shared_ptr<Frame> frame, const EigerFrameHeader* hdrPtr) {

	  std::string dataType(hdrPtr->dataType);

	  if (dataType.compare("uint8") == 0) {
		  frame->set_parameter("dataType", 8);
	  } else if (dataType.compare("uint16") == 0) {
		  frame->set_parameter("dataType", 16);
	  } else if (dataType.compare("uint32") == 0) {
		  frame->set_parameter("dataType", 32);
	  } else {
		  LOG4CXX_ERROR(logger_, "Unknown frame data type :" << dataType);
	  }

  }

  void EigerProcessPlugin::setFrameDimensions(boost::shared_ptr<Frame> frame, const EigerFrameHeader* hdrPtr) {

	  if (hdrPtr->shapeSizeZ > 0) {
		dimensions_t p2m_dims(3);
		p2m_dims[0] = hdrPtr->shapeSizeX;
		p2m_dims[1] = hdrPtr->shapeSizeY;
		p2m_dims[2] = hdrPtr->shapeSizeZ;
		frame->set_dimensions("frame", p2m_dims);
	  } else {
		dimensions_t p2m_dims(2);
		p2m_dims[0] = hdrPtr->shapeSizeX;
		p2m_dims[1] = hdrPtr->shapeSizeY;
		frame->set_dimensions("frame", p2m_dims);
	  }
  }

} /* namespace filewriter */

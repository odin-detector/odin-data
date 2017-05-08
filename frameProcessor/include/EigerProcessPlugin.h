/*
 * EigerProcessPlugin.h
 *
 *  Created on: 7 Jun 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_EIGERPROCESSPLUGIN_H_
#define TOOLS_FILEWRITER_EIGERPROCESSPLUGIN_H_

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FileWriterPlugin.h"
#include "ClassLoader.h"

namespace filewriter
{
    enum EigerMessageType { GLOBAL_HEADER_NONE, GLOBAL_HEADER_CONFIG, GLOBAL_HEADER_FLATFIELD, GLOBAL_HEADER_MASK, GLOBAL_HEADER_COUNTRATE, IMAGE_DATA, END_OF_STREAM};

    enum CompressionType { LZ4_COMPRESSION, LZ4_BITSHUFFLE_COMPRESSION };

	typedef struct
	{
		EigerMessageType currentMessageType;
		uint32_t frame_number;

		uint32_t shapeSizeX;
		uint32_t shapeSizeY;
		uint32_t shapeSizeZ;

		uint64_t startTime;
		uint64_t stopTime;
		uint64_t realTime;

		uint32_t data_size;

		char encoding[11];  // String of the form "[bs<BIT>][[-]lz4][<|>]"
		char dataType[8];	// "uint16" or "uint32" or "float32"
	} EigerFrameHeader;

  /** Processing of Eiger Frame objects.
   *
   * The EigerProcessPlugin class is currently responsible for receiving a raw data
   * Frame object and parsing the header information, before splitting the raw data into
   * the two "data" and "reset" Frame objects.
   */
  class EigerProcessPlugin : public FileWriterPlugin
  {
  public:
    EigerProcessPlugin();
    virtual ~EigerProcessPlugin();

  private:
    void processFrame(boost::shared_ptr<Frame> frame);
    void setFrameEncoding(boost::shared_ptr<Frame> frame, const EigerFrameHeader* hdrPtr);
    void setFrameDataType(boost::shared_ptr<Frame> frame, const EigerFrameHeader* hdrPtr);
    void setFrameDimensions(boost::shared_ptr<Frame> frame, const EigerFrameHeader* hdrPtr);


    /** Pointer to logger */
    LoggerPtr logger_;
  };

  /**
   * Registration of this plugin through the ClassLoader.  This macro
   * registers the class without needing to worry about name mangling
   */
  REGISTER(FileWriterPlugin, EigerProcessPlugin, "EigerProcessPlugin");

} /* namespace filewriter */

#endif /* TOOLS_FILEWRITER_EIGERPROCESSPLUGIN_H_ */

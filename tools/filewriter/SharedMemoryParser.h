/*
 * SharedMemoryParser.h
 *
 *  Created on: 31 May 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_SHAREDMEMORYPARSER_H_
#define TOOLS_FILEWRITER_SHAREDMEMORYPARSER_H_

#include <string>
#include <stdint.h>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <log4cxx/logger.h>

#include "Frame.h"
//#include "PercivalEmulatorDefinitions.h"

// Shared Buffer (IPC) Header
//typedef struct
//{
//    size_t manager_id;
//    size_t num_buffers;
//    size_t buffer_size;
//} Header;

typedef unsigned long long dimsize_t;
typedef std::vector<dimsize_t> dimensions_t;

namespace filewriter
{

  class SharedMemoryParser
  {
  public:
    SharedMemoryParser(const std::string & shared_mem_name);
    ~SharedMemoryParser();
    void get_frame(Frame& dest_frame, unsigned int buffer_id);
    size_t get_buffer_size();
    const void* get_buffer_address(unsigned int bufferid) const;

  private:
    SharedMemoryParser();
    SharedMemoryParser(const SharedMemoryParser& src); // Don't copy one of these!

    log4cxx::LoggerPtr logger;
    boost::interprocess::shared_memory_object shared_mem;
    boost::interprocess::mapped_region        shared_mem_region;
    Header*                                   shared_mem_header;
  };

} /* namespace filewriter */

#endif /* TOOLS_FILEWRITER_SHAREDMEMORYPARSER_H_ */

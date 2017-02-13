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

typedef unsigned long long dimsize_t;
typedef std::vector<dimsize_t> dimensions_t;

namespace filewriter
{

  /** Parses shared memory buffer and populates Frame objects.
   *
   * The SharedMemoryParser class manages the boost shared memory objects,
   * and is used to extract raw data from shared memory buffers and copy the
   * data into Frame objects for further processing.
   */
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
    /**
     * Do not allow Frame copy
     */
    SharedMemoryParser(const SharedMemoryParser& src); // Don't copy one of these!

    /** Pointer to logger */
    log4cxx::LoggerPtr logger;
    /** Shared memory object */
    boost::interprocess::shared_memory_object shared_mem;
    /** Shared memory region object */
    boost::interprocess::mapped_region        shared_mem_region;
    /** IPC shared memory header */
    Header*                                   shared_mem_header;
  };

} /* namespace filewriter */

#endif /* TOOLS_FILEWRITER_SHAREDMEMORYPARSER_H_ */

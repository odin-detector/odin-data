/*
 * SharedMemoryParser.cpp
 *
 *  Created on: 31 May 2016
 *      Author: gnx91527
 */

#include <SharedMemoryParser.h>

namespace filewriter
{

  /** Constructor.
   *
   * The constructor sets up logging used within the class.  It also opens the shared
   * memory buffer and maps the specific region used by the IPC classes.  Memory is
   * allocated for the shared memory buffer header information and this is memcopied
   * from the shared memory location into the allocated program memory.
   *
   * \param[in] shared_mem_name - name of the shared memory buffer.
   */
  SharedMemoryParser::SharedMemoryParser(const std::string& shared_mem_name) :
    shared_mem(boost::interprocess::open_only, shared_mem_name.c_str(), boost::interprocess::read_write),
    logger(log4cxx::Logger::getLogger("SharedMemParser")),
    shared_mem_header(static_cast<Header*>(malloc(sizeof(Header))))
  {
    LOG4CXX_DEBUG(logger, "Registering shared memory region \"" << shared_mem_name << "\"");

    // Map the whole shared memory region into this process
    this->shared_mem_region = boost::interprocess::mapped_region(this->shared_mem, boost::interprocess::read_write);

    // Copy the buffer manager header - it's quite small
    memcpy(this->shared_mem_header, reinterpret_cast<const void*>(shared_mem_region.get_address()), sizeof(Header));
    LOG4CXX_DEBUG(logger, "Shared mem: buffers=" << this->shared_mem_header->num_buffers
                          << " bufsize=" << this->shared_mem_header->buffer_size
                          << " headersize=" << sizeof(Header));
  }

  /**
   * Destructor.
   */
  SharedMemoryParser::~SharedMemoryParser()
  {
    free(this->shared_mem_header);
  }

  /** Copy a raw data frame into a Frame object.
   *
   * The buffer_id value is used to determine which section of shared memory should
   * be copied.  The shared memory is memcopied into the Frame DataBlock of the
   * Frame reference passed into this method.  The address and buffer size are provided
   * by the shared memory header information.
   *
   * \param[out] dest_frame - reference to the Frame object to store the raw data.
   * \param[in] buffer_id - the ID of the shared memory buffer that contains the raw data.
   */
  void SharedMemoryParser::get_frame(Frame& dest_frame, unsigned int buffer_id)
  {
    LOG4CXX_DEBUG(logger, "get_frame called for buffer " << buffer_id);
    dest_frame.copy_data(this->get_buffer_address(buffer_id), this->get_buffer_size());
  }

  /** Return the size of a shared memory buffer in bytes.
   *
   * \return the size in bytes of a shared memory buffer.
   */
  size_t SharedMemoryParser::get_buffer_size()
  {
    return this->shared_mem_header->buffer_size;
  }

  /** Return a pointer to a shared memory buffer.
   *
   * \param[in] bufferid - the ID of the shared memory buffer.
   * \return a void pointer to the shared memory starting address.
   */
  const void * SharedMemoryParser::get_buffer_address(unsigned int bufferid) const
  {
    return static_cast<const void *>(static_cast<char*>(shared_mem_region.get_address())
                                     + sizeof(Header)
                                     + bufferid * shared_mem_header->buffer_size);
  }

} /* namespace filewriter */

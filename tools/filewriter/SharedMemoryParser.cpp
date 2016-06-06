/*
 * SharedMemoryParser.cpp
 *
 *  Created on: 31 May 2016
 *      Author: gnx91527
 */

#include <SharedMemoryParser.h>

namespace filewriter
{

SharedMemoryParser::SharedMemoryParser(const std::string& shared_mem_name)
: shared_mem(boost::interprocess::open_only, shared_mem_name.c_str(), boost::interprocess::read_write),
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
                          << " headersize=" << sizeof(Header)
                          << " frameheadersize=" << sizeof(PercivalEmulator::FrameHeader));
}

SharedMemoryParser::~SharedMemoryParser()
{
    free(this->shared_mem_header);
}

void SharedMemoryParser::get_frame(Frame& dest_frame, unsigned int buffer_id)
{
    dest_frame.copy_header(this->get_frame_header_address(buffer_id));
    dest_frame.copy_data(this->get_frame_data_address(buffer_id), PercivalEmulator::data_type_size);
}

void SharedMemoryParser::get_reset_frame(Frame& dest_frame, unsigned int buffer_id)
{
    dest_frame.copy_header(this->get_frame_header_address(buffer_id));
    dest_frame.copy_data(this->get_reset_data_address(buffer_id), PercivalEmulator::data_type_size);
}

size_t SharedMemoryParser::get_buffer_size()
{
    return this->shared_mem_header->buffer_size;
}

const void * SharedMemoryParser::get_buffer_address(unsigned int bufferid) const
{
    return static_cast<const void *>(
            static_cast<char*>(shared_mem_region.get_address())
            + sizeof(Header) + bufferid * shared_mem_header->buffer_size);
}

const void * SharedMemoryParser::get_frame_header_address(unsigned int bufferid) const
{
    return this->get_buffer_address(bufferid);
}

const void * SharedMemoryParser::get_reset_data_address(unsigned int bufferid) const
{
    return static_cast<const void *>(
            static_cast<const char*>(this->get_buffer_address(bufferid))
            + sizeof(PercivalEmulator::FrameHeader)
            + PercivalEmulator::data_type_size);
}

const void * SharedMemoryParser::get_frame_data_address(unsigned int bufferid) const
{
    return static_cast<const void *>(
            static_cast<const char*>(this->get_buffer_address(bufferid))
            + sizeof(PercivalEmulator::FrameHeader));
}

} /* namespace filewriter */

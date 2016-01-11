/**
framenotifier_data.cpp

  Created on: 15 Oct 2015
      Author: up45
*/

#include <cstring> // memcpy
#include <cstdlib> // malloc

#include "framenotifier_data.h"

using namespace boost::interprocess;

Frame::Frame(size_t buffer_size)
: data_size(buffer_size - sizeof(FrameHeader))
{
    this->data = static_cast<uint16_t*>(malloc( this->data_size ));
    this->header = static_cast<FrameHeader*>(malloc(sizeof(FrameHeader)));
}
Frame::~Frame()
{
    if (this->header != NULL) free (this->header);
    if (this->data != NULL) free (this->data);
}

void Frame::copy_data(const void* data_src)
{
    memcpy(this->data, data_src, this->data_size);
}

void Frame::copy_header(const void* header_src)
{
    memcpy(this->header, header_src, sizeof(FrameHeader));
}


SharedMemParser::SharedMemParser(const std::string& shared_mem_name)
: shared_mem(boost::interprocess::open_only, shared_mem_name.c_str(), boost::interprocess::read_write),
  logger(log4cxx::Logger::getLogger("DataMuncher")),
  shared_mem_header(static_cast<Header*>(malloc(sizeof(Header))))
{
    LOG4CXX_DEBUG(logger, "Registering shared memory region \"" << shared_mem_name << "\"");

    // Map the whole shared memory region into this process
    this->shared_mem_region = mapped_region(this->shared_mem, boost::interprocess::read_write);

    // Copy the buffer manager header - it's quite small
    memcpy(this->shared_mem_header, reinterpret_cast<const void*>(shared_mem_region.get_address()), sizeof(Header));
    LOG4CXX_DEBUG(logger, "Shared mem: buffers=" << this->shared_mem_header->num_buffers
                          << " bufsize=" << this->shared_mem_header->buffer_size
                          << " headersize=" << sizeof(Header)
                          << " frameheadersize=" << sizeof(FrameHeader));
}

SharedMemParser::~SharedMemParser()
{
    free(this->shared_mem_header);
}

void SharedMemParser::get_frame(Frame& dest_frame, unsigned int buffer_id)
{
    dest_frame.copy_header(this->get_frame_header_address(buffer_id));
    dest_frame.copy_data(this->get_frame_data_address(buffer_id));
}

size_t SharedMemParser::get_buffer_size()
{
    return this->shared_mem_header->buffer_size;
}

const void * SharedMemParser::get_buffer_address(unsigned int bufferid) const
{
    return static_cast<const void *>((static_cast<char*>(shared_mem_region.get_address()) + sizeof(Header)) + bufferid * shared_mem_header->buffer_size);
}

const void * SharedMemParser::get_frame_header_address(unsigned int bufferid) const
{
    return this->get_buffer_address(bufferid);
}

const void * SharedMemParser::get_frame_data_address(unsigned int bufferid) const
{
    return static_cast<const void *>(static_cast<const char*>(this->get_buffer_address(bufferid)) + sizeof(FrameHeader));
}

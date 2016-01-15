/**
framenotifier_data.cpp

  Created on: 15 Oct 2015
      Author: up45
*/

#include <cstring> // memcpy
#include <cstdlib> // malloc
#include <numeric> // accumulate

#include "framenotifier_data.h"

using namespace boost::interprocess;

Frame::Frame(size_t bytes_per_pixel, const dimensions_t& dimensions)
: bytes_per_pixel(bytes_per_pixel),
  dimensions(dimensions)
{
    this->buffer_allocated_bytes = this->get_data_size();
    this->data = malloc( this->buffer_allocated_bytes );
    this->frame_header = static_cast<FrameHeader*>(malloc(sizeof(FrameHeader)));
}
Frame::~Frame()
{
    if (this->frame_header != NULL) free (this->frame_header);
    if (this->data != NULL) free (this->data);
}

void Frame::copy_data(const void* data_src, size_t nbytes)
{
    assert(nbytes <= this->buffer_allocated_bytes);
    memcpy(this->data, data_src, nbytes);
}

void Frame::copy_header(const void* header_src)
{
    memcpy(this->frame_header, header_src, sizeof(FrameHeader));
}

size_t Frame::get_data_size() const
{
    dimsize_t npixels = 1;
    dimensions_t::const_iterator it;
    for (it = this->dimensions.begin(); it != this->dimensions.end(); ++it) npixels *= *it;
    size_t nbytes = this->bytes_per_pixel * static_cast<size_t>(npixels);
    return static_cast<size_t>(nbytes);
}

unsigned long long Frame::get_frame_number() const
{
    return static_cast<dimsize_t>(this->frame_header->frame_number);
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
    dest_frame.copy_data(this->get_frame_data_address(buffer_id),
            this->get_buffer_size() - sizeof(FrameHeader));
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


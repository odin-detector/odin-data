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
  dimensions(dimensions),
  logger(log4cxx::Logger::getLogger("Frame"))
{
    this->buffer_allocated_bytes = Frame::get_data_size(dimensions, bytes_per_pixel);
    LOG4CXX_DEBUG(logger, "Allocating frame buffer: "
                    << this->buffer_allocated_bytes << " bytes");
    this->data = calloc( 1, this->buffer_allocated_bytes );
    assert(this->data != NULL);
    LOG4CXX_DEBUG(logger, "Allocating FrameHeader buffer: "
                    << sizeof(FrameHeader) << " bytes");
    this->frame_header = static_cast<FrameHeader*>(calloc(1, sizeof(FrameHeader)));
    assert(this->frame_header != NULL);
}
Frame::~Frame()
{
    if (this->frame_header != NULL) free (this->frame_header);
    if (this->data != NULL) free (this->data);
}

void Frame::copy_data(const void* data_src, size_t nbytes)
{
    if (nbytes > this->buffer_allocated_bytes) {
        LOG4CXX_WARN(logger, "Trying to copy: " << nbytes
                            << " But allocated buffer only: "
                            << this->buffer_allocated_bytes
                            <<" bytes. Truncating copy.");
        nbytes = this->buffer_allocated_bytes;
    }
    memcpy(this->data, data_src, nbytes);
}

void Frame::copy_header(const void* header_src)
{
    memcpy(this->frame_header, header_src, sizeof(FrameHeader));
}

size_t Frame::get_data_size() const
{
    return Frame::get_data_size(this->dimensions, this->bytes_per_pixel);
}

size_t Frame::get_data_size(const dimensions_t dimensions, size_t bytes_per_pixel)
{
    dimsize_t npixels = 1;
    dimensions_t::const_iterator it;
    for (it = dimensions.begin(); it != dimensions.end(); ++it) npixels *= *it;
    size_t nbytes = bytes_per_pixel * static_cast<size_t>(npixels);
    return nbytes;
}

unsigned long long Frame::get_frame_number() const
{
    return static_cast<dimsize_t>(this->frame_header->frame_number);
}




SharedMemParser::SharedMemParser(const std::string& shared_mem_name)
: shared_mem(boost::interprocess::open_only, shared_mem_name.c_str(), boost::interprocess::read_write),
  logger(log4cxx::Logger::getLogger("SharedMemParser")),
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
    dest_frame.copy_data(this->get_frame_data_address(buffer_id),raw_frame_data_size);
}

void SharedMemParser::get_reset_frame(Frame& dest_frame, unsigned int buffer_id)
{
    dest_frame.copy_header(this->get_reset_header_address(buffer_id));
    dest_frame.copy_data(this->get_reset_data_address(buffer_id),raw_frame_data_size);
}

size_t SharedMemParser::get_buffer_size()
{
    return this->shared_mem_header->buffer_size;
}

const void * SharedMemParser::get_buffer_address(unsigned int bufferid) const
{
    return static_cast<const void *>(
            static_cast<char*>(shared_mem_region.get_address())
            + sizeof(Header) + bufferid * shared_mem_header->buffer_size);
}

const void * SharedMemParser::get_reset_header_address(unsigned int bufferid) const
{
    return this->get_buffer_address(bufferid);
}

const void * SharedMemParser::get_reset_data_address(unsigned int bufferid) const
{
    return static_cast<const void *>(
            static_cast<const char*>(this->get_buffer_address(bufferid))
            + sizeof(FrameHeader));
}

const void * SharedMemParser::get_frame_header_address(unsigned int bufferid) const
{
    return static_cast<const void *>(
            static_cast<const char*>(this->get_buffer_address(bufferid))
            + total_frame_size);
}

const void * SharedMemParser::get_frame_data_address(unsigned int bufferid) const
{
    return static_cast<const void *>(
            static_cast<const char*>(this->get_buffer_address(bufferid))
            + total_frame_size
            + sizeof(FrameHeader));
}

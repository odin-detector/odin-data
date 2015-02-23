/*!
 * SharedBufferManager.cpp
 *
 *  Created on: Feb 18, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "SharedBufferManager.h"

#include <iostream>
#include <sstream>

using namespace FrameReceiver;
using namespace boost::interprocess;

SharedBufferManager::SharedBufferManager(const std::string& shared_mem_name, const size_t shared_mem_size,
        const size_t buffer_size, bool remove_when_deleted) try :
    shared_mem_name_(shared_mem_name),
    shared_mem_size_(shared_mem_size),
    remove_when_deleted_(remove_when_deleted),
    shared_mem_(open_or_create, shared_mem_name_.c_str(), read_write),
    manager_hdr_(0)
{

    // Set the size of the shared memory object
    shared_mem_.truncate(sizeof(Header) + shared_mem_size_);

    // Map the whole shared memory region into this process
    shared_mem_region_ = mapped_region(shared_mem_, read_write);

    // Determine how many buffers of the requested size fit into the shared memory region
    size_t num_buffers = shared_mem_size_ / buffer_size;

    // Initialise the buffer manager header
    manager_hdr_ = reinterpret_cast<Header*>(shared_mem_region_.get_address());
    manager_hdr_->manager_id = last_manager_id++;
    manager_hdr_->num_buffers = num_buffers;
    manager_hdr_->buffer_size = buffer_size;

}
catch (interprocess_exception& e)
{
    // Catch, transform and rethrow any exceptions thrown during the member initializer list
    std::stringstream ss;
    ss << "Failed to create shared buffer manager: " << e.what();
    throw (SharedBufferManagerException(ss.str()));
}

SharedBufferManager::SharedBufferManager(const std::string& shared_mem_name) try :
    shared_mem_name_(shared_mem_name),
    remove_when_deleted_(false),
    shared_mem_(open_only, shared_mem_name_.c_str(), read_write)
{

    // Map the whole shared memory region into this process
    shared_mem_region_ = mapped_region(shared_mem_, read_write);

    // Determine how big the region is
    shared_mem_size_ = shared_mem_region_.get_size();

    // Map the buffer manager header
    manager_hdr_ = reinterpret_cast<Header*>(shared_mem_region_.get_address());

}
catch (interprocess_exception& e)
{
    // Catch, transform and rethrow any exceptions thrown during the member initializer list
    std::stringstream ss;
    ss << "Failed to map existing shared buffer manager: " << e.what();
    throw (SharedBufferManagerException(ss.str()));
}

SharedBufferManager::~SharedBufferManager()
{
    if (remove_when_deleted_)
    {
        shared_memory_object::remove(shared_mem_name_.c_str());
    }
}

const size_t SharedBufferManager::get_manager_id(void) const
{
    return manager_hdr_->manager_id;
}
const size_t SharedBufferManager::get_num_buffers(void) const
{
    return manager_hdr_->num_buffers;
}

const size_t SharedBufferManager::get_buffer_size(void) const
{
    return manager_hdr_->buffer_size;
}

void* SharedBufferManager::get_buffer_address(const unsigned int buffer) const
{
    if (buffer >= manager_hdr_->num_buffers)
    {
        std::stringstream ss;
        ss << "Illegal buffer index specified: " << buffer;
        throw SharedBufferManagerException(ss.str());
    }
    return reinterpret_cast<void *>(((char*)shared_mem_region_.get_address() + sizeof(Header)) + buffer * manager_hdr_->buffer_size);
}

size_t SharedBufferManager::last_manager_id = 0;

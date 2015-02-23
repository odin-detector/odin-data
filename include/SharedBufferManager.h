/*!
 * SharedBufferManager.h
 *
 *  Created on: Feb 18, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef SHAREDBUFFERMANAGER_H_
#define SHAREDBUFFERMANAGER_H_

#include "FrameReceiverException.h"

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <string>

#include <stddef.h>

namespace FrameReceiver
{

    //! SharedBufferManagerException - custom exception class implementing "what" for error string
    class SharedBufferManagerException : public FrameReceiverException {
    public:
        SharedBufferManagerException(const std::string what) : FrameReceiverException(what) { }
    };

    class SharedBufferManager
    {
    public:

        typedef struct
        {
            size_t manager_id;
            size_t num_buffers;
            size_t buffer_size;
        } Header;

        SharedBufferManager(const std::string& shared_mem_name, const size_t shared_mem_size,
                const size_t buffer_size, bool remove_when_deleted=true);
        SharedBufferManager(const std::string& shared_mem_name);

        ~SharedBufferManager();

        const size_t get_manager_id(void) const;
        const size_t get_num_buffers(void) const;
        const size_t get_buffer_size(void) const;

        void* get_buffer_address(const unsigned int buffer) const;

    private:

        std::string shared_mem_name_;
        size_t      shared_mem_size_;
        bool        remove_when_deleted_;
        boost::interprocess::shared_memory_object shared_mem_;
        boost::interprocess::mapped_region        shared_mem_region_;
        Header*                                   manager_hdr_;

        static size_t last_manager_id;
    };

} // namespace FrameReceiver



#endif /* SHAREDBUFFERMANAGER_H_ */

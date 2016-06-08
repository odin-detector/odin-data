/**
framenotifier_data.h

  Created on: 15 Oct 2015
      Author: up45
*/

#ifndef TOOLS_CLIENT_FRAMENOTIFIER_DATA_H_
#define TOOLS_CLIENT_FRAMENOTIFIER_DATA_H_

#include <string>
#include <stdint.h>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/shared_ptr.hpp>

#include <log4cxx/logger.h>

#include "DataBlock.h"
#include "DataBlockPool.h"

// Shared Buffer (IPC) Header
typedef struct
{
    size_t manager_id;
    size_t num_buffers;
    size_t buffer_size;
} Header;

typedef unsigned long long dimsize_t;
typedef std::vector<dimsize_t> dimensions_t;

//============== end of stolen bits ============================================================

namespace filewriter
{

  class Frame
  {
  public:
    Frame(const std::string& index);
    virtual ~Frame();
    void copy_data(const void* data_src, size_t nbytes);
    const void* get_data() const;
    size_t get_data_size() const;
    const std::string& get_dataset_name() const { return dataset_name; }
    void set_dataset_name(const std::string& dataset) { this->dataset_name = dataset; }
    void set_frame_number(unsigned long long number) { this->frameNumber_ = number; }
    unsigned long long get_frame_number() const { return this->frameNumber_; }

    void set_dimensions(const std::string& type, const std::vector<unsigned long long>& dimensions);
    dimensions_t get_dimensions(const std::string& type) const;
    void set_parameter(const std::string& index, size_t parameter);
    size_t get_parameter(const std::string& index) const;

  private:
    Frame();
    Frame(const Frame& src); // Don't try to copy one of these!

    log4cxx::LoggerPtr logger;
    std::string dataset_name;
    std::string blockIndex_;
    size_t bytes_per_pixel;
    unsigned long long frameNumber_;
    std::map<std::string, dimensions_t> dimensions_;
    std::map<std::string, size_t> parameters_;
    boost::shared_ptr<DataBlock> raw_;
  };

} /* namespace filewriter */

#endif /* TOOLS_CLIENT_FRAMENOTIFIER_DATA_H_ */

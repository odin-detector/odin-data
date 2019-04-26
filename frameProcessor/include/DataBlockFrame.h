#ifndef FRAMEPROCSESOR_DATABLOCKFRAME_H
#define FRAMEPROCESSOR_DATABLOCKFRAME_H

#include "Frame.h"
#include "DataBlock.h"

#include <boost/shared_ptr.hpp>

namespace FrameProcessor {

class DataBlockFrame : public Frame {

 public:

  /** Construct a DataBlockFrame */
  DataBlockFrame(const FrameMetaData &meta_data,
                 const void *data_src,
                 size_t block_size,
                 const int &image_offset = 0);

  /** Construct a DataBlockFrame */
  DataBlockFrame(const FrameMetaData& meta_data,
                 size_t block_size,
                 const int &image_offset = 0);

  /** Shallow-copy copy */
  DataBlockFrame(const DataBlockFrame &frame);

  /** Deep-copy assignment */
  DataBlockFrame& operator=(DataBlockFrame &frame);

  /** Destructor */
  ~DataBlockFrame();

  /** Return a void pointer to the raw data */
  virtual void* get_data_ptr() const;

  /** Return the data size */
  virtual size_t get_data_size() const;

  /** Change the data size */
  virtual void resize(size_t size);

 private:

  /** Pointer to raw data block */
  boost::shared_ptr <DataBlock> raw_data_block_ptr_;

};

}

#endif

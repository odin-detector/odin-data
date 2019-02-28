/*
 * DataBlockPool.h
 *
 *  Created on: 24 May 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_DATABLOCKPOOL_H_
#define TOOLS_FILEWRITER_DATABLOCKPOOL_H_

#include <list>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "DataBlock.h"

namespace FrameProcessor
{

/**
 * The DataBlock and DataBlockPool classes provide memory management for
 * data within Frames. Memory is allocated by a data block on construction,
 * and then the data block can be re-used without continually freeing and re-
 * allocating the memory.
 * The DataBlockPool provides a singleton class that can be used to access
 * data blocks through shared memory pointers and manages the data blocks to
 * avoid continuous allocating and freeing of memory. The DataBlockPool also
 * contains details of how many blocks are available, in use and the total
 * memory used.
 */
class DataBlockPool
{
public:
  virtual ~DataBlockPool();

  static void allocate(size_t block_count, size_t block_size);
  static boost::shared_ptr<DataBlock> take(size_t block_size);
  static void release(boost::shared_ptr<DataBlock> block);
  static size_t get_free_blocks(size_t block_size);
  static size_t get_used_blocks(size_t block_size);
  static size_t get_total_blocks(size_t block_size);
  static size_t get_memory_allocated(size_t block_size);
  static void tearDownClass();

private:
  static DataBlockPool *instance(size_t block_size);
  DataBlockPool();
  void internal_allocate(size_t block_count, size_t block_size);
  boost::shared_ptr<DataBlock> internal_take(size_t block_size);
  void internal_release(boost::shared_ptr<DataBlock> block);
  size_t internal_get_free_blocks();
  size_t internal_get_used_blocks();
  size_t internal_get_total_blocks();
  size_t internal_get_memory_allocated();

  /** Pointer to logger */
  log4cxx::LoggerPtr logger_;
  /** Mutex used to make this class thread safe */
  boost::recursive_mutex mutex_;
  /** List of currently available DataBlock objects */
  std::list<boost::shared_ptr<DataBlock> > free_list_;
  /** Map of currently used DataBlock objects, indexed by their unique IDs */
  std::map<int, boost::shared_ptr<DataBlock> > used_map_;
  /** Number of currently available DataBlock objects */
  size_t free_blocks_;
  /** Number of currently used DataBlock objects */
  size_t used_blocks_;
  /** Total number of DataBlock objects, used + free */
  size_t total_blocks_;
  /** Total number of bytes allocated (sum of all DataBlocks) */
  size_t memory_allocated_;
  /** Static map of all DataBlockPool objects, indexed by their names */
  static std::map<size_t , DataBlockPool*> instance_map_;
};

} /* namespace FrameProcessor */
#endif /* TOOLS_FILEWRITER_DATABLOCKPOOL_H_ */

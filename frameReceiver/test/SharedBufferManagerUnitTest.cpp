/*
 * SharedBufferManagerUnitTest.cpp
 *
 *  Created on: Feb 18, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include <boost/test/unit_test.hpp>
#include <sys/wait.h>

#include "SharedBufferManager.h"

const std::string shared_mem_name = "TestSharedBuffer";
const size_t buffer_size          = 100;
const size_t num_buffers          = 10;
const size_t shared_mem_size      = buffer_size * num_buffers;


class SharedBufferManagerTestFixture
{
public:
  SharedBufferManagerTestFixture() :
      shared_buffer_manager(shared_mem_name, shared_mem_size, buffer_size, false)
  {
    BOOST_TEST_MESSAGE("Setup test fixture");
  }

  ~SharedBufferManagerTestFixture()
  {
    BOOST_TEST_MESSAGE("Tear down test fixture");
  }

  OdinData::SharedBufferManager shared_buffer_manager;
};

BOOST_FIXTURE_TEST_SUITE( SharedBufferManagerUnitTest, SharedBufferManagerTestFixture );

BOOST_AUTO_TEST_CASE( BasicSharedBufferTest )
{

  void* buf_address = shared_buffer_manager.get_buffer_address(0);
  BOOST_CHECK_NE(buf_address, (void*)0);
  BOOST_CHECK_EQUAL(buffer_size, shared_buffer_manager.get_buffer_size());
  BOOST_CHECK_EQUAL(num_buffers, shared_buffer_manager.get_num_buffers());

}

BOOST_AUTO_TEST_CASE( IllegalSharedBufferIndexTest )
{
  BOOST_CHECK_THROW(shared_buffer_manager.get_buffer_address(num_buffers), OdinData::SharedBufferManagerException);
}

BOOST_AUTO_TEST_CASE( SharedWithChildProcessTest )
{
  size_t manager_id = shared_buffer_manager.get_manager_id();

  // Initialise the contents of first buffer to incrementing byte values
  char* buf_address = reinterpret_cast<char*>(shared_buffer_manager.get_buffer_address(0));
  size_t buffer_size = shared_buffer_manager.get_buffer_size();
  for (int i = 0; i < buffer_size; i++)
  {
    buf_address[i] = i % 256;
  }

  // Initialise the contents of the last buffer to a fixed value
  const int max_buffer_value = 123;
  void* max_buf_address = shared_buffer_manager.get_buffer_address(num_buffers - 1);
  memset(max_buf_address, max_buffer_value, buffer_size);

  int pid = fork();
  BOOST_REQUIRE(pid >= 0);
  if (pid == 0)
  {
    int rc = 0;

    // Child process - create a shared buffer manager object pointing at the existing shared memory
    OdinData::SharedBufferManager child_manager(shared_mem_name);

    // Check manager ID matches parent
    size_t child_manager_id = child_manager.get_manager_id();
    BOOST_CHECK_EQUAL(child_manager_id, manager_id);
    if (child_manager_id != manager_id)
    {
      rc = -1;
    }

    // Check child has non-zero buffer address
    char* child_buf_address = reinterpret_cast<char*>(child_manager.get_buffer_address(0));
    BOOST_CHECK_NE(child_buf_address, (char*)0);

    // Check child buffer size matches parent
    size_t child_buffer_size = child_manager.get_buffer_size();
    BOOST_CHECK_EQUAL(buffer_size, child_buffer_size);
    if (buffer_size != child_buffer_size) {
      rc = -1;
    }

    // Check child number of buffers matches parent
    size_t child_num_buffers = child_manager.get_num_buffers();
    BOOST_CHECK_EQUAL(num_buffers, child_num_buffers);
    if (num_buffers != child_num_buffers)
    {
      rc = -1;
    }

    // Check the first buffer has been initialised with incrementing byte values
    int buffer_values_mismatched = 0;
    for (int i = 0; i < child_buffer_size; i++)
    {
      if (buf_address[i] != i % 256)
      {
        buffer_values_mismatched++;
      }
    }
    BOOST_CHECK_EQUAL(buffer_values_mismatched, 0);
    if (buffer_values_mismatched) {
      rc = -1;
    }

    // Check the last buffer has been initialized with the fixed value
    char* child_max_buffer_address = reinterpret_cast<char*>(child_manager.get_buffer_address(child_num_buffers-1));
    int max_buffer_values_mismatched = 0;
    for (int i = 0; i < child_buffer_size; i++)
    {
      if (child_max_buffer_address[i] != max_buffer_value)
      {
        max_buffer_values_mismatched++;
      }
    }
    BOOST_CHECK_EQUAL(max_buffer_values_mismatched, 0);
    if (max_buffer_values_mismatched)
    {
      rc = -1;
    }

    _exit(rc);
  }
  else
  {
    int child_rc = -1;
    wait(&child_rc);
    BOOST_CHECK_EQUAL(child_rc, 0);
  }

}

BOOST_AUTO_TEST_CASE( MapMissingSharedBufferTest )
{
  // Try to create a shared buffer manager pointing at name that doesn't exist - should throw
  // a SharedBufferManagerException
  BOOST_CHECK_THROW(OdinData::SharedBufferManager illegal_manager("ThisIsNotShared"),
                    OdinData::SharedBufferManagerException);

}

BOOST_AUTO_TEST_CASE( BufferBiggerThanSharedMemTest)
{
  BOOST_CHECK_THROW(OdinData::SharedBufferManager illegal_manager("BadSize", 100, 1000),
                    OdinData::SharedBufferManagerException);
}

BOOST_AUTO_TEST_SUITE_END();

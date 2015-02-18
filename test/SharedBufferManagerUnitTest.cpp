/*
 * SharedBufferManagerUnitTest.cpp
 *
 *  Created on: Feb 18, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <sys/wait.h>

#include "SharedBufferManager.h"

const size_t buffer_size     = 100;
const size_t num_buffers     = 10;
const size_t shared_mem_size = buffer_size * num_buffers;


class SharedBufferManagerTestFixture
{
public:
    SharedBufferManagerTestFixture() :
        shared_buffer_manager("TestSharedBuffer", shared_mem_size, buffer_size, false)
    {
        BOOST_TEST_MESSAGE("Setup test fixture");
    }

    ~SharedBufferManagerTestFixture()
    {
        BOOST_TEST_MESSAGE("Tear down test fixture");
    }

    FrameReceiver::SharedBufferManager shared_buffer_manager;
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
    BOOST_CHECK_THROW(shared_buffer_manager.get_buffer_address(num_buffers), FrameReceiver::SharedBufferManagerException);
}

BOOST_AUTO_TEST_CASE( SharedWithChildProcessTest )
{
    int pid = fork();
    BOOST_REQUIRE(pid >= 0);
    if (pid == 0)
    {
        // Child process
        sleep(1);
        _exit(0);
    }
    else
    {
        int child_rc = -1;
        wait(&child_rc);
        BOOST_CHECK_EQUAL(child_rc, 0);
    }

}
BOOST_AUTO_TEST_SUITE_END();



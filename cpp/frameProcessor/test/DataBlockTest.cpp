#define BOOST_TEST_MODULE "DataBlockTests"
#define BOOST_TEST_MAIN

#include "Fixtures.h"

#include "DataBlockPool.h"

BOOST_TEST_GLOBAL_FIXTURE(GlobalConfig);

BOOST_AUTO_TEST_SUITE(DataBlockUnitTest);

BOOST_AUTO_TEST_CASE(DataBlockTest)
{
    char data1[1024];
    char data2[2048];
    std::shared_ptr<FrameProcessor::DataBlock> block1;
    std::shared_ptr<FrameProcessor::DataBlock> block2;
    int initial_block_index_count = FrameProcessor::DataBlock::get_current_index_count();
    BOOST_CHECK_NO_THROW(block1 = std::shared_ptr<FrameProcessor::DataBlock>(new FrameProcessor::DataBlock(1024)));
    BOOST_CHECK_EQUAL(block1->get_index(), initial_block_index_count);
    BOOST_CHECK_EQUAL(block1->get_size(), 1024);
    BOOST_CHECK_NO_THROW(block2 = std::shared_ptr<FrameProcessor::DataBlock>(new FrameProcessor::DataBlock(2048)));
    BOOST_CHECK_EQUAL(block2->get_index(), initial_block_index_count + 1);
    BOOST_CHECK_EQUAL(block2->get_size(), 2048);
    memset(data1, 1, 1024);
    BOOST_CHECK_NO_THROW(block1->copy_data(data1, 1024));
    memset(data2, 2, 2048);
    BOOST_CHECK_NO_THROW(block2->copy_data(data2, 2048));
    char* testPtr;
    BOOST_CHECK_NO_THROW(testPtr = (char*)block1->get_data());
    for (int index = 0; index < 1024; index++) {
        BOOST_CHECK_EQUAL(testPtr[index], 1);
    }
    BOOST_CHECK_NO_THROW(testPtr = (char*)block2->get_data());
    for (int index = 0; index < 2048; index++) {
        BOOST_CHECK_EQUAL(testPtr[index], 2);
    }
}

BOOST_AUTO_TEST_CASE(DataBlockPoolTest)
{
    std::shared_ptr<FrameProcessor::DataBlock> block1;
    std::shared_ptr<FrameProcessor::DataBlock> block2;
    // Allocate 100 blocks
    BOOST_CHECK_NO_THROW(FrameProcessor::DataBlockPool::allocate(100, 1024));
    // Check pool statistics
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_free_blocks(1024), 100);
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_used_blocks(1024), 0);
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_total_blocks(1024), 100);
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_memory_allocated(1024), 102400);
    // Take 2 blocks
    BOOST_CHECK_NO_THROW(block1 = FrameProcessor::DataBlockPool::take(1024));
    BOOST_CHECK_NO_THROW(block2 = FrameProcessor::DataBlockPool::take(1024));
    // Check pool statistics
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_free_blocks(1024), 98);
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_used_blocks(1024), 2);
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_total_blocks(1024), 100);
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_memory_allocated(1024), 102400);
    // Check the taken blocks have different index values
    BOOST_CHECK_NE(block1->get_index(), block2->get_index());
    // Release 1 block
    BOOST_CHECK_NO_THROW(FrameProcessor::DataBlockPool::release(block1));
    // Check pool statistics
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_free_blocks(1024), 99);
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_used_blocks(1024), 1);
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_total_blocks(1024), 100);
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_memory_allocated(1024), 102400);
    // Allocate another 100 blocks
    BOOST_CHECK_NO_THROW(FrameProcessor::DataBlockPool::allocate(100, 1024));
    // Check pool statistics
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_free_blocks(1024), 199);
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_used_blocks(1024), 1);
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_total_blocks(1024), 200);
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_memory_allocated(1024), 204800);
    // Now take a block of a different size
    BOOST_CHECK_NO_THROW(block2 = FrameProcessor::DataBlockPool::take(1025));
    // Check new and old pool statistics
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_free_blocks(1024), 199);
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_used_blocks(1024), 1);
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_used_blocks(1025), 1);
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_total_blocks(1025), 2);
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_total_blocks(1024), 200);
    // Memory allocated should have increased by 0 bytes old pool; 2050 new pool
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_memory_allocated(1024), 204800);
    BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::get_memory_allocated(1025), 2050);
    // Check the blocks have different index values
    BOOST_CHECK_NE(block1->get_index(), block2->get_index());
    // Check the blocks have different sizes
    BOOST_CHECK_NE(block1->get_size(), block2->get_size());
}

BOOST_AUTO_TEST_CASE(DataBlockFrameTest)
{
    unsigned short img[12] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
    dimensions_t img_dims(2);
    img_dims[0] = 3;
    img_dims[1] = 4;

    FrameProcessor::FrameMetaData frame_meta(
        7, "data", FrameProcessor::raw_16bit, "test", img_dims, FrameProcessor::no_compression
    );
    FrameProcessor::DataBlockFrame frame(frame_meta, static_cast<void*>(img), 24);
    BOOST_REQUIRE_EQUAL(frame.get_data_size(), 24);
    BOOST_REQUIRE_EQUAL(frame.get_meta_data().get_dimensions()[0], 3);
    BOOST_REQUIRE_EQUAL(frame.get_meta_data().get_dimensions()[1], 4);
    BOOST_CHECK_EQUAL(frame.get_frame_number(), 7);
    const unsigned short* img_copy = static_cast<const unsigned short*>(frame.get_data_ptr());
    BOOST_CHECK_EQUAL(img_copy[0], img[0]);
    BOOST_CHECK_EQUAL(img_copy[11], img[11]);
}

BOOST_AUTO_TEST_SUITE_END();

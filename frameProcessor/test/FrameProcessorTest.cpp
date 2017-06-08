
/*
 * FileWriterTest.cpp
 *
 */
#define BOOST_TEST_MODULE "FrameProcessorUnitTests"
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>

#include <log4cxx/logger.h>
#include <log4cxx/xml/domconfigurator.h>
#include <log4cxx/simplelayout.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/basicconfigurator.h>
using namespace log4cxx;
using namespace log4cxx::xml;

#include "DataBlock.h"
#include "DataBlockPool.h"
#include "FileWriterPlugin.h"
#include "Frame.h"

void checkFrames(const char *filename, const char *dataset, int expectedFrames) {
  hid_t file = H5Fopen(filename, H5F_ACC_RDONLY, 0);
  hid_t dataset_id = H5Dopen(file, dataset, H5P_DEFAULT);
  hid_t dspace_id = H5Dget_space(dataset_id);
  const int ndims = H5Sget_simple_extent_ndims(dspace_id);
  hsize_t dims[ndims];
  H5Sget_simple_extent_dims(dspace_id, dims, NULL);
  BOOST_CHECK_EQUAL(dims[0], expectedFrames);
}

class GlobalConfig {
public:
    GlobalConfig() {
        //std::cout << "GlobalConfig constructor" << std::endl;
        // Create a default simple console appender for log4cxx.
        consoleAppender = new ConsoleAppender(LayoutPtr(new SimpleLayout()));
        BasicConfigurator::configure(AppenderPtr(consoleAppender));
        Logger::getRootLogger()->setLevel(Level::getWarn());
    }
    ~GlobalConfig(){
        //std::cout << "GlobalConfig constructor" << std::endl;
        //delete consoleAppender;
    };
private:
    ConsoleAppender *consoleAppender;
};
BOOST_GLOBAL_FIXTURE(GlobalConfig);

BOOST_AUTO_TEST_SUITE(DataBlockUnitTest);

BOOST_AUTO_TEST_CASE(DataBlockTest)
{
  char data1[1024];
  char data2[2048];
  boost::shared_ptr<FrameProcessor::DataBlock> block1;
  boost::shared_ptr<FrameProcessor::DataBlock> block2;
  BOOST_CHECK_NO_THROW(block1 = boost::shared_ptr<FrameProcessor::DataBlock>(new FrameProcessor::DataBlock(1024)));
  BOOST_CHECK_EQUAL(block1->getIndex(), 0);
  BOOST_CHECK_EQUAL(block1->getSize(), 1024);
  BOOST_CHECK_NO_THROW(block2 = boost::shared_ptr<FrameProcessor::DataBlock>(new FrameProcessor::DataBlock(2048)));
  BOOST_CHECK_EQUAL(block2->getIndex(), 1);
  BOOST_CHECK_EQUAL(block2->getSize(), 2048);
  memset(data1, 1, 1024);
  BOOST_CHECK_NO_THROW(block1->copyData(data1, 1024));
  memset(data2, 2, 2048);
  BOOST_CHECK_NO_THROW(block2->copyData(data2, 2048));
  char *testPtr;
  BOOST_CHECK_NO_THROW(testPtr = (char *)block1->get_data());
  for (int index = 0; index < 1024; index++){
    BOOST_CHECK_EQUAL(testPtr[index], 1);
  }
  BOOST_CHECK_NO_THROW(testPtr = (char *)block2->get_data());
  for (int index = 0; index < 2048; index++){
    BOOST_CHECK_EQUAL(testPtr[index], 2);
  }
}

BOOST_AUTO_TEST_CASE(DataBlockPoolTest)
{
  boost::shared_ptr<FrameProcessor::DataBlock> block1;
  boost::shared_ptr<FrameProcessor::DataBlock> block2;
  // Allocate 100 blocks
  BOOST_CHECK_NO_THROW(FrameProcessor::DataBlockPool::allocate("test1", 100, 1024));
  // Check pool statistics
  BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::getFreeBlocks("test1"), 100);
  BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::getUsedBlocks("test1"), 0);
  BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::getTotalBlocks("test1"), 100);
  BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::getMemoryAllocated("test1"), 102400);
  // Take 2 blocks
  BOOST_CHECK_NO_THROW(block1 = FrameProcessor::DataBlockPool::take("test1", 1024));
  BOOST_CHECK_NO_THROW(block2 = FrameProcessor::DataBlockPool::take("test1", 1024));
  // Check pool statistics
  BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::getFreeBlocks("test1"), 98);
  BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::getUsedBlocks("test1"), 2);
  BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::getTotalBlocks("test1"), 100);
  BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::getMemoryAllocated("test1"), 102400);
  // Check the taken blocks have different index values
  BOOST_CHECK_NE(block1->getIndex(), block2->getIndex());
  // Release 1 block
  BOOST_CHECK_NO_THROW(FrameProcessor::DataBlockPool::release("test1", block1));
  // Check pool statistics
  BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::getFreeBlocks("test1"), 99);
  BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::getUsedBlocks("test1"), 1);
  BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::getTotalBlocks("test1"), 100);
  BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::getMemoryAllocated("test1"), 102400);
  // Allocate another 100 blocks
  BOOST_CHECK_NO_THROW(FrameProcessor::DataBlockPool::allocate("test1", 100, 1024));
  // Check pool statistics
  BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::getFreeBlocks("test1"), 199);
  BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::getUsedBlocks("test1"), 1);
  BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::getTotalBlocks("test1"), 200);
  BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::getMemoryAllocated("test1"), 204800);
  // Now take a block of a different size
  BOOST_CHECK_NO_THROW(block2 = FrameProcessor::DataBlockPool::take("test1", 1025));
  // Check pool statistics
  BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::getFreeBlocks("test1"), 198);
  BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::getUsedBlocks("test1"), 2);
  BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::getTotalBlocks("test1"), 200);
  // Memory allocated should have increased by 1 byte
  BOOST_CHECK_EQUAL(FrameProcessor::DataBlockPool::getMemoryAllocated("test1"), 204801);
  // Check the blocks have different index values
  BOOST_CHECK_NE(block1->getIndex(), block2->getIndex());
  // Check the blocks have different sizes
  BOOST_CHECK_NE(block1->getSize(), block2->getSize());
}

BOOST_AUTO_TEST_SUITE_END(); //DataBlockUnitTest

BOOST_AUTO_TEST_SUITE(FrameUnitTest);

BOOST_AUTO_TEST_CASE( FrameTest )
{
    unsigned short img[12] =  { 1, 2, 3, 4,
                                5, 6, 7, 8,
                                9,10,11,12 };
    dimensions_t img_dims(2); img_dims[0] = 3; img_dims[1] = 4;

    FrameProcessor::Frame frame("raw");
    frame.set_dimensions("frame", img_dims);
    frame.set_frame_number(7);
    BOOST_CHECK_NO_THROW(frame.copy_data(static_cast<void*>(img), 24));
    BOOST_REQUIRE_EQUAL(frame.get_data_size(), 24);
    BOOST_REQUIRE_EQUAL(frame.get_dimensions("frame")[0], 3);
    BOOST_REQUIRE_EQUAL(frame.get_dimensions("frame")[1], 4);
    BOOST_CHECK_EQUAL(frame.get_frame_number(), 7);
    const unsigned short *img_copy = static_cast<const unsigned short*>(frame.get_data());
    BOOST_CHECK_EQUAL(img_copy[0], img[0]);
    BOOST_CHECK_EQUAL(img_copy[11], img[11]);
}

BOOST_AUTO_TEST_SUITE_END(); //FrameUnitTest



class FileWriterPluginTestFixture
{
public:
    FileWriterPluginTestFixture()
    {
        unsigned short img[12] =  { 1, 2, 3, 4,
                                    5, 6, 7, 8,
                                    9,10,11,12 };
        dimensions_t img_dims(2); img_dims[0] = 3; img_dims[1] = 4;
        //PercivalEmulator::FrameHeader img_header;
        //img_header.frame_number = 7;

        dset_def.name = "data";
        dset_def.num_frames = 2; //unused?
        dset_def.pixel = FrameProcessor::FileWriterPlugin::pixel_raw_16bit;
        dset_def.frame_dimensions = dimensions_t(2);
        dset_def.frame_dimensions[0] = 3;
        dset_def.frame_dimensions[1] = 4;

        frame = boost::shared_ptr<FrameProcessor::Frame>(new FrameProcessor::Frame("data"));
        frame->set_frame_number(7);
//        frame->
//        2, img_dims));
//        frame->copy_header(&img_header);
        frame->copy_data(static_cast<void*>(img), 24);

        for (int i = 1; i<6; i++)
        {
            boost::shared_ptr<FrameProcessor::Frame> tmp_frame(new FrameProcessor::Frame("data")); //2, img_dims));
            tmp_frame->set_frame_number(i);
//            img_header.frame_number = i;
//            tmp_frame->copy_header(&img_header);
            img[0] = i;
            tmp_frame->copy_data(static_cast<void*>(img), 24);
            frames.push_back(tmp_frame);
        }
    }
    ~FileWriterPluginTestFixture(){}
    boost::shared_ptr<FrameProcessor::Frame> frame;
    std::vector< boost::shared_ptr<FrameProcessor::Frame> >frames;
    FrameProcessor::FileWriterPlugin fw;
    FrameProcessor::FileWriterPlugin::DatasetDefinition dset_def;
};

BOOST_FIXTURE_TEST_SUITE(FileWriterPluginUnitTest, FileWriterPluginTestFixture);

BOOST_AUTO_TEST_CASE( FileWriterPluginTest )
{
    BOOST_REQUIRE_NO_THROW(fw.createFile("/tmp/blah.h5"));
    BOOST_REQUIRE_NO_THROW(fw.createDataset(dset_def));
    BOOST_REQUIRE_EQUAL(dset_def.name, frame->get_dataset_name());

    BOOST_REQUIRE_NO_THROW(fw.writeFrame(*frame));
    BOOST_REQUIRE_NO_THROW(fw.closeFile());
}

BOOST_AUTO_TEST_CASE( FileWriterPluginMultiDatasetTest )
{
    BOOST_REQUIRE_NO_THROW(fw.createFile("/tmp/blah_multidataset.h5"));

    // Create the first datset "data"
    BOOST_REQUIRE_NO_THROW(fw.createDataset(dset_def));
    BOOST_CHECK_EQUAL(dset_def.name, frame->get_dataset_name());

    // Create the second datset "stuff"
    dset_def.name = "stuff";
    BOOST_REQUIRE_NO_THROW(fw.createDataset(dset_def));

    // Write first frame to "data"
    BOOST_REQUIRE_NO_THROW(fw.writeFrame(*frame));

    // Write the frame to "stuff"
    BOOST_CHECK_NO_THROW(frame->set_dataset_name("stuff"));
    BOOST_CHECK_EQUAL(dset_def.name, frame->get_dataset_name());
    BOOST_REQUIRE_NO_THROW(fw.writeFrame(*frame));

    // write another frame to "data"
    BOOST_CHECK_EQUAL("data", frames[2]->get_dataset_name());
    BOOST_REQUIRE_NO_THROW(fw.writeFrame(*frames[2]));
    // and yet another frame to "stuff"
    BOOST_CHECK_NO_THROW(frames[2]->set_dataset_name("stuff"));
    BOOST_REQUIRE_NO_THROW(fw.writeFrame(*frames[2]));

    BOOST_REQUIRE_NO_THROW(fw.closeFile());
}

BOOST_AUTO_TEST_CASE( FileWriterPluginBadFileTest )
{
  // Check for an error when a bad file path is provided
  BOOST_CHECK_THROW(fw.createFile("/non/existent/path/blah_throw.h5"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE( FileWriterPluginDatasetWithoutOpenFileTest )
{
  // Check for an error when a dataset is created without a file open
  BOOST_CHECK_THROW(fw.createDataset(dset_def), std::runtime_error);
}

BOOST_AUTO_TEST_CASE( FileWriterPluginNoDatasetDefinitionsTest )
{
  // Create a file ready for writing
  BOOST_REQUIRE_NO_THROW(fw.createFile("/tmp/blah_throw.h5"));

  // Write a frame without first creating the dataset
  BOOST_CHECK_THROW(fw.writeFrame(*frame), std::runtime_error);

  // Attempt to close the file
  BOOST_REQUIRE_NO_THROW(fw.closeFile());
}

BOOST_AUTO_TEST_CASE( FileWriterPluginInvalidDatasetTest )
{
    BOOST_REQUIRE_NO_THROW(fw.createFile("/tmp/blah_throw.h5"));
    BOOST_REQUIRE_NO_THROW(fw.createDataset(dset_def));
    BOOST_REQUIRE_NO_THROW(frame->set_dataset_name("non_existing_dataset_name"));

    BOOST_CHECK_THROW(fw.writeFrame(*frame), std::runtime_error);
    BOOST_REQUIRE_NO_THROW(fw.closeFile());
}

BOOST_AUTO_TEST_CASE( FileWriterPluginMultipleFramesTest )
{
    BOOST_REQUIRE_NO_THROW(fw.createFile("/tmp/blah_multiple.h5"));
    BOOST_REQUIRE_NO_THROW(fw.createDataset(dset_def));

    std::vector<boost::shared_ptr<FrameProcessor::Frame> >::iterator it;
    for (it = frames.begin(); it != frames.end(); ++it){
        BOOST_TEST_MESSAGE("Writing frame: " <<  (*it)->get_frame_number());
        BOOST_REQUIRE_NO_THROW(fw.writeFrame(*(*it)));
    }
    BOOST_REQUIRE_NO_THROW(fw.closeFile());
}

BOOST_AUTO_TEST_CASE( FileWriterPluginMultipleReverseTest )
{
    // Just reverse through the list of frames and write them out.
    // The frames should still appear in the file in the original order...
    BOOST_REQUIRE_NO_THROW(fw.createFile("/tmp/blah_multiple_reverse.h5"));
    BOOST_REQUIRE_NO_THROW(fw.createDataset(dset_def));

    // The first frame to write is used as an offset - so must be the lowest
    // frame number. Frames received later with a smaller number would result
    // in negative file index...
    BOOST_TEST_MESSAGE("Writing frame: " << frame->get_frame_number());
    BOOST_REQUIRE_NO_THROW(fw.writeFrame(*frame));

    std::vector<boost::shared_ptr<FrameProcessor::Frame> >::reverse_iterator rit;
    for (rit = frames.rbegin(); rit != frames.rend(); ++rit){
        BOOST_TEST_MESSAGE("Writing frame: " <<  (*rit)->get_frame_number());
        BOOST_REQUIRE_NO_THROW(fw.writeFrame(*(*rit)));
    }
    BOOST_REQUIRE_NO_THROW(fw.closeFile());
}

BOOST_AUTO_TEST_CASE( FileWriterPluginSubframesTest )
{
	dset_def.chunks = dimensions_t(3);
	dset_def.chunks[0] = 1; dset_def.chunks[1] = 3; dset_def.chunks[2] = 2;
	dimensions_t subdims(2); subdims[0] = 3; subdims[1] = 2;

    BOOST_REQUIRE_NO_THROW(fw.createFile("/tmp/blah_subframes.h5"));
    BOOST_REQUIRE_NO_THROW(fw.createDataset(dset_def));

    std::vector<boost::shared_ptr<FrameProcessor::Frame> >::iterator it;
    for (it = frames.begin(); it != frames.end(); ++it){
      (*it)->set_dimensions("subframe", subdims);
      (*it)->set_parameter("subframe_count", 2);
      (*it)->set_parameter("subframe_size", 12);
        BOOST_TEST_MESSAGE("Writing frame: " <<  (*it)->get_frame_number());
        BOOST_REQUIRE_NO_THROW(fw.writeSubFrames(*(*it)));
    }
    BOOST_REQUIRE_NO_THROW(fw.closeFile());
}

BOOST_AUTO_TEST_CASE( FileWriterPluginAdjustHugeOffset )
{
    BOOST_REQUIRE_NO_THROW(fw.createFile("/tmp/test_huge_offset.h5"));
    BOOST_REQUIRE_NO_THROW(fw.createDataset(dset_def));

    hsize_t huge_offset = 100000;
    BOOST_REQUIRE_NO_THROW(fw.queueFrameOffsetAdjustment(0, huge_offset));

    std::vector<boost::shared_ptr<FrameProcessor::Frame> >::iterator it;
    for (it = frames.begin(); it != frames.end(); ++it){
        size_t frame_no = (*it)->get_frame_number();
        //PercivalEmulator::FrameHeader img_header = *((*it)->get_header());
        //img_header.frame_number = frame_no + huge_offset;
        //(*it)->copy_header(&img_header);
        (*it)->set_frame_number(frame_no + huge_offset);
        hsize_t offset = fw.calculateFrameOffset((*it)->get_frame_number());
        BOOST_TEST_MESSAGE("Writing frame: " <<  frame_no << " offset:" <<  offset);
        BOOST_CHECK_EQUAL(offset, frame_no);
        BOOST_REQUIRE_NO_THROW(fw.writeFrame(*(*it)));
    }
    BOOST_REQUIRE_NO_THROW(fw.closeFile());
}

BOOST_AUTO_TEST_CASE( FileWriterPluginRewind )
{
  BOOST_REQUIRE_NO_THROW(fw.createFile("/tmp/test_rewind.h5"));
  BOOST_REQUIRE_NO_THROW(fw.createDataset(dset_def));

  BOOST_REQUIRE_NO_THROW(fw.queueFrameOffsetAdjustment(0, 1));  // Frames start at 1
  std::vector<boost::shared_ptr<FrameProcessor::Frame> >::iterator it;
  for (it = frames.begin(); it != frames.end() - 2; ++it) {     // Write first 3
    BOOST_REQUIRE_NO_THROW(fw.writeFrame(*(*it)));
  }
  BOOST_REQUIRE_NO_THROW(fw.queueFrameOffsetAdjustment(4, 3));  // Go back 2
  for (it = frames.begin() + 3; it != frames.end(); ++it) {     // Write last 2
    BOOST_REQUIRE_NO_THROW(fw.writeFrame(*(*it)));
  }
  BOOST_REQUIRE_NO_THROW(fw.closeFile());
  checkFrames("/tmp/test_rewind.h5", "data", 3);
}

BOOST_AUTO_TEST_CASE( FileWriterPluginMultiRewind )
{
  BOOST_REQUIRE_NO_THROW(fw.createFile("/tmp/test_multi_rewind.h5"));
  BOOST_REQUIRE_NO_THROW(fw.createDataset(dset_def));

  BOOST_REQUIRE_NO_THROW(fw.queueFrameOffsetAdjustment(0, 1));  // Frames start at 1
  // Add offset adjustment to queue for every frame, increasing by one time
  for (int i = 0; i < 6; i++) {
    BOOST_REQUIRE_NO_THROW(fw.queueFrameOffsetAdjustment(i, i + 2));
  }
  // We should then write the offset=0 frame five times
  std::vector<boost::shared_ptr<FrameProcessor::Frame> >::iterator it;
  for (it = frames.begin(); it != frames.end(); ++it) {
    BOOST_REQUIRE_NO_THROW(fw.writeFrame(*(*it)));
  }
  BOOST_REQUIRE_NO_THROW(fw.closeFile());
  checkFrames("/tmp/test_multi_rewind.h5", "data", 1);
}

BOOST_AUTO_TEST_CASE( FileWriterPluginSubProcess )
{
    // Frame numbers start from 1: 1,2,3,4,5  - but are indexed from 0 in the frames vector.
    // Process numbers start from 0: 0,1,2 - this process pretends to be process 1.
    OdinData::IpcMessage reply;

    FrameProcessor::FileWriterPlugin fw1;
    {
      OdinData::IpcMessage cfg;
      cfg.set_param("process/number", 3);
      cfg.set_param("process/rank", 1);
      fw1.configure(cfg, reply);
    }
    BOOST_REQUIRE_NO_THROW(fw1.createFile("/tmp/process_1of3.h5"));
    BOOST_REQUIRE_NO_THROW(fw1.createDataset(dset_def));
    BOOST_REQUIRE_EQUAL(dset_def.name, frame->get_dataset_name());
    BOOST_REQUIRE_NO_THROW(fw1.queueFrameOffsetAdjustment(0, frames[0]->get_frame_number()));

    // Write frame no. 2 to "data"
    BOOST_CHECK_EQUAL(frames[1]->get_frame_number(), 2);
    BOOST_REQUIRE_NO_THROW(fw1.writeFrame(*frames[1]));

    // write frame no. 5 to "data"
    BOOST_CHECK_EQUAL(frames[4]->get_frame_number(), 5);
    BOOST_REQUIRE_NO_THROW(fw1.writeFrame(*frames[4]));

    // check the edge case where the frame no. is not supposed to go into this file
    // write frame no. 4 to "data" and watch it except!
    BOOST_CHECK_EQUAL(frames[3]->get_frame_number(), 4);
    BOOST_REQUIRE_THROW(fw1.writeFrame(*frames[3]), std::runtime_error);

    BOOST_REQUIRE_NO_THROW(fw1.closeFile());

    FrameProcessor::FileWriterPlugin fw0;
    {
      OdinData::IpcMessage cfg;
      cfg.set_param("process/number", 3);
      cfg.set_param("process/rank", 0);
      fw0.configure(cfg, reply);
    }
    BOOST_REQUIRE_NO_THROW(fw0.createFile("/tmp/process_0of3.h5"));
    BOOST_REQUIRE_NO_THROW(fw0.createDataset(dset_def));
    BOOST_REQUIRE_EQUAL(dset_def.name, frame->get_dataset_name());
    BOOST_REQUIRE_NO_THROW(fw0.queueFrameOffsetAdjustment(0, frames[0]->get_frame_number()));

    // Write frame no. 1 to "data"
    BOOST_CHECK_EQUAL(frames[0]->get_frame_number(), 1);
    BOOST_REQUIRE_NO_THROW(fw0.writeFrame(*frames[0]));

    // write frame no. 4 to "data"
    BOOST_CHECK_EQUAL(frames[3]->get_frame_number(), 4);
    BOOST_REQUIRE_NO_THROW(fw0.writeFrame(*frames[3]));

    BOOST_REQUIRE_NO_THROW(fw0.closeFile());

}

BOOST_AUTO_TEST_SUITE_END(); //FileWriterUnitTest

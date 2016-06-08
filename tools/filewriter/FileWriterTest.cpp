/*
 * FileWriterTest.cpp
 *
 */
#define BOOST_TEST_MODULE "FileWriterUnitTests"
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
#include "FileWriter.h"
#include "Frame.h"
#include "JSONMessage.h"

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

/*
BOOST_AUTO_TEST_SUITE(FrameUnitTest);

BOOST_AUTO_TEST_CASE( FrameTest )
{
    unsigned short img[12] =  { 1, 2, 3, 4,
                                5, 6, 7, 8,
                                9,10,11,12 };
    dimensions_t img_dims(2); img_dims[0] = 3; img_dims[1] = 4;
    PercivalEmulator::FrameHeader img_header;
    img_header.frame_number = 7;

    Frame frame(2, img_dims);
    BOOST_REQUIRE_EQUAL(frame.get_data_size(), 24);
    BOOST_CHECK_NO_THROW(frame.copy_header(&img_header));
    BOOST_CHECK_NO_THROW(frame.copy_data(static_cast<void*>(img), 24));
    BOOST_CHECK_EQUAL(frame.get_frame_number(), 7);
    const unsigned short *img_copy = static_cast<const unsigned short*>(frame.get_data());
    BOOST_CHECK_EQUAL(img_copy[0], img[0]);
    BOOST_CHECK_EQUAL(img_copy[11], img[11]);
}

BOOST_AUTO_TEST_SUITE_END(); //FrameUnitTest

*/
/*
class FileWriterTestFixture
{
public:
    FileWriterTestFixture()
    {
        unsigned short img[12] =  { 1, 2, 3, 4,
                                    5, 6, 7, 8,
                                    9,10,11,12 };
        dimensions_t img_dims(2); img_dims[0] = 3; img_dims[1] = 4;
        PercivalEmulator::FrameHeader img_header;
        img_header.frame_number = 7;

        dset_def.name = "data";
        dset_def.num_frames = 2; //unused?
        dset_def.pixel = FileWriter::pixel_raw_16bit;
        dset_def.frame_dimensions = dimensions_t(2);
        dset_def.frame_dimensions[0] = 3;
        dset_def.frame_dimensions[1] = 4;

        frame = boost::shared_ptr<Frame>(new Frame(2, img_dims));
        frame->copy_header(&img_header);
        frame->copy_data(static_cast<void*>(img), 24);

        for (int i = 1; i<6; i++)
        {
            boost::shared_ptr<Frame> tmp_frame(new Frame(2, img_dims));
            img_header.frame_number = i;
            tmp_frame->copy_header(&img_header);
            img[0] = i;
            tmp_frame->copy_data(static_cast<void*>(img), 24);
            frames.push_back(tmp_frame);
        }
    }
    ~FileWriterTestFixture(){}
    boost::shared_ptr<Frame> frame;
    std::vector< boost::shared_ptr<Frame> >frames;
    FileWriter fw;
    FileWriter::DatasetDefinition dset_def;
};

BOOST_FIXTURE_TEST_SUITE(FileWriterUnitTest, FileWriterTestFixture);

BOOST_AUTO_TEST_CASE( FileWriterTest )
{
    BOOST_REQUIRE_NO_THROW(fw.createFile("/tmp/blah.h5"));
    BOOST_REQUIRE_NO_THROW(fw.createDataset(dset_def));
    BOOST_REQUIRE_EQUAL(dset_def.name, frame->get_dataset_name());

    BOOST_REQUIRE_NO_THROW(fw.writeFrame(*frame));
    BOOST_REQUIRE_NO_THROW(fw.closeFile());
}

BOOST_AUTO_TEST_CASE( FileWriterMultiDatasetTest )
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

BOOST_AUTO_TEST_CASE( FileWriterInvalidDatasetTest )
{
    BOOST_REQUIRE_NO_THROW(fw.createFile("/tmp/blah_throw.h5"));
    BOOST_REQUIRE_NO_THROW(fw.createDataset(dset_def));
    BOOST_REQUIRE_NO_THROW(frame->set_dataset_name("non_existing_dataset_name"));

    BOOST_CHECK_THROW(fw.writeFrame(*frame), std::runtime_error);
    BOOST_REQUIRE_NO_THROW(fw.closeFile());
}

BOOST_AUTO_TEST_CASE( FileWriterMultipleFramesTest )
{
    BOOST_REQUIRE_NO_THROW(fw.createFile("/tmp/blah_multiple.h5"));
    BOOST_REQUIRE_NO_THROW(fw.createDataset(dset_def));

    std::vector< boost::shared_ptr<Frame> >::iterator it;
    for (it = frames.begin(); it != frames.end(); ++it){
        BOOST_TEST_MESSAGE("Writing frame: " <<  (*it)->get_frame_number());
        BOOST_REQUIRE_NO_THROW(fw.writeFrame(*(*it)));
    }
    BOOST_REQUIRE_NO_THROW(fw.closeFile());
}

BOOST_AUTO_TEST_CASE( FileWriterMultipleReverseTest )
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

    std::vector< boost::shared_ptr<Frame> >::reverse_iterator rit;
    for (rit = frames.rbegin(); rit != frames.rend(); ++rit){
        BOOST_TEST_MESSAGE("Writing frame: " <<  (*rit)->get_frame_number());
        BOOST_REQUIRE_NO_THROW(fw.writeFrame(*(*rit)));
    }
    BOOST_REQUIRE_NO_THROW(fw.closeFile());
}

BOOST_AUTO_TEST_CASE( FileWriterSubframesTest )
{
	dset_def.chunks = dimensions_t(3);
	dset_def.chunks[0] = 1; dset_def.chunks[1] = 3; dset_def.chunks[2] = 2;
	dimensions_t subdims(2); subdims[0] = 3; subdims[1] = 2;

    BOOST_REQUIRE_NO_THROW(fw.createFile("/tmp/blah_subframes.h5"));
    BOOST_REQUIRE_NO_THROW(fw.createDataset(dset_def));

    std::vector< boost::shared_ptr<Frame> >::iterator it;
    for (it = frames.begin(); it != frames.end(); ++it){
    	(*it)->set_subframe_dimensions(subdims);
        BOOST_TEST_MESSAGE("Writing frame: " <<  (*it)->get_frame_number());
        BOOST_REQUIRE_NO_THROW(fw.writeSubFrames(*(*it)));
    }
    BOOST_REQUIRE_NO_THROW(fw.closeFile());
}

BOOST_AUTO_TEST_CASE( FileWriterAdjustHugeOffset )
{
    BOOST_REQUIRE_NO_THROW(fw.createFile("/tmp/test_huge_offset.h5"));
    BOOST_REQUIRE_NO_THROW(fw.createDataset(dset_def));

    hsize_t huge_offset = 100000;
    BOOST_REQUIRE_NO_THROW(fw.setStartFrameOffset(huge_offset));

    std::vector< boost::shared_ptr<Frame> >::iterator it;
    for (it = frames.begin(); it != frames.end(); ++it){
        size_t frame_no = (*it)->get_frame_number();
        PercivalEmulator::FrameHeader img_header = *((*it)->get_header());
        img_header.frame_number = frame_no + huge_offset;
        (*it)->copy_header(&img_header);
        hsize_t offset = fw.getFrameOffset((*it)->get_frame_number());
        BOOST_TEST_MESSAGE("Writing frame: " <<  frame_no << " offset:" <<  offset);
        BOOST_CHECK_EQUAL(offset, frame_no);
        BOOST_REQUIRE_NO_THROW(fw.writeFrame(*(*it)));
    }
    BOOST_REQUIRE_NO_THROW(fw.closeFile());
}

BOOST_AUTO_TEST_CASE( FileWriterSubProcess )
{
    // Frame numbers start from 1: 1,2,3,4,5  - but are indexed from 0 in the frames vector.
    // Process numbers start from 0: 0,1,2 - this process pretends to be process 1.

    FileWriter fw1(3, 1);
    BOOST_REQUIRE_NO_THROW(fw1.createFile("/tmp/process_1of3.h5"));
    BOOST_REQUIRE_NO_THROW(fw1.createDataset(dset_def));
    BOOST_REQUIRE_EQUAL(dset_def.name, frame->get_dataset_name());
    BOOST_REQUIRE_NO_THROW(fw1.setStartFrameOffset(frames[0]->get_frame_number()));

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

    FileWriter fw0(3, 0);
    BOOST_REQUIRE_NO_THROW(fw0.createFile("/tmp/process_0of3.h5"));
    BOOST_REQUIRE_NO_THROW(fw0.createDataset(dset_def));
    BOOST_REQUIRE_EQUAL(dset_def.name, frame->get_dataset_name());
    BOOST_REQUIRE_NO_THROW(fw0.setStartFrameOffset(frames[0]->get_frame_number()));

    // Write frame no. 1 to "data"
    BOOST_CHECK_EQUAL(frames[0]->get_frame_number(), 1);
    BOOST_REQUIRE_NO_THROW(fw0.writeFrame(*frames[0]));

    // write frame no. 4 to "data"
    BOOST_CHECK_EQUAL(frames[3]->get_frame_number(), 4);
    BOOST_REQUIRE_NO_THROW(fw0.writeFrame(*frames[3]));

    BOOST_REQUIRE_NO_THROW(fw0.closeFile());

}

BOOST_AUTO_TEST_SUITE_END(); //FileWriterUnitTest
*/
BOOST_AUTO_TEST_SUITE(DataBlockUnitTest);

BOOST_AUTO_TEST_CASE(DataBlockTest)
{
  char data1[1024];
  char data2[2048];
  boost::shared_ptr<filewriter::DataBlock> block1;
  boost::shared_ptr<filewriter::DataBlock> block2;
  BOOST_CHECK_NO_THROW(block1 = boost::shared_ptr<filewriter::DataBlock>(new filewriter::DataBlock(1024)));
  BOOST_CHECK_EQUAL(block1->getIndex(), 0);
  BOOST_CHECK_EQUAL(block1->getSize(), 1024);
  BOOST_CHECK_NO_THROW(block2 = boost::shared_ptr<filewriter::DataBlock>(new filewriter::DataBlock(2048)));
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
  boost::shared_ptr<filewriter::DataBlock> block1;
  boost::shared_ptr<filewriter::DataBlock> block2;
  // Allocate 100 blocks
  BOOST_CHECK_NO_THROW(filewriter::DataBlockPool::allocate("test1", 100, 1024));
  // Check pool statistics
  BOOST_CHECK_EQUAL(filewriter::DataBlockPool::getFreeBlocks("test1"), 100);
  BOOST_CHECK_EQUAL(filewriter::DataBlockPool::getUsedBlocks("test1"), 0);
  BOOST_CHECK_EQUAL(filewriter::DataBlockPool::getTotalBlocks("test1"), 100);
  BOOST_CHECK_EQUAL(filewriter::DataBlockPool::getMemoryAllocated("test1"), 102400);
  // Take 2 blocks
  BOOST_CHECK_NO_THROW(block1 = filewriter::DataBlockPool::take("test1", 1024));
  BOOST_CHECK_NO_THROW(block2 = filewriter::DataBlockPool::take("test1", 1024));
  // Check pool statistics
  BOOST_CHECK_EQUAL(filewriter::DataBlockPool::getFreeBlocks("test1"), 98);
  BOOST_CHECK_EQUAL(filewriter::DataBlockPool::getUsedBlocks("test1"), 2);
  BOOST_CHECK_EQUAL(filewriter::DataBlockPool::getTotalBlocks("test1"), 100);
  BOOST_CHECK_EQUAL(filewriter::DataBlockPool::getMemoryAllocated("test1"), 102400);
  // Check the taken blocks have different index values
  BOOST_CHECK_NE(block1->getIndex(), block2->getIndex());
  // Release 1 block
  BOOST_CHECK_NO_THROW(filewriter::DataBlockPool::release("test1", block1));
  // Check pool statistics
  BOOST_CHECK_EQUAL(filewriter::DataBlockPool::getFreeBlocks("test1"), 99);
  BOOST_CHECK_EQUAL(filewriter::DataBlockPool::getUsedBlocks("test1"), 1);
  BOOST_CHECK_EQUAL(filewriter::DataBlockPool::getTotalBlocks("test1"), 100);
  BOOST_CHECK_EQUAL(filewriter::DataBlockPool::getMemoryAllocated("test1"), 102400);
  // Allocate another 100 blocks
  BOOST_CHECK_NO_THROW(filewriter::DataBlockPool::allocate("test1", 100, 1024));
  // Check pool statistics
  BOOST_CHECK_EQUAL(filewriter::DataBlockPool::getFreeBlocks("test1"), 199);
  BOOST_CHECK_EQUAL(filewriter::DataBlockPool::getUsedBlocks("test1"), 1);
  BOOST_CHECK_EQUAL(filewriter::DataBlockPool::getTotalBlocks("test1"), 200);
  BOOST_CHECK_EQUAL(filewriter::DataBlockPool::getMemoryAllocated("test1"), 204800);
  // Now take a block of a different size
  BOOST_CHECK_NO_THROW(block2 = filewriter::DataBlockPool::take("test1", 1025));
  // Check pool statistics
  BOOST_CHECK_EQUAL(filewriter::DataBlockPool::getFreeBlocks("test1"), 198);
  BOOST_CHECK_EQUAL(filewriter::DataBlockPool::getUsedBlocks("test1"), 2);
  BOOST_CHECK_EQUAL(filewriter::DataBlockPool::getTotalBlocks("test1"), 200);
  // Memory allocated should have increased by 1 byte
  BOOST_CHECK_EQUAL(filewriter::DataBlockPool::getMemoryAllocated("test1"), 204801);
  // Check the blocks have different index values
  BOOST_CHECK_NE(block1->getIndex(), block2->getIndex());
  // Check the blocks have different sizes
  BOOST_CHECK_NE(block1->getSize(), block2->getSize());
}

BOOST_AUTO_TEST_SUITE_END(); //DataBlockUnitTest

BOOST_AUTO_TEST_SUITE(JSONMessageUnitTest);

BOOST_AUTO_TEST_CASE(JSONMessageTest)
{
  filewriter::JSONMessage msg1("{\"hello\": \"world\",\"t\": true ,\"f\": false,\"n\": null,\"i\": 123,\"pi\": 3.1416,\"obj\": {\"first\" : 1, \"second\": 2} }");
  BOOST_CHECK_EQUAL(msg1["hello"].GetString(), "world");
  BOOST_CHECK_EQUAL(msg1["t"].GetBool(), true);
  BOOST_CHECK_EQUAL(msg1["f"].GetBool(), false);
  BOOST_CHECK_EQUAL(msg1["n"].IsNull(), true);
  BOOST_CHECK_EQUAL(msg1["i"].GetInt(), 123);
  BOOST_CHECK_EQUAL(msg1["pi"].GetDouble(), 3.1416);
  BOOST_CHECK_EQUAL(msg1["obj"]["first"].GetInt(), 1);
  BOOST_CHECK_EQUAL(msg1["obj"]["second"].GetInt(), 2);
  BOOST_CHECK_EQUAL(msg1.toString(), "{\n\
    \"hello\": \"world\",\n\
    \"t\": true,\n\
    \"f\": false,\n\
    \"n\": null,\n\
    \"i\": 123,\n\
    \"pi\": 3.1416,\n\
    \"obj\": {\n\
        \"first\": 1,\n\
        \"second\": 2\n\
    }\n\
}");
  msg1["hello"].SetString("test1");
  msg1["t"].SetBool(false);
  msg1["f"].SetBool(true);
  msg1["i"].SetInt(321);
  msg1["pi"].SetDouble(6.1413);
  msg1["obj"]["first"].SetString("test2");
  msg1["obj"]["second"].SetString("test3");
  BOOST_CHECK_EQUAL(msg1.toString(), "{\n\
    \"hello\": \"test1\",\n\
    \"t\": false,\n\
    \"f\": true,\n\
    \"n\": null,\n\
    \"i\": 321,\n\
    \"pi\": 6.1413,\n\
    \"obj\": {\n\
        \"first\": \"test2\",\n\
        \"second\": \"test3\"\n\
    }\n\
}");

}

BOOST_AUTO_TEST_SUITE_END(); //JSONMessageUnitTest


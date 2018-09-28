/*
 * FileWriterTest.cpp
 *
 */
#define BOOST_TEST_MODULE "FrameProcessorUnitTests"
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

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
#include "Acquisition.h"
#include "FrameProcessorDefinitions.h"
#include "UIDAdjustmentPlugin.h"
#include "OffsetAdjustmentPlugin.h"
#include "LiveViewPlugin.h"

class GlobalConfig {
public:
  GlobalConfig() :
    metaRxChannel_(ZMQ_PULL)
{
    //std::cout << "GlobalConfig constructor" << std::endl;
    // Create a default simple console appender for log4cxx.
    consoleAppender = new ConsoleAppender(LayoutPtr(new SimpleLayout()));
    BasicConfigurator::configure(AppenderPtr(consoleAppender));
    Logger::getRootLogger()->setLevel(Level::getWarn());
    metaRxChannel_.bind("inproc://meta_rx");
  }
  ~GlobalConfig() {
    //std::cout << "GlobalConfig constructor" << std::endl;
    //delete consoleAppender;
  };
private:
  ConsoleAppender *consoleAppender;
  OdinData::IpcChannel metaRxChannel_;
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
  for (int index = 0; index < 1024; index++) {
    BOOST_CHECK_EQUAL(testPtr[index], 1);
  }
  BOOST_CHECK_NO_THROW(testPtr = (char *)block2->get_data());
  for (int index = 0; index < 2048; index++) {
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
  frame.set_dimensions(img_dims);
  frame.set_frame_number(7);
  BOOST_CHECK_NO_THROW(frame.copy_data(static_cast<void*>(img), 24));
  BOOST_REQUIRE_EQUAL(frame.get_data_size(), 24);
  BOOST_REQUIRE_EQUAL(frame.get_dimensions()[0], 3);
  BOOST_REQUIRE_EQUAL(frame.get_dimensions()[1], 4);
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
    dimensions_t chunk_dims(3); chunk_dims[0] = 1; chunk_dims[1] = 3; chunk_dims[2] = 4;
    //PercivalEmulator::FrameHeader img_header;
    //img_header.frame_number = 7;

    dset_def.name = "data";
    dset_def.num_frames = 2; //unused?
    dset_def.data_type = FrameProcessor::raw_16bit;
    dset_def.frame_dimensions = dimensions_t(2);
    dset_def.frame_dimensions[0] = 3;
    dset_def.frame_dimensions[1] = 4;
    dset_def.chunks = chunk_dims;
    dset_def.create_low_high_indexes = false;

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
  ~FileWriterPluginTestFixture() {}
  boost::shared_ptr<FrameProcessor::Frame> frame;
  std::vector< boost::shared_ptr<FrameProcessor::Frame> >frames;
  FrameProcessor::FileWriterPlugin fw;
  FrameProcessor::HDF5File hdf5f;
  FrameProcessor::DatasetDefinition dset_def;
};

BOOST_FIXTURE_TEST_SUITE(HDF5FileUnitTest, FileWriterPluginTestFixture);

BOOST_AUTO_TEST_CASE( HDF5FileTest )
{
  BOOST_REQUIRE_NO_THROW(hdf5f.create_file("/tmp/blah.h5", 0, false, 1, 1));
  BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));
  BOOST_REQUIRE_EQUAL(dset_def.name, frame->get_dataset_name());

  BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*frame, frame->get_frame_number(), 1));
  BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE( HDF5FileMultiDatasetTest )
{
  BOOST_REQUIRE_NO_THROW(hdf5f.create_file("/tmp/blah_multidataset.h5", 0, false, 1, 1));

  // Create the first dataset "data"
  BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));
  BOOST_CHECK_EQUAL(dset_def.name, frame->get_dataset_name());

  // Create the second dataset "stuff"
  dset_def.name = "stuff";
  BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));

  // Write first frame to "data"
  BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*frame, frame->get_frame_number(), 1));

  // Write the frame to "stuff"
  BOOST_CHECK_NO_THROW(frame->set_dataset_name("stuff"));
  BOOST_CHECK_EQUAL(dset_def.name, frame->get_dataset_name());
  BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*frame, frame->get_frame_number(), 1));

  // write another frame to "data"
  BOOST_CHECK_EQUAL("data", frames[2]->get_dataset_name());
  BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*frames[2], frames[2]->get_frame_number(), 1));
  // and yet another frame to "stuff"
  BOOST_CHECK_NO_THROW(frames[2]->set_dataset_name("stuff"));
  BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*frames[2], frames[2]->get_frame_number(), 1));

  BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE( HDF5FileBadFileTest )
{
  // Check for an error when a bad file path is provided
  BOOST_CHECK_THROW(hdf5f.create_file("/non/existent/path/blah_throw.h5", 0, false, 1, 1), std::runtime_error);
}

BOOST_AUTO_TEST_CASE( FileWriterPluginDatasetWithoutOpenFileTest )
{
  // Check for an error when a dataset is created without a file open
  BOOST_CHECK_THROW(hdf5f.create_dataset(dset_def, -1, -1), std::runtime_error);
}

BOOST_AUTO_TEST_CASE( HDF5FileNoDatasetDefinitionsTest )
{
  // Create a file ready for writing
  BOOST_REQUIRE_NO_THROW(hdf5f.create_file("/tmp/blah_throw.h5", 0, false, 1, 1));

  // Write a frame without first creating the dataset
  BOOST_CHECK_THROW(hdf5f.write_frame(*frame, frame->get_frame_number(), 1), std::runtime_error);

  // Attempt to close the file
  BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE( HDF5FileInvalidDatasetTest )
{
  BOOST_REQUIRE_NO_THROW(hdf5f.create_file("/tmp/blah_throw.h5", 0, false, 1, 1));
  BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));
  BOOST_REQUIRE_NO_THROW(frame->set_dataset_name("non_existing_dataset_name"));

  BOOST_CHECK_THROW(hdf5f.write_frame(*frame, frame->get_frame_number(), 1), std::runtime_error);
  BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE( FileWriterPluginMultipleFramesTest )
{
  BOOST_REQUIRE_NO_THROW(hdf5f.create_file("/tmp/blah_multiple.h5", 0, false, 1, 1));
  BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));

  std::vector<boost::shared_ptr<FrameProcessor::Frame> >::iterator it;
  for (it = frames.begin(); it != frames.end(); ++it) {
    BOOST_TEST_MESSAGE("Writing frame: " <<  (*it)->get_frame_number());
    BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*(*it), (*it)->get_frame_number(), 1));
  }
  BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE( HDF5FileMultipleReverseTest )
{
  // Just reverse through the list of frames and write them out.
  // The frames should still appear in the file in the original order...
  BOOST_REQUIRE_NO_THROW(hdf5f.create_file("/tmp/blah_multiple_reverse.h5", 0, false, 1, 1));
  BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));

  // The first frame to write is used as an offset - so must be the lowest
  // frame number. Frames received later with a smaller number would result
  // in negative file index...
  BOOST_TEST_MESSAGE("Writing frame: " << frame->get_frame_number());
  BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*frame, frame->get_frame_number(), 1));

  std::vector<boost::shared_ptr<FrameProcessor::Frame> >::reverse_iterator rit;
  for (rit = frames.rbegin(); rit != frames.rend(); ++rit) {
    BOOST_TEST_MESSAGE("Writing frame: " <<  (*rit)->get_frame_number());
    BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*(*rit), (*rit)->get_frame_number(), 1));
  }
  BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE( HDF5FileAdjustHugeOffset )
{
  BOOST_REQUIRE_NO_THROW(hdf5f.create_file("/tmp/test_huge_offset.h5", 0, false, 1, 1));
  BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));

  hsize_t huge_offset = 100000;

  std::vector<boost::shared_ptr<FrameProcessor::Frame> >::iterator it;
  for (it = frames.begin(); it != frames.end(); ++it){
    size_t frame_no = (*it)->get_frame_number();
    //PercivalEmulator::FrameHeader img_header = *((*it)->get_header());
    //img_header.frame_number = frame_no + huge_offset;
    //(*it)->copy_header(&img_header);
    (*it)->set_frame_offset(huge_offset);
    (*it)->set_frame_number(frame_no + huge_offset);
    BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*(*it), (*it)->get_frame_number(), 1));
  }
  BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE( FileWriterPluginWriteParamTest )
{
  BOOST_REQUIRE_NO_THROW(hdf5f.create_file("/tmp/test_params.h5", 0, false, 1, 1));

  FrameProcessor::DatasetDefinition param_dset_def;
  param_dset_def.name = "p1";
  param_dset_def.data_type = FrameProcessor::raw_64bit;
  dimensions_t chunk_dims(1);
  chunk_dims[0] = 1;
  param_dset_def.chunks = chunk_dims;

  BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(param_dset_def, -1, -1));

  std::vector<boost::shared_ptr<FrameProcessor::Frame> >::iterator it;
  for (it = frames.begin(); it != frames.end(); ++it) {
    uint64_t val = 123;
    (*it)->set_parameter("p1", val);
    BOOST_TEST_MESSAGE("Writing frame: " <<  (*it)->get_frame_number());
    BOOST_REQUIRE_NO_THROW(hdf5f.write_parameter(*(*it), param_dset_def, (*it)->get_frame_number()));
  }
  BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE( FileWriterPluginWriteParamWrongTypeTest )
{
  BOOST_REQUIRE_NO_THROW(hdf5f.create_file("/tmp/test_params.h5", 0, false, 1, 1));

  FrameProcessor::DatasetDefinition param_dset_def;
  param_dset_def.name = "p1";
  param_dset_def.data_type = FrameProcessor::raw_16bit;
  dimensions_t chunk_dims(1);
  chunk_dims[0] = 1;
  param_dset_def.chunks = chunk_dims;

  BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(param_dset_def, -1, -1));

  std::vector<boost::shared_ptr<FrameProcessor::Frame> >::iterator it;
  for (it = frames.begin(); it != frames.end(); ++it) {
    uint64_t val = 123;
    (*it)->set_parameter("p1", val);
    BOOST_TEST_MESSAGE("Writing frame: " <<  (*it)->get_frame_number());
    BOOST_CHECK_THROW(hdf5f.write_parameter(*(*it), param_dset_def, (*it)->get_frame_number()), std::runtime_error);
  }
  BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE( FileWriterPluginWriteParamNoParamTest )
{
  BOOST_REQUIRE_NO_THROW(hdf5f.create_file("/tmp/test_params.h5", 0, false, 1, 1));

  FrameProcessor::DatasetDefinition param_dset_def;
  param_dset_def.name = "p1";
  param_dset_def.data_type = FrameProcessor::raw_64bit;
  dimensions_t chunk_dims(1);
  chunk_dims[0] = 1;
  param_dset_def.chunks = chunk_dims;

  BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(param_dset_def, -1, -1));

  std::vector<boost::shared_ptr<FrameProcessor::Frame> >::iterator it;
  for (it = frames.begin(); it != frames.end(); ++it) {
    uint64_t val = 123;
    (*it)->set_parameter("p2", val);
    BOOST_TEST_MESSAGE("Writing frame: " <<  (*it)->get_frame_number());
    BOOST_CHECK_THROW(hdf5f.write_parameter(*(*it), param_dset_def, (*it)->get_frame_number()), std::runtime_error);
  }
  BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_SUITE_END(); //HDF5FileUnitTest


BOOST_FIXTURE_TEST_SUITE(AcquisitionUnitTest, FileWriterPluginTestFixture);

BOOST_AUTO_TEST_CASE( AcquisitionGetFileIndex )
{
  FrameProcessor::Acquisition acquisition;

  acquisition.concurrent_rank_ = 0;
  acquisition.concurrent_processes_ = 4;
  acquisition.frames_per_block_ = 1000;
  acquisition.blocks_per_file_ = 1;

  size_t file_index = acquisition.get_file_index(0);
  BOOST_CHECK_EQUAL(0, file_index);

  file_index = acquisition.get_file_index(1);
  BOOST_CHECK_EQUAL(0, file_index);

  file_index = acquisition.get_file_index(999);
  BOOST_CHECK_EQUAL(0, file_index);

  file_index = acquisition.get_file_index(4000);
  BOOST_CHECK_EQUAL(4, file_index);

  file_index = acquisition.get_file_index(4523);
  BOOST_CHECK_EQUAL(4, file_index);

  file_index = acquisition.get_file_index(4999);
  BOOST_CHECK_EQUAL(4, file_index);

  file_index = acquisition.get_file_index(8231);
  BOOST_CHECK_EQUAL(8, file_index);

  acquisition.concurrent_rank_ = 1;

  file_index = acquisition.get_file_index(1000);
  BOOST_CHECK_EQUAL(1, file_index);

  file_index = acquisition.get_file_index(1111);
  BOOST_CHECK_EQUAL(1, file_index);

  file_index = acquisition.get_file_index(1999);
  BOOST_CHECK_EQUAL(1, file_index);

  file_index = acquisition.get_file_index(5000);
  BOOST_CHECK_EQUAL(5, file_index);

  acquisition.concurrent_rank_ = 2;

  file_index = acquisition.get_file_index(2000);
  BOOST_CHECK_EQUAL(2, file_index);

  file_index = acquisition.get_file_index(2311);
  BOOST_CHECK_EQUAL(2, file_index);

  file_index = acquisition.get_file_index(2999);
  BOOST_CHECK_EQUAL(2, file_index);

  file_index = acquisition.get_file_index(6999);
  BOOST_CHECK_EQUAL(6, file_index);

  acquisition.concurrent_rank_ = 3;

  file_index = acquisition.get_file_index(3000);
  BOOST_CHECK_EQUAL(3, file_index);

  file_index = acquisition.get_file_index(3311);
  BOOST_CHECK_EQUAL(3, file_index);

  file_index = acquisition.get_file_index(3999);
  BOOST_CHECK_EQUAL(3, file_index);

  file_index = acquisition.get_file_index(7452);
  BOOST_CHECK_EQUAL(7, file_index);

  acquisition.concurrent_rank_ = 0;
  acquisition.concurrent_processes_ = 1;
  acquisition.frames_per_block_ = 3;
  acquisition.blocks_per_file_ = 5;

  file_index = acquisition.get_file_index(0);
  BOOST_CHECK_EQUAL(0, file_index);

  file_index = acquisition.get_file_index(2);
  BOOST_CHECK_EQUAL(0, file_index);

  file_index = acquisition.get_file_index(10);
  BOOST_CHECK_EQUAL(0, file_index);

  file_index = acquisition.get_file_index(14);
  BOOST_CHECK_EQUAL(0, file_index);

  file_index = acquisition.get_file_index(15);
  BOOST_CHECK_EQUAL(1, file_index);

  file_index = acquisition.get_file_index(20);
  BOOST_CHECK_EQUAL(1, file_index);

  file_index = acquisition.get_file_index(29);
  BOOST_CHECK_EQUAL(1, file_index);

  file_index = acquisition.get_file_index(30);
  BOOST_CHECK_EQUAL(2, file_index);

  acquisition.concurrent_rank_ = 0;
  acquisition.concurrent_processes_ = 4;
  acquisition.frames_per_block_ = 100;
  acquisition.blocks_per_file_ = 1;

  file_index = acquisition.get_file_index(0);
  BOOST_CHECK_EQUAL(0, file_index);

  file_index = acquisition.get_file_index(401);
  BOOST_CHECK_EQUAL(4, file_index);


}

BOOST_AUTO_TEST_CASE( AcquisitionGetOffsetInFile )
{
  FrameProcessor::Acquisition acquisition;

  acquisition.concurrent_rank_ = 0;
  acquisition.concurrent_processes_ = 4;
  acquisition.frames_per_block_ = 1000;
  acquisition.blocks_per_file_ = 1;

  size_t file_index = acquisition.get_frame_offset_in_file(0);
  BOOST_CHECK_EQUAL(0, file_index);

  file_index = acquisition.get_frame_offset_in_file(1);
  BOOST_CHECK_EQUAL(1, file_index);

  file_index = acquisition.get_frame_offset_in_file(999);
  BOOST_CHECK_EQUAL(999, file_index);

  file_index = acquisition.get_frame_offset_in_file(4000);
  BOOST_CHECK_EQUAL(0, file_index);

  file_index = acquisition.get_frame_offset_in_file(4523);
  BOOST_CHECK_EQUAL(523, file_index);

  file_index = acquisition.get_frame_offset_in_file(4999);
  BOOST_CHECK_EQUAL(999, file_index);

  file_index = acquisition.get_frame_offset_in_file(8231);
  BOOST_CHECK_EQUAL(231, file_index);

  acquisition.concurrent_rank_ = 1;

  file_index = acquisition.get_frame_offset_in_file(1000);
  BOOST_CHECK_EQUAL(0, file_index);

  file_index = acquisition.get_frame_offset_in_file(1430);
  BOOST_CHECK_EQUAL(430, file_index);

  file_index = acquisition.get_frame_offset_in_file(5999);
  BOOST_CHECK_EQUAL(999, file_index);

  acquisition.concurrent_rank_ = 2;

  file_index = acquisition.get_frame_offset_in_file(2000);
  BOOST_CHECK_EQUAL(0, file_index);

  file_index = acquisition.get_frame_offset_in_file(2999);
  BOOST_CHECK_EQUAL(999, file_index);

  acquisition.concurrent_rank_ = 3;

  file_index = acquisition.get_frame_offset_in_file(7000);
  BOOST_CHECK_EQUAL(0, file_index);

  file_index = acquisition.get_frame_offset_in_file(7549);
  BOOST_CHECK_EQUAL(549, file_index);

  acquisition.concurrent_rank_ = 0;
  acquisition.concurrent_processes_ = 4;
  acquisition.frames_per_block_ = 100;
  acquisition.blocks_per_file_ = 2;

  file_index = acquisition.get_frame_offset_in_file(23);
  BOOST_CHECK_EQUAL(23, file_index);

  file_index = acquisition.get_frame_offset_in_file(464);
  BOOST_CHECK_EQUAL(164, file_index);

  file_index = acquisition.get_frame_offset_in_file(801);
  BOOST_CHECK_EQUAL(1, file_index);

  acquisition.concurrent_rank_ = 1;

  file_index = acquisition.get_frame_offset_in_file(151);
  BOOST_CHECK_EQUAL(51, file_index);

  file_index = acquisition.get_frame_offset_in_file(599);
  BOOST_CHECK_EQUAL(199, file_index);

  acquisition.concurrent_rank_ = 3;

  file_index = acquisition.get_frame_offset_in_file(1141);
  BOOST_CHECK_EQUAL(41, file_index);

}

BOOST_AUTO_TEST_CASE( AcquisitionAdjustFrameOffset )
{
  FrameProcessor::Acquisition acquisition;

  boost::shared_ptr<FrameProcessor::Frame> frame(new FrameProcessor::Frame("raw"));
  frame->set_frame_number(0);
  frame->set_frame_offset(0);

  size_t adjusted_offset = acquisition.adjust_frame_offset(frame);
  BOOST_CHECK_EQUAL(0, adjusted_offset);

  frame->set_frame_number(1);
  adjusted_offset = acquisition.adjust_frame_offset(frame);
  BOOST_CHECK_EQUAL(1, adjusted_offset);

  frame->set_frame_number(0);
  frame->set_frame_offset(1);
  adjusted_offset = acquisition.adjust_frame_offset(frame);
  BOOST_CHECK_EQUAL(1, adjusted_offset);

  frame->set_frame_number(1);
  frame->set_frame_offset(1);
  adjusted_offset = acquisition.adjust_frame_offset(frame);
  BOOST_CHECK_EQUAL(2, adjusted_offset);

  frame->set_frame_number(1);
  frame->set_frame_offset(-1);
  adjusted_offset = acquisition.adjust_frame_offset(frame);
  BOOST_CHECK_EQUAL(0, adjusted_offset);

  frame->set_frame_number(0);
  frame->set_frame_offset(-1);
  BOOST_CHECK_THROW(acquisition.adjust_frame_offset(frame), std::range_error);
}

BOOST_AUTO_TEST_SUITE_END(); //AcquisitionUnitTest

BOOST_FIXTURE_TEST_SUITE(UIDAdjustmentPluginUnitTest, FileWriterPluginTestFixture);

BOOST_AUTO_TEST_CASE( AdjustUID )
{
  FrameProcessor::UIDAdjustmentPlugin plugin;
  boost::shared_ptr<FrameProcessor::Frame> frame(new FrameProcessor::Frame("raw"));
  uint64_t uid = 0;
  frame->set_parameter("UID", uid);

  // Check 0 goes to 0 when no config has been sent
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(0, frame->get_i64_parameter("UID"));

  // Check a non 0 goes to the same when no config has been sent
  uid = 27;
  frame->set_frame_number(uid);
  frame->set_parameter("UID", uid);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(27, frame->get_i64_parameter("UID"));

  // Configure a frame offset of 4
  OdinData::IpcMessage reply;
  OdinData::IpcMessage cfg;
  cfg.set_param(FrameProcessor::UID_ADJUSTMENT_CONFIG, 4);
  plugin.configure(cfg, reply);

  // Check the offset is not applied as not seen first frame since configuring
  uid = 28;
  frame->set_frame_number(uid);
  frame->set_parameter("UID", uid);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(28, frame->get_i64_parameter("UID"));

  // Check the offset is applied when a new first frame is now sent
  uid = 0;
  frame->set_frame_number(uid);
  frame->set_parameter("UID", uid);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(4, frame->get_i64_parameter("UID"));

  // Check the offset is still applied when not on frame 0
  uid = 10;
  frame->set_frame_number(uid);
  frame->set_parameter("UID", uid);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(14, frame->get_i64_parameter("UID"));

  // Check the offset is still applied when uid is different to frame number
  uid = 100;
  frame->set_frame_number(27);
  frame->set_parameter("UID", uid);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(104, frame->get_i64_parameter("UID"));

  // Configure a start frame of 1
  cfg.set_param(FrameProcessor::UID_ADJUSTMENT_CONFIG, 33);
  cfg.set_param(FrameProcessor::FIRST_FRAME_CONFIG, 1);
  plugin.configure(cfg, reply);

  // Check the new offset is not applied when not on frame 0
  uid = 12;
  frame->set_frame_number(uid);
  frame->set_parameter("UID", uid);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(16, frame->get_i64_parameter("UID"));

  // Check the new offset is not applied when on frame 0
  uid = 0;
  frame->set_frame_number(uid);
  frame->set_parameter("UID", uid);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(4, frame->get_i64_parameter("UID"));

  // Check the new offset is applied when on frame 1
  uid = 1;
  frame->set_frame_number(uid);
  frame->set_parameter("UID", uid);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(34, frame->get_i64_parameter("UID"));

  // Check the new offset is applied when on frame 2
  uid = 2;
  frame->set_frame_number(uid);
  frame->set_parameter("UID", uid);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(35, frame->get_i64_parameter("UID"));
}
 
BOOST_AUTO_TEST_SUITE_END(); //UIDAdjustmentPluginUnitTest

BOOST_FIXTURE_TEST_SUITE(OffsetAdjustmentPluginUnitTest, FileWriterPluginTestFixture);

BOOST_AUTO_TEST_CASE( AdjustOffset )
{
  FrameProcessor::OffsetAdjustmentPlugin plugin;
  boost::shared_ptr<FrameProcessor::Frame> frame(new FrameProcessor::Frame("raw"));
  uint64_t offset = 0;
  frame->set_frame_offset(offset);

  // Check 0 goes to 0 when no config has been sent
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(0, frame->get_frame_offset());

  // Check a non 0 goes to the same when no config has been sent
  offset = 27;
  frame->set_frame_number(offset);
  frame->set_frame_offset(offset);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(27, frame->get_frame_offset());

  // Configure a frame offset of 4
  OdinData::IpcMessage reply;
  OdinData::IpcMessage cfg;
  cfg.set_param(FrameProcessor::OFFSET_ADJUSTMENT_CONFIG, 4);
  plugin.configure(cfg, reply);

  // Check the offset is not applied as not seen first frame since configuring
  offset = 28;
  frame->set_frame_number(offset);
  frame->set_frame_offset(offset);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(28, frame->get_frame_offset());

  // Check the offset is applied when a new first frame is now sent
  offset = 0;
  frame->set_frame_number(offset);
  frame->set_frame_offset(offset);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(4, frame->get_frame_offset());

  // Check the offset is still applied when not on frame 0
  offset = 10;
  frame->set_frame_number(offset);
  frame->set_frame_offset(offset);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(14, frame->get_frame_offset());

  // Check the offset is still applied when uid is different to frame number
  offset = 100;
  frame->set_frame_number(27);
  frame->set_frame_offset(offset);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(104, frame->get_frame_offset());

  // Configure a start frame of 1
  cfg.set_param(FrameProcessor::OFFSET_ADJUSTMENT_CONFIG, 33);
  cfg.set_param(FrameProcessor::FIRST_FRAME_CONFIG, 1);
  plugin.configure(cfg, reply);

  // Check the new offset is not applied when not on frame 0
  offset = 12;
  frame->set_frame_number(offset);
  frame->set_frame_offset(offset);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(16, frame->get_frame_offset());

  // Check the new offset is not applied when on frame 0
  offset = 0;
  frame->set_frame_number(offset);
  frame->set_frame_offset(offset);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(4, frame->get_frame_offset());

  // Check the new offset is applied when on frame 1
  offset = 1;
  frame->set_frame_number(offset);
  frame->set_frame_offset(offset);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(34, frame->get_frame_offset());

  // Check the new offset is applied when on frame 2
  offset = 2;
  frame->set_frame_number(offset);
  frame->set_frame_offset(offset);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(35, frame->get_frame_offset());
}

BOOST_AUTO_TEST_SUITE_END(); //UIDAdjustmentPluginUnitTest

BOOST_FIXTURE_TEST_SUITE(LiveViewPluginUnitTest, FileWriterPluginTestFixture);

class LiveViewPluginTestFixture
{
public:
  LiveViewPluginTestFixture()
  {
    dimensions_t img_dims(2); img_dims[0] = 3; img_dims[1] = 4;

    dset_def.data_type = FrameProcessor::raw_8bit;
    dset_def.frame_dimensions = dimensions_t(2);
    dset_def.frame_dimensions[0] = 3;
    dset_def.frame_dimensions[1] = 4;

    frame = boost::shared_ptr<FrameProcessor::Frame>(new FrameProcessor::Frame("data"));
    frame->set_frame_number(2);
    frame->set_dimensions(dset_def.frame_dimensions);
    frame->set_data_type(dset_def.data_type);
    frame->set_compression(0);
    frame->copy_data(static_cast<void*>(img_8), 12);

    frame_16 = boost::shared_ptr<FrameProcessor::Frame>(new FrameProcessor::Frame("data"));
    frame_16->set_frame_number(2);
    frame_16->set_dimensions(dset_def.frame_dimensions);
    frame_16->set_data_type(FrameProcessor::raw_16bit);
    frame_16->set_compression(0);
    frame_16->copy_data(static_cast<void*>(img_16), 24);

    for(int i = 0; i < 10; i++)
      {
        boost::shared_ptr<FrameProcessor::Frame> tmp_frame(new FrameProcessor::Frame("data"));
        tmp_frame->set_frame_number(i);
        tmp_frame->set_dimensions(dset_def.frame_dimensions);
        tmp_frame->set_data_type(dset_def.data_type);
        tmp_frame->set_dataset_name(i % 4 ? "data" : "not_data");

        tmp_frame->copy_data(static_cast<void*>(img_8), 12);

        frames.push_back(tmp_frame);
      }
    new_socket_recieve = false;

  }
  ~LiveViewPluginTestFixture() {}
  boost::shared_ptr<FrameProcessor::Frame> frame;
  boost::shared_ptr<FrameProcessor::Frame> frame_16;
  std::vector< boost::shared_ptr<FrameProcessor::Frame> >frames;
  FrameProcessor::FileWriterPlugin fw;
  FrameProcessor::HDF5File hdf5f;
  FrameProcessor::DatasetDefinition dset_def;
  uint8_t img_8[12]   =  { 1, 2, 3, 4,
                           5, 6, 7, 8,
                           9,10,11,12 };
  uint16_t img_16[12] =  { 1, 2, 3, 4,
                           5, 6, 7, 8,
                           9,10,11,12 };
  const std::string DATA_TYPES[3] = {"uint8","uint16","uint32"};
  const std::string COMPRESS_TYPES[3] = {"none","LZ4","BSLZ4"};
  bool new_socket_recieve; //we have to declare this here, or we get a memory access error around line 825

};

BOOST_FIXTURE_TEST_SUITE(LiveViewPluginUnitTest, LiveViewPluginTestFixture);

BOOST_AUTO_TEST_CASE(LiveViewTest)
{

  uint8_t  pbuf[frame->get_data_size()]; //create basic array to store data
  uint16_t pbuf_16[12];

  FrameProcessor::LiveViewPlugin plugin;
  OdinData::IpcMessage reply;
  OdinData::IpcMessage cfg;

  OdinData::IpcChannel recv_socket(ZMQ_SUB);
  OdinData::IpcChannel recv_socket_other(ZMQ_SUB);
  recv_socket.subscribe("");
  recv_socket_other.subscribe("");
  //std::string live_view_socket_addr = plugin.getPubSocketAddr();
  recv_socket.connect("tcp://127.0.0.1:5020");
  recv_socket_other.connect("tcp://127.0.0.1:1337");
  //std::cout << live_view_socket_addr << std::endl;

  frame->set_frame_number(FrameProcessor::LiveViewPlugin::DEFAULT_FRAME_FREQ);
  //test we can output frame
  while(!recv_socket.poll(100)) //to avoid issue with slow joiners, using a while loop here
  {
    plugin.process_frame(frame);
  }
  std::string message = recv_socket.recv();
  BOOST_TEST_MESSAGE(message);
  rapidjson::Document doc;
  doc.Parse(message.c_str());

  //TEST HEADER CONTENTS
  BOOST_CHECK_EQUAL(doc["frame_num"].GetInt(), frame->get_frame_number());
  BOOST_CHECK_EQUAL(doc["acquisition_id"].GetString(), frame->get_acquisition_id());
  BOOST_CHECK_EQUAL(doc["dtype"].GetString(), DATA_TYPES[frame->get_data_type()]);
  BOOST_CHECK_EQUAL(doc["dsize"].GetInt() , frame->get_data_size());
  BOOST_CHECK_EQUAL(doc["compression"].GetString(), COMPRESS_TYPES[frame->get_compression()]);
  BOOST_CHECK_EQUAL(atoi(doc["shape"][0].GetString()), frame->get_dimensions()[0]);
  BOOST_CHECK_EQUAL(atoi(doc["shape"][1].GetString()), frame->get_dimensions()[1]);


  //TEST DATA CONTENTS
  recv_socket.recv_raw(&pbuf); //put data into arary
  std::vector<uint8_t> buf(frame->get_data_size()); //create vector of size of data
  std::copy(pbuf, pbuf + frame->get_data_size(), buf.begin()); //copy data from array to vector, so it can be compared
  std::vector<uint8_t> original(img_8, img_8 + sizeof img_8 / sizeof img_8[0]);
  BOOST_CHECK_EQUAL_COLLECTIONS(buf.begin(), buf.end() , original.begin(), original.end()); //collection comparison needs beginning and end of each vector


  //TEST DOWNSCALE OPTION
  while(recv_socket.poll(10))
  {
    recv_socket.recv(); //clear any extra data from the above while loop
  }
  //process all frames. with a downscale factor of 2, this should return all the even numbered frames.
  for(int i = 0; i < frames.size(); i++)
  {
    plugin.process_frame(frames[i]);
  }

  std::vector<std::string> processed_frames;
  while(recv_socket.poll(10))
  {
    std::string tmp_string = recv_socket.recv();
    processed_frames.push_back(tmp_string);
    recv_socket.recv_raw(pbuf); //we dont need the data for this test but still need to read from the socket to clear it

  }
  BOOST_CHECK_EQUAL(processed_frames.size(), frames.size()/FrameProcessor::LiveViewPlugin::DEFAULT_FRAME_FREQ);


  //TEST CONFIGURATION
  cfg.set_param(FrameProcessor::LiveViewPlugin::CONFIG_FRAME_FREQ, 1);
  cfg.set_param(FrameProcessor::LiveViewPlugin::CONFIG_DATASET_NAME, std::string("data"));
  cfg.set_param(FrameProcessor::LiveViewPlugin::CONFIG_SOCKET_ADDR, std::string("tcp://*:1337"));

  BOOST_CHECK_NO_THROW(plugin.configure(cfg, reply));

  //send frame again to check its going to a different socket

  for(int i = 0; i < 10; i ++)
  {
    plugin.process_frame(frame);
    BOOST_CHECK(!recv_socket.poll(100));
    new_socket_recieve = recv_socket_other.poll(100);
    if(new_socket_recieve)
    {
      break;
    }
  }
  BOOST_CHECK(new_socket_recieve);

  //TEST DATASET FILTERING
  while(recv_socket_other.poll(10))
  {
    recv_socket_other.recv(); //clear any extra data from the above while loop
  }

  for(int i = 0; i < frames.size(); i++)
  {
    plugin.process_frame(frames[i]);
  }

  std::vector<std::string> dataset_processed_frames;
  while(recv_socket_other.poll(10))
  {
    std::string tmp_string = recv_socket_other.recv();
    std::cout << "Received Header: " << tmp_string <<std::endl;
    dataset_processed_frames.push_back(tmp_string);
    recv_socket_other.recv_raw(pbuf); //we dont need the data for this test but still need to read from the socket to clear it from the queue
  }
  BOOST_CHECK_EQUAL(dataset_processed_frames.size(), 7);

  //test other data types
  plugin.process_frame(frame_16);
  message = recv_socket_other.recv();
  BOOST_TEST_MESSAGE(message);
  recv_socket_other.recv_raw(pbuf_16);
  std::vector<uint16_t> buf_16(12);
  std::copy(pbuf_16, pbuf_16 + frame_16->get_data_size()/2, buf_16.begin());
  std::vector<uint16_t> original_16(img_16, img_16 + sizeof img_16 / sizeof img_16[0]);
  BOOST_CHECK_EQUAL_COLLECTIONS(buf_16.begin(), buf_16.end(), original_16.begin(), original_16.end());

  doc.Parse(message.c_str());
  BOOST_CHECK_EQUAL(doc["dsize"].GetInt() , frame_16->get_data_size());
  BOOST_CHECK_EQUAL(doc["dtype"].GetString(), DATA_TYPES[frame_16->get_data_type()]);

}

BOOST_AUTO_TEST_SUITE_END(); //LiveViewPluginUnitTest

BOOST_FIXTURE_TEST_SUITE(FileWriterPluginTestUnitTest, FileWriterPluginTestFixture);

BOOST_AUTO_TEST_CASE( FileWriterPluginCalcNumFramesTest1fpb )
{
  OdinData::IpcMessage reply;
  FrameProcessor::FileWriterPlugin fwp0;
  {
    OdinData::IpcMessage cfg;
    cfg.set_param("process/number", 4);
    cfg.set_param("process/rank", 0);
    cfg.set_param("process/frames_per_block", 1);
    fwp0.configure(cfg, reply);
  }
  FrameProcessor::FileWriterPlugin fwp1;
  {
    OdinData::IpcMessage cfg;
    cfg.set_param("process/number", 4);
    cfg.set_param("process/rank", 1);
    cfg.set_param("process/frames_per_block", 1);
    fwp1.configure(cfg, reply);
  }
  FrameProcessor::FileWriterPlugin fwp2;
  {
    OdinData::IpcMessage cfg;
    cfg.set_param("process/number", 4);
    cfg.set_param("process/rank", 2);
    cfg.set_param("process/frames_per_block", 1);
    fwp2.configure(cfg, reply);
  }
  FrameProcessor::FileWriterPlugin fwp3;
  {
    OdinData::IpcMessage cfg;
    cfg.set_param("process/number", 4);
    cfg.set_param("process/rank", 3);
    cfg.set_param("process/frames_per_block", 1);
    fwp3.configure(cfg, reply);
  }

  size_t num_frames_0 = fwp0.calc_num_frames(1141);
  BOOST_CHECK_EQUAL(286, num_frames_0);
  size_t num_frames_1 = fwp1.calc_num_frames(1141);
  BOOST_CHECK_EQUAL(285, num_frames_1);
  size_t num_frames_2 = fwp2.calc_num_frames(1141);
  BOOST_CHECK_EQUAL(285, num_frames_2);
  size_t num_frames_3 = fwp3.calc_num_frames(1141);
  BOOST_CHECK_EQUAL(285, num_frames_3);

  num_frames_0 = fwp0.calc_num_frames(3);
  BOOST_CHECK_EQUAL(1, num_frames_0);
  num_frames_1 = fwp1.calc_num_frames(3);
  BOOST_CHECK_EQUAL(1, num_frames_1);
  num_frames_2 = fwp2.calc_num_frames(3);
  BOOST_CHECK_EQUAL(1, num_frames_2);
  num_frames_3 = fwp3.calc_num_frames(3);
  BOOST_CHECK_EQUAL(0, num_frames_3);

}

BOOST_AUTO_TEST_CASE( FileWriterPluginCalcNumFramesTest1000fpb )
{
  OdinData::IpcMessage reply;
  FrameProcessor::FileWriterPlugin fwp0;
  {
    OdinData::IpcMessage cfg;
    cfg.set_param("process/number", 4);
    cfg.set_param("process/rank", 0);
    cfg.set_param("process/frames_per_block", 1000);
    fwp0.configure(cfg, reply);
  }
  FrameProcessor::FileWriterPlugin fwp1;
  {
    OdinData::IpcMessage cfg;
    cfg.set_param("process/number", 4);
    cfg.set_param("process/rank", 1);
    cfg.set_param("process/frames_per_block", 1000);
    fwp1.configure(cfg, reply);
  }
  FrameProcessor::FileWriterPlugin fwp2;
  {
    OdinData::IpcMessage cfg;
    cfg.set_param("process/number", 4);
    cfg.set_param("process/rank", 2);
    cfg.set_param("process/frames_per_block", 1000);
    fwp2.configure(cfg, reply);
  }
  FrameProcessor::FileWriterPlugin fwp3;
  {
    OdinData::IpcMessage cfg;
    cfg.set_param("process/number", 4);
    cfg.set_param("process/rank", 3);
    cfg.set_param("process/frames_per_block", 1000);
    fwp3.configure(cfg, reply);
  }

  size_t num_frames_0 = fwp0.calc_num_frames(1141);
  BOOST_CHECK_EQUAL(1000, num_frames_0);
  size_t num_frames_1 = fwp1.calc_num_frames(1141);
  BOOST_CHECK_EQUAL(141, num_frames_1);
  size_t num_frames_2 = fwp2.calc_num_frames(1141);
  BOOST_CHECK_EQUAL(0, num_frames_2);
  size_t num_frames_3 = fwp3.calc_num_frames(1141);
  BOOST_CHECK_EQUAL(0, num_frames_3);

  num_frames_0 = fwp0.calc_num_frames(3);
  BOOST_CHECK_EQUAL(3, num_frames_0);
  num_frames_1 = fwp1.calc_num_frames(3);
  BOOST_CHECK_EQUAL(0, num_frames_1);
  num_frames_2 = fwp2.calc_num_frames(3);
  BOOST_CHECK_EQUAL(0, num_frames_2);
  num_frames_3 = fwp3.calc_num_frames(3);
  BOOST_CHECK_EQUAL(0, num_frames_3);

  num_frames_0 = fwp0.calc_num_frames(33999);
  BOOST_CHECK_EQUAL(9000, num_frames_0);
  num_frames_1 = fwp1.calc_num_frames(33999);
  BOOST_CHECK_EQUAL(8999, num_frames_1);
  num_frames_2 = fwp2.calc_num_frames(33999);
  BOOST_CHECK_EQUAL(8000, num_frames_2);
  num_frames_3 = fwp3.calc_num_frames(33999);
  BOOST_CHECK_EQUAL(8000, num_frames_3);

}

BOOST_AUTO_TEST_SUITE_END(); //FileWriterPluginTest

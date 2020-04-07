/*
 * FileWriterTest.cpp
 *
 */
#include "DebugLevelLogger.h"

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
#include "DataBlockFrame.h"
#include "FileWriterPlugin.h"
#include "Acquisition.h"
#include "FrameProcessorDefinitions.h"
#include "ParameterAdjustmentPlugin.h"
#include "OffsetAdjustmentPlugin.h"
#include "LiveViewPlugin.h"
#include "SumPlugin.h"
#include "FrameProcessorDefinitions.h"
#include "TestHelperFunctions.h"

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
  int initial_block_index_count = FrameProcessor::DataBlock::get_current_index_count();
  BOOST_CHECK_NO_THROW(block1 = boost::shared_ptr<FrameProcessor::DataBlock>(new FrameProcessor::DataBlock(1024)));
  BOOST_CHECK_EQUAL(block1->get_index(), initial_block_index_count);
  BOOST_CHECK_EQUAL(block1->get_size(), 1024);
  BOOST_CHECK_NO_THROW(block2 = boost::shared_ptr<FrameProcessor::DataBlock>(new FrameProcessor::DataBlock(2048)));
  BOOST_CHECK_EQUAL(block2->get_index(), initial_block_index_count + 1);
  BOOST_CHECK_EQUAL(block2->get_size(), 2048);
  memset(data1, 1, 1024);
  BOOST_CHECK_NO_THROW(block1->copy_data(data1, 1024));
  memset(data2, 2, 2048);
  BOOST_CHECK_NO_THROW(block2->copy_data(data2, 2048));
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

BOOST_AUTO_TEST_SUITE_END(); //DataBlockUnitTest

BOOST_AUTO_TEST_SUITE(FrameUnitTest);

BOOST_AUTO_TEST_CASE( DataBlockFrameTest )
{
  unsigned short img[12] =  { 1, 2, 3, 4,
                              5, 6, 7, 8,
                              9,10,11,12 };
  dimensions_t img_dims(2); img_dims[0] = 3; img_dims[1] = 4;

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

    dset_def.name = "data";
    dset_def.num_frames = 10;
    dset_def.data_type = FrameProcessor::raw_16bit;
    dset_def.frame_dimensions = dimensions_t(2);
    dset_def.frame_dimensions[0] = 3;
    dset_def.frame_dimensions[1] = 4;
    dset_def.chunks = chunk_dims;
    dset_def.create_low_high_indexes = false;

    FrameProcessor::FrameMetaData frame_meta(
            7, "data", FrameProcessor::raw_16bit, "test", img_dims, FrameProcessor::no_compression
    );
    frame = boost::shared_ptr<FrameProcessor::DataBlockFrame>(
        new FrameProcessor::DataBlockFrame(frame_meta, static_cast<void*>(img), 24)
    );

    for (unsigned short i = 0; i < 10; i++)
    {
      FrameProcessor::FrameMetaData loop_frame_meta(
              i, "data", FrameProcessor::raw_16bit, "test", img_dims, FrameProcessor::no_compression
      );
      boost::shared_ptr<FrameProcessor::DataBlockFrame> tmp_frame(
          new FrameProcessor::DataBlockFrame(loop_frame_meta, static_cast<void*>(img), 24));
      img[0] = i;
      frames.push_back(tmp_frame);
    }
  }
  ~FileWriterPluginTestFixture() {}
  boost::shared_ptr<FrameProcessor::DataBlockFrame> frame;
  std::vector< boost::shared_ptr<FrameProcessor::DataBlockFrame> >frames;
  FrameProcessor::FileWriterPlugin fw;
  FrameProcessor::DatasetDefinition dset_def;
};

BOOST_FIXTURE_TEST_SUITE(HDF5FileUnitTest, FileWriterPluginTestFixture);

BOOST_AUTO_TEST_CASE( HDF5FileTest )
{
  FrameProcessor::HDF5File hdf5f;

  BOOST_REQUIRE_NO_THROW(hdf5f.create_file("/tmp/blah.h5", 0, false, 1, 1));
  BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));
  BOOST_REQUIRE_EQUAL(dset_def.name, frame->get_meta_data().get_dataset_name());

  BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*frame, frame->get_frame_number(), 1));
  BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE( HDF5FileMultiDatasetTest )
{
  FrameProcessor::HDF5File hdf5f;

  BOOST_REQUIRE_NO_THROW(hdf5f.create_file("/tmp/blah_multidataset.h5", 0, false, 1, 1));

  // Create the first dataset "data"
  BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));
  BOOST_CHECK_EQUAL(dset_def.name, frame->get_meta_data().get_dataset_name());

  // Create the second dataset "stuff"
  dset_def.name = "stuff";
  BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));

  // Write first frame to "data"
  BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*frame, frame->get_frame_number(), 1));

  // Write the frame to "stuff"
  BOOST_CHECK_NO_THROW(frame->meta_data().set_dataset_name("stuff"));
  BOOST_CHECK_EQUAL(dset_def.name, frame->get_meta_data().get_dataset_name());
  BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*frame, frame->get_frame_number(), 1));

  // write another frame to "data"
  BOOST_CHECK_EQUAL("data", frames[2]->get_meta_data().get_dataset_name());
  BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*frames[2], frames[2]->get_frame_number(), 1));
  // and yet another frame to "stuff"
  BOOST_CHECK_NO_THROW(frames[2]->meta_data().set_dataset_name("stuff"));
  BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*frames[2], frames[2]->get_frame_number(), 1));

  BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE( HDF5FileBadFileTest )
{
  FrameProcessor::HDF5File hdf5f;

  // Check for an error when a bad file path is provided
  BOOST_CHECK_THROW(hdf5f.create_file("/non/existent/path/blah_throw.h5", 0, false, 1, 1), std::runtime_error);
}

BOOST_AUTO_TEST_CASE( FileWriterPluginDatasetWithoutOpenFileTest )
{
  FrameProcessor::HDF5File hdf5f;

  // Check for an error when a dataset is created without a file open
  BOOST_CHECK_THROW(hdf5f.create_dataset(dset_def, -1, -1), std::runtime_error);
}

BOOST_AUTO_TEST_CASE( HDF5FileNoDatasetDefinitionsTest )
{
  FrameProcessor::HDF5File hdf5f;

  // Create a file ready for writing
  BOOST_REQUIRE_NO_THROW(hdf5f.create_file("/tmp/blah_throw.h5", 0, false, 1, 1));

  // Write a frame without first creating the dataset
  BOOST_CHECK_THROW(hdf5f.write_frame(*frame, frame->get_frame_number(), 1), std::runtime_error);

  // Attempt to close the file
  BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE( HDF5FileInvalidDatasetTest )
{
  FrameProcessor::HDF5File hdf5f;

  BOOST_REQUIRE_NO_THROW(hdf5f.create_file("/tmp/blah_throw.h5", 0, false, 1, 1));
  BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));
  BOOST_REQUIRE_NO_THROW(frame->meta_data().set_dataset_name("non_existing_dataset_name"));

  BOOST_CHECK_THROW(hdf5f.write_frame(*frame, frame->get_frame_number(), 1), std::runtime_error);
  BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE( FileWriterPluginMultipleFramesTest )
{
  FrameProcessor::HDF5File hdf5f;

  BOOST_REQUIRE_NO_THROW(hdf5f.create_file("/tmp/blah_multiple.h5", 0, false, 1, 1));
  BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));

  std::vector<boost::shared_ptr<FrameProcessor::DataBlockFrame> >::iterator it;
  for (it = frames.begin(); it != frames.end(); ++it) {
    BOOST_TEST_MESSAGE("Writing frame: " <<  (*it)->get_frame_number());
    BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*(*it), (*it)->get_frame_number(), 1));
  }
  BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE( HDF5FileMultipleReverseTest )
{
  FrameProcessor::HDF5File hdf5f;

  // Just reverse through the list of frames and write them out.
  // The frames should still appear in the file in the original order...
  BOOST_REQUIRE_NO_THROW(hdf5f.create_file("/tmp/blah_multiple_reverse.h5", 0, false, 1, 1));
  BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));

  // The first frame to write is used as an offset - so must be the lowest
  // frame number. Frames received later with a smaller number would result
  // in negative file index...
  BOOST_TEST_MESSAGE("Writing frame: " << frame->get_frame_number());
  BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*frame, frame->get_frame_number(), 1));

  std::vector<boost::shared_ptr<FrameProcessor::DataBlockFrame> >::reverse_iterator rit;
  for (rit = frames.rbegin(); rit != frames.rend(); ++rit) {
    BOOST_TEST_MESSAGE("Writing frame: " <<  (*rit)->get_frame_number());
    BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*(*rit), (*rit)->get_frame_number(), 1));
  }
  BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE( HDF5FileAdjustHugeOffset )
{
  FrameProcessor::HDF5File hdf5f;

  BOOST_REQUIRE_NO_THROW(hdf5f.create_file("/tmp/test_huge_offset.h5", 0, false, 1, 1));
  BOOST_REQUIRE_NO_THROW(hdf5f.set_unlimited());
  BOOST_REQUIRE_NO_THROW(dset_def.num_frames = 0);
  BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(dset_def, -1, -1));

  hsize_t huge_offset = 100000;

  std::vector<boost::shared_ptr<FrameProcessor::DataBlockFrame> >::iterator it;
  for (it = frames.begin(); it != frames.end(); ++it){
    size_t frame_no = (*it)->get_frame_number();
    //PercivalEmulator::FrameHeader img_header = *((*it)->get_header());
    //img_header.frame_number = frame_no + huge_offset;
    //(*it)->copy_header(&img_header);
    (*it)->meta_data().set_frame_offset(huge_offset);
    (*it)->set_frame_number(frame_no + huge_offset);
    BOOST_REQUIRE_NO_THROW(hdf5f.write_frame(*(*it), (*it)->get_frame_number(), 1));
  }
  BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE( FileWriterPluginWriteParamTest )
{
  FrameProcessor::HDF5File hdf5f;

  BOOST_REQUIRE_NO_THROW(hdf5f.create_file("/tmp/test_write_params.h5", 0, false, 1, 1));

  FrameProcessor::DatasetDefinition param_dset_def;
  param_dset_def.name = "p1";
  param_dset_def.data_type = FrameProcessor::raw_64bit;
  param_dset_def.num_frames = 10;
  dimensions_t chunk_dims(1);
  chunk_dims[0] = 1;
  param_dset_def.chunks = chunk_dims;

  BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(param_dset_def, -1, -1));

  std::vector<boost::shared_ptr<FrameProcessor::DataBlockFrame> >::iterator it;
  uint64_t val = 0;
  for (it = frames.begin(); it != frames.end(); ++it) {
    (*it)->meta_data().set_parameter("p1", val);
    BOOST_TEST_MESSAGE("Writing frame: " <<  (*it)->get_frame_number());
    BOOST_REQUIRE_NO_THROW(hdf5f.write_parameter(*(*it), param_dset_def, (*it)->get_frame_number()));
    val++;
  }
  BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE( FileWriterPluginWriteParamWrongTypeTest )
{
  FrameProcessor::HDF5File hdf5f;

  BOOST_REQUIRE_NO_THROW(hdf5f.create_file("/tmp/test_wrong_params.h5", 0, false, 1, 1));

  FrameProcessor::DatasetDefinition param_dset_def;
  param_dset_def.name = "p1";
  param_dset_def.data_type = FrameProcessor::raw_16bit;
  param_dset_def.num_frames = 10;
  dimensions_t chunk_dims(1);
  chunk_dims[0] = 1;
  param_dset_def.chunks = chunk_dims;

  BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(param_dset_def, -1, -1));

  std::vector<boost::shared_ptr<FrameProcessor::DataBlockFrame> >::iterator it;
  for (it = frames.begin(); it != frames.end(); ++it) {
    uint64_t val = 123;
    (*it)->meta_data().set_parameter("p1", val);
    BOOST_TEST_MESSAGE("Writing frame: " <<  (*it)->get_frame_number());
    BOOST_CHECK_THROW(hdf5f.write_parameter(*(*it), param_dset_def, (*it)->get_frame_number()), std::runtime_error);
  }
  BOOST_REQUIRE_NO_THROW(hdf5f.close_file());
}

BOOST_AUTO_TEST_CASE( FileWriterPluginWriteParamNoParamTest )
{
  FrameProcessor::HDF5File hdf5f;

  BOOST_REQUIRE_NO_THROW(hdf5f.create_file("/tmp/test_no_params.h5", 0, false, 1, 1));

  FrameProcessor::DatasetDefinition param_dset_def;
  param_dset_def.name = "p1";
  param_dset_def.data_type = FrameProcessor::raw_64bit;
  param_dset_def.num_frames = 10;
  dimensions_t chunk_dims(1);
  chunk_dims[0] = 1;
  param_dset_def.chunks = chunk_dims;

  BOOST_REQUIRE_NO_THROW(hdf5f.create_dataset(param_dset_def, -1, -1));

  std::vector<boost::shared_ptr<FrameProcessor::DataBlockFrame> >::iterator it;
  for (it = frames.begin(); it != frames.end(); ++it) {
    uint64_t val = 123;
    (*it)->meta_data().set_parameter("p2", val);
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

  boost::shared_ptr<FrameProcessor::DataBlockFrame> frame = get_dummy_frame();
  frame->meta_data().set_frame_offset(0);

  size_t adjusted_offset = acquisition.adjust_frame_offset(frame);
  BOOST_CHECK_EQUAL(0, adjusted_offset);

  frame->set_frame_number(1);
  adjusted_offset = acquisition.adjust_frame_offset(frame);
  BOOST_CHECK_EQUAL(1, adjusted_offset);

  frame->set_frame_number(0);
  frame->meta_data().set_frame_offset(1);
  adjusted_offset = acquisition.adjust_frame_offset(frame);
  BOOST_CHECK_EQUAL(1, adjusted_offset);

  frame->set_frame_number(1);
  frame->meta_data().set_frame_offset(1);
  adjusted_offset = acquisition.adjust_frame_offset(frame);
  BOOST_CHECK_EQUAL(2, adjusted_offset);

  frame->set_frame_number(1);
  frame->meta_data().set_frame_offset(-1);
  adjusted_offset = acquisition.adjust_frame_offset(frame);
  BOOST_CHECK_EQUAL(0, adjusted_offset);

  frame->set_frame_number(0);
  frame->meta_data().set_frame_offset(-1);
  BOOST_CHECK_THROW(acquisition.adjust_frame_offset(frame), std::range_error);
}

BOOST_AUTO_TEST_CASE( AcquisitionFrameVerification )
{
  FrameProcessor::Acquisition acquisition;

  FrameProcessor::DatasetDefinition dset_def;
  // Provide default values for the dataset
  dset_def.name = "raw";
  dset_def.data_type = FrameProcessor::raw_16bit;
  dset_def.compression = FrameProcessor::no_compression;
  dset_def.blosc_compressor = 0;
  dset_def.blosc_level = 0;
  dset_def.blosc_shuffle = 0;
  dset_def.num_frames = 1;
  std::vector<long long unsigned int> dims(2, 0);
  dset_def.frame_dimensions = dims;
  dset_def.chunks = dims;
  dset_def.create_low_high_indexes = false;

  acquisition.dataset_defs_["raw"] = dset_def;

  boost::shared_ptr<FrameProcessor::DataBlockFrame> frame = get_dummy_frame();
  frame->meta_data().set_compression_type(FrameProcessor::unknown_compression);

  bool verified = acquisition.check_frame_valid(frame);
  BOOST_CHECK_EQUAL(false, verified);

  frame = get_dummy_frame();
  frame->meta_data().set_data_type(FrameProcessor::raw_unknown);

  verified = acquisition.check_frame_valid(frame);
  BOOST_CHECK_EQUAL(false, verified);
}

BOOST_AUTO_TEST_SUITE_END(); //AcquisitionUnitTest

BOOST_FIXTURE_TEST_SUITE(ParameterAdjustmentPluginUnitTest, FileWriterPluginTestFixture);

BOOST_AUTO_TEST_CASE( AddParameter )
{
  FrameProcessor::ParameterAdjustmentPlugin plugin;
  boost::shared_ptr<FrameProcessor::DataBlockFrame> frame = get_dummy_frame();

  // Check 0 goes to 0 when no config has been sent, and frame doesn't have parameter
  frame->set_frame_number(0);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(false, frame->get_meta_data().has_parameter("UID"));

  // Check a non 0 goes to the same when no config has been sent
  frame->set_frame_number(27);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(false, frame->get_meta_data().has_parameter("UID"));

  // Configure an offset of 4 for the UID parameter
  OdinData::IpcMessage reply;
  OdinData::IpcMessage cfg;
  cfg.set_param(FrameProcessor::PARAMETER_NAME_CONFIG + "/UID/" + FrameProcessor::PARAMETER_ADJUSTMENT_CONFIG, 4);
  plugin.configure(cfg, reply);

  // Check the offset is applied and the parameter exists when a frame is sent
  frame->set_frame_number(0);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(4, frame->get_meta_data().get_parameter<uint64_t>("UID"));

  // Check the offset is still applied when not on frame 0
  frame->set_frame_number(10);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(14, frame->get_meta_data().get_parameter<uint64_t>("UID"));

  // Configure a new offset
  cfg.set_param(FrameProcessor::PARAMETER_NAME_CONFIG + "/UID/" + FrameProcessor::PARAMETER_ADJUSTMENT_CONFIG, 33);
  plugin.configure(cfg, reply);

  // Check the new offset is applied
  frame->set_frame_number(1);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(34, frame->get_meta_data().get_parameter<uint64_t>("UID"));

  // Check the new offset is applied when on frame 2
  frame->set_frame_number(2);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(35, frame->get_meta_data().get_parameter<uint64_t>("UID"));

  // Configure a negative adjustment value
  cfg.set_param(FrameProcessor::PARAMETER_NAME_CONFIG + "/UID/" + FrameProcessor::PARAMETER_ADJUSTMENT_CONFIG, -2);
  plugin.configure(cfg, reply);

  // Check the new offset is applied ok
  frame->set_frame_number(3);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(1, frame->get_meta_data().get_parameter<uint64_t>("UID"));

}

BOOST_AUTO_TEST_CASE( AdjustExistingParameter )
{
  FrameProcessor::ParameterAdjustmentPlugin plugin;
  boost::shared_ptr<FrameProcessor::DataBlockFrame> frame = get_dummy_frame();

  uint64_t uid = 0;

  // Check 0 goes to 0 when no config has been sent
  frame->set_frame_number(0);
  frame->meta_data().set_parameter("UID", uid);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(0, frame->get_meta_data().get_parameter<uint64_t>("UID"));

  // Check a non 0 goes to the same when no config has been sent
  frame->set_frame_number(27);
  uid = 27;
  frame->meta_data().set_parameter("UID", uid);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(27, frame->get_meta_data().get_parameter<uint64_t>("UID"));

  // Configure an offset of 4 for the UID parameter
  OdinData::IpcMessage reply;
  OdinData::IpcMessage cfg;
  cfg.set_param(FrameProcessor::PARAMETER_NAME_CONFIG + "/UID/" + FrameProcessor::PARAMETER_ADJUSTMENT_CONFIG, 4);
  plugin.configure(cfg, reply);

  // Check the offset is applied and the parameter set when a new first frame is now sent
  frame->set_frame_number(0);
  uid = 0;
  frame->meta_data().set_parameter("UID", uid);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(4, frame->get_meta_data().get_parameter<uint64_t>("UID"));

  // Check the parameter overwrites any existing value
  frame->set_frame_number(10);
  uid = 123;
  frame->meta_data().set_parameter("UID", uid);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(14, frame->get_meta_data().get_parameter<uint64_t>("UID"));
}

BOOST_AUTO_TEST_SUITE_END(); //ParameterAdjustmentPluginUnitTest

BOOST_FIXTURE_TEST_SUITE(OffsetAdjustmentPluginUnitTest, FileWriterPluginTestFixture);

BOOST_AUTO_TEST_CASE( AdjustOffset )
{
  FrameProcessor::OffsetAdjustmentPlugin plugin;

  dimensions_t dims(2, 0);
  FrameProcessor::FrameMetaData frame_meta(
          0, "raw", FrameProcessor::raw_16bit, "test", dims, FrameProcessor::no_compression
  );
  char dummy_data[2] = {0, 0};

  boost::shared_ptr<FrameProcessor::DataBlockFrame> frame(
          new FrameProcessor::DataBlockFrame(frame_meta, static_cast<void*>(dummy_data), 2));
  uint64_t offset = 0;
  frame->meta_data().set_frame_offset(offset);

  // Check 0 goes to 0 when no config has been sent
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(0, frame->get_meta_data().get_frame_offset());

  // Check a non 0 goes to the same when no config has been sent
  offset = 27;
  frame->set_frame_number(offset);
  frame->meta_data().set_frame_offset(offset);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(27, frame->get_meta_data().get_frame_offset());

  // Configure a frame offset of 4
  OdinData::IpcMessage reply;
  OdinData::IpcMessage cfg;
  cfg.set_param(FrameProcessor::OFFSET_ADJUSTMENT_CONFIG, 4);
  plugin.configure(cfg, reply);

  // Check the offset is applied when a new frame is now sent
  offset = 0;
  frame->set_frame_number(offset);
  frame->meta_data().set_frame_offset(offset);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(4, frame->get_meta_data().get_frame_offset());

  // Check the offset is still applied when not on frame 0
  offset = 10;
  frame->set_frame_number(offset);
  frame->meta_data().set_frame_offset(offset);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(14, frame->get_meta_data().get_frame_offset());

  // Check the offset is still applied when offset is different to frame number
  offset = 100;
  frame->set_frame_number(27);
  frame->meta_data().set_frame_offset(offset);
  plugin.process_frame(frame);
  BOOST_CHECK_EQUAL(104, frame->get_meta_data().get_frame_offset());
}

BOOST_AUTO_TEST_SUITE_END(); //UIDAdjustmentPluginUnitTest

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

BOOST_AUTO_TEST_SUITE(SumPluginUnitTest);

BOOST_AUTO_TEST_CASE( SumFrame )
{
  FrameProcessor::SumPlugin plugin;
  unsigned short img[12] =  {
    1, 2, 3, 4,
    5, 6, 7, 8,
    9, 10, 11, 12
  };
  dimensions_t img_dims(2); img_dims[0] = 3; img_dims[1] = 4;

  FrameProcessor::FrameMetaData frame_meta(
          1, "raw", FrameProcessor::raw_16bit, "test", img_dims, FrameProcessor::no_compression
  );

  boost::shared_ptr<FrameProcessor::DataBlockFrame> frame(
          new FrameProcessor::DataBlockFrame(frame_meta, static_cast<void*>(img), 24));

  plugin.process_frame(frame);

  BOOST_CHECK(frame->get_meta_data().has_parameter(FrameProcessor::SUM_PARAM_NAME));

  // check that the sum of each pixel is the correct value
  BOOST_CHECK_EQUAL(78, frame->get_meta_data().get_parameter<uint64_t>(FrameProcessor::SUM_PARAM_NAME));
}

BOOST_AUTO_TEST_CASE( SumEmptyFrame )
{
  FrameProcessor::SumPlugin plugin;

  dimensions_t dims(2, 0);
  FrameProcessor::FrameMetaData frame_meta(
          0, "raw", FrameProcessor::raw_16bit, "test", dims, FrameProcessor::no_compression
  );
  char dummy_data[2] = {0, 0};
  boost::shared_ptr<FrameProcessor::DataBlockFrame> frame(
          new FrameProcessor::DataBlockFrame(frame_meta, static_cast<void*>(dummy_data), 2));

  plugin.process_frame(frame);
  BOOST_CHECK(frame->get_meta_data().has_parameter(FrameProcessor::SUM_PARAM_NAME));
  // check that a empty frame sum is zero
  BOOST_CHECK_EQUAL(0, frame->get_meta_data().get_parameter<uint64_t>(FrameProcessor::SUM_PARAM_NAME));
}

BOOST_AUTO_TEST_CASE( SumNotSupportedDataType )
{
  FrameProcessor::SumPlugin plugin;
  // check that sum parameter is not set in unsupported data types
  boost::shared_ptr<FrameProcessor::DataBlockFrame> frame = get_dummy_frame();
  frame->meta_data().set_data_type(FrameProcessor::raw_float);
  plugin.process_frame(frame);
  BOOST_CHECK(!frame->get_meta_data().has_parameter(FrameProcessor::SUM_PARAM_NAME));

}

BOOST_AUTO_TEST_SUITE_END(); //SumPluginUnitTest

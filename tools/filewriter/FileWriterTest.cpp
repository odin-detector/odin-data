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

#include "FileWriter.h"
#include "Frame.h"

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


BOOST_AUTO_TEST_SUITE(FrameUnitTest);

BOOST_AUTO_TEST_CASE( FrameTest )
{
    unsigned short img[12] =  { 1, 2, 3, 4,
                                5, 6, 7, 8,
                                9,10,11,12 };
    dimensions_t img_dims(2); img_dims[0] = 3; img_dims[1] = 4;
    FrameHeader img_header;
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


class FileWriterTestFixture
{
public:
    FileWriterTestFixture()
    {
        unsigned short img[12] =  { 1, 2, 3, 4,
                                    5, 6, 7, 8,
                                    9,10,11,12 };
        dimensions_t img_dims(2); img_dims[0] = 3; img_dims[1] = 4;
        FrameHeader img_header;
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
        FrameHeader img_header = *((*it)->get_header());
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


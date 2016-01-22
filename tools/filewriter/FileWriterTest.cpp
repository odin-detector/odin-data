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
#include "framenotifier_data.h"

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

        for (int i = 0; i<5; i++)
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

    std::vector< boost::shared_ptr<Frame> >::reverse_iterator rit;
    for (rit = frames.rbegin(); rit != frames.rend(); ++rit){
        BOOST_TEST_MESSAGE("Writing frame: " <<  (*rit)->get_frame_number());
        BOOST_REQUIRE_NO_THROW(fw.writeFrame(*(*rit)));
    }
    BOOST_REQUIRE_NO_THROW(fw.closeFile());
}


BOOST_AUTO_TEST_SUITE_END(); //FileWriterUnitTest


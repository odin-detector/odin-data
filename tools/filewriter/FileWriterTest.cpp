/*
 * FileWriterTest.cpp
 *
 */
#define BOOST_TEST_MODULE "FileWriterUnitTests"
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

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

}

BOOST_AUTO_TEST_SUITE_END(); //FrameUnitTest


class FileWriterTestFixture
{
public:
    FileWriterTestFixture() //: logger(log4cxx::Logger::getLogger("FileWriterUnitTest"))
    {}
    ~FileWriterTestFixture(){}
    log4cxx::LoggerPtr logger;
};
BOOST_FIXTURE_TEST_SUITE(FileWriterUnitTest, FileWriterTestFixture);

BOOST_AUTO_TEST_CASE( FileWriterTest )
{
    FileWriter fw;
    BOOST_CHECK_NO_THROW(fw.createFile("/tmp/blah.h5"));
}

BOOST_AUTO_TEST_SUITE_END(); //FileWriterUnitTest


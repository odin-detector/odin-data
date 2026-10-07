/*
 * FrameMetaDataTest.cpp
 *
 *  Created on: 07 Oct 2026
 *      Author: Famous Alele
 */
#define BOOST_TEST_MODULE "FrameMetaDataTests"
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>

#include "DebugLevelLogger.h"
#include "FrameMetaData.h"

#include <log4cxx/basicconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/logger.h>
#include <log4cxx/simplelayout.h>
#include <log4cxx/xml/domconfigurator.h>
using namespace log4cxx;
using namespace log4cxx::xml;

class LocalConfig {
public:
    LocalConfig()
    {
        consoleAppender = new ConsoleAppender(LayoutPtr(new SimpleLayout()));
        BasicConfigurator::configure(AppenderPtr(consoleAppender));
        Logger::getRootLogger()->setLevel(Level::getWarn());
        set_debug_level(3);
    }
    ~LocalConfig() { };

private:
    ConsoleAppender* consoleAppender;
};

static bool is_not_critical1(const std::out_of_range& er)
{
    return true;
}

static bool is_not_critical2(const boost::bad_get& er)
{
    return true;
}

BOOST_GLOBAL_FIXTURE(LocalConfig);
BOOST_AUTO_TEST_SUITE(FrameMeaDataUnitTest);

BOOST_AUTO_TEST_CASE(FrameMetaDataSetGetParam)
{
    namespace FP = FrameProcessor;
    using FPFMD = FP::FrameMetaData;
    FPFMD frame_md { 123, "Test_Ds", FP::DataType::raw_64bit, "Acq_1", { 3, 4, 5 } };

    BOOST_CHECK(frame_md.get_dataset_name() == "Test_Ds");
    BOOST_CHECK(frame_md.get_acquisition_ID() == "Acq_1");
    BOOST_CHECK(frame_md.get_frame_number() == 123);
    BOOST_CHECK(frame_md.get_data_type() == FP::DataType::raw_64bit);
    BOOST_CHECK((frame_md.get_dimensions() == std::vector<::dimsize_t> { 3, 4, 5 }));

    BOOST_REQUIRE_NO_THROW(frame_md.set_dataset_name("New_Dataset"));
    BOOST_CHECK(frame_md.get_dataset_name() == "New_Dataset");
    BOOST_REQUIRE_NO_THROW(frame_md.set_acquisition_ID("Acq_2"));
    BOOST_CHECK(frame_md.get_acquisition_ID() == "Acq_2");
    frame_md.set_data_type(FP::DataType::raw_8bit);
    BOOST_CHECK(frame_md.get_data_type() == FP::DataType::raw_8bit);
    BOOST_REQUIRE_NO_THROW(frame_md.set_dimensions(std::vector<::dimsize_t> { 6, 7, 8 }));
    BOOST_CHECK((frame_md.get_dimensions() == std::vector<::dimsize_t> { 6, 7, 8 }));
    frame_md.set_frame_number(789);
    BOOST_CHECK(frame_md.get_frame_number() == 789);
    frame_md.set_frame_offset(32);
    BOOST_CHECK(frame_md.get_frame_offset() == 32);
    frame_md.adjust_frame_offset(16);
    BOOST_CHECK(frame_md.get_frame_offset() == 48);

    BOOST_REQUIRE_NO_THROW(frame_md.set_parameter("param1", 777UL));
    BOOST_REQUIRE_EXCEPTION(frame_md.get_parameter<float>("invalid"), std::out_of_range, is_not_critical1);
    BOOST_REQUIRE_EXCEPTION(frame_md.get_parameter<float>("param1"), boost::bad_get, is_not_critical2);
    BOOST_CHECK(frame_md.get_parameter<uint64_t>("param1") == 777UL);
}

BOOST_AUTO_TEST_CASE(FrameMetaDataTestParamType)
{
    namespace FP = FrameProcessor;
    using FPFMD = FP::FrameMetaData;
    FPFMD frame_md { 123, "Test_Ds", FP::DataType::raw_64bit, "Acq_1", { 3, 4, 5 } };
    frame_md.set_parameter("param2", 3.142f);
    BOOST_CHECK(frame_md.is_type<uint64_t>("param2") == false);
    BOOST_CHECK(frame_md.is_type<float>("param2") == true);
}

BOOST_AUTO_TEST_CASE(FrameMetaDataTestHasParam)
{
    namespace FP = FrameProcessor;
    using FPFMD = FP::FrameMetaData;
    FPFMD frame_md { 123, "Test_Ds", FP::DataType::raw_64bit, "Acq_1", { 3, 4, 5 } };
    frame_md.set_parameter("param3", (unsigned short)47);
    BOOST_CHECK(frame_md.has_parameter("param2") == false);
    BOOST_CHECK(frame_md.is_type<float>("param3") == false);
    BOOST_CHECK(frame_md.is_type<unsigned short>("param3") == true);
}

BOOST_AUTO_TEST_SUITE_END(); // FrameMetaDataUnitTest

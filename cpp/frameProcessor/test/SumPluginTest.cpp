#define BOOST_TEST_MODULE "SumPluginTests"
#define BOOST_TEST_MAIN

#include "Fixtures.h"
#include "TestHelperFunctions.h"

#include "SumPlugin.h"

BOOST_GLOBAL_FIXTURE(GlobalConfig);

BOOST_AUTO_TEST_SUITE(SumPluginUnitTest);

BOOST_AUTO_TEST_CASE(SumFrame)
{
    FrameProcessor::SumPlugin plugin;
    unsigned short img[12] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
    dimensions_t img_dims(2);
    img_dims[0] = 3;
    img_dims[1] = 4;

    FrameProcessor::FrameMetaData frame_meta(
        1, "raw", FrameProcessor::raw_16bit, "test", img_dims, FrameProcessor::no_compression
    );

    boost::shared_ptr<FrameProcessor::DataBlockFrame> frame(
        new FrameProcessor::DataBlockFrame(frame_meta, static_cast<void*>(img), 24)
    );

    plugin.process_frame(frame);

    BOOST_CHECK(frame->get_meta_data().has_parameter(FrameProcessor::SUM_PARAM_NAME));

    // check that the sum of each pixel is the correct value
    BOOST_CHECK_EQUAL(
        78, boost::get<uint64_t>(frame->get_meta_data().get_parameter<uint64_t>(FrameProcessor::SUM_PARAM_NAME))
    );
}

BOOST_AUTO_TEST_CASE(SumEmptyFrame)
{
    FrameProcessor::SumPlugin plugin;

    dimensions_t dims(2, 0);
    FrameProcessor::FrameMetaData frame_meta(
        0, "raw", FrameProcessor::raw_16bit, "test", dims, FrameProcessor::no_compression
    );
    char dummy_data[2] = { 0, 0 };
    boost::shared_ptr<FrameProcessor::DataBlockFrame> frame(
        new FrameProcessor::DataBlockFrame(frame_meta, static_cast<void*>(dummy_data), 2)
    );

    plugin.process_frame(frame);
    BOOST_CHECK(frame->get_meta_data().has_parameter(FrameProcessor::SUM_PARAM_NAME));
    // check that a empty frame sum is zero
    BOOST_CHECK_EQUAL(
        0, boost::get<uint64_t>(frame->get_meta_data().get_parameter<uint64_t>(FrameProcessor::SUM_PARAM_NAME))
    );
}

BOOST_AUTO_TEST_CASE(SumNotSupportedDataType)
{
    FrameProcessor::SumPlugin plugin;
    // check that sum parameter is not set in unsupported data types
    boost::shared_ptr<FrameProcessor::DataBlockFrame> frame = get_dummy_frame();
    frame->meta_data().set_data_type(FrameProcessor::raw_float);
    plugin.process_frame(frame);
    BOOST_CHECK(!frame->get_meta_data().has_parameter(FrameProcessor::SUM_PARAM_NAME));
}

BOOST_AUTO_TEST_SUITE_END(); // SumPluginUnitTest

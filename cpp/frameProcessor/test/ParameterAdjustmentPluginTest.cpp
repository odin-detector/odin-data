#define BOOST_TEST_MODULE "ParameterAdjustmentPluginTests"
#define BOOST_TEST_MAIN

#include "Fixtures.h"
#include "TestHelperFunctions.h"

#include "ParameterAdjustmentPlugin.h"

BOOST_GLOBAL_FIXTURE(GlobalConfig);

BOOST_FIXTURE_TEST_SUITE(ParameterAdjustmentPluginUnitTest, FileWriterPluginTestFixture);

BOOST_AUTO_TEST_CASE(AddParameter)
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

BOOST_AUTO_TEST_CASE(AdjustExistingParameter)
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

BOOST_AUTO_TEST_SUITE_END(); // ParameterAdjustmentPluginUnitTest

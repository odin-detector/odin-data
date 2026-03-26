#define BOOST_TEST_MODULE "OffsetAdjustmentPluginTests"
#define BOOST_TEST_MAIN

#include "Fixtures.h"

#include "OffsetAdjustmentPlugin.h"

BOOST_GLOBAL_FIXTURE(GlobalConfig);

BOOST_FIXTURE_TEST_SUITE(OffsetAdjustmentPluginUnitTest, FileWriterPluginTestFixture);

BOOST_AUTO_TEST_CASE(AdjustOffset)
{
    FrameProcessor::OffsetAdjustmentPlugin plugin;

    dimensions_t dims(2, 0);
    FrameProcessor::FrameMetaData frame_meta(
        0, "raw", FrameProcessor::raw_16bit, "test", dims, FrameProcessor::no_compression
    );
    char dummy_data[2] = { 0, 0 };

    boost::shared_ptr<FrameProcessor::DataBlockFrame> frame(
        new FrameProcessor::DataBlockFrame(frame_meta, static_cast<void*>(dummy_data), 2)
    );
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

BOOST_AUTO_TEST_SUITE_END(); // UIDAdjustmentPluginUnitTest

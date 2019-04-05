/*
 * GapFillPluginTest.cpp
 *
 *  Created on: 5 Feb 2019
 *      Author: Alan Greer
 */

#include <boost/test/unit_test.hpp>
#include <DebugLevelLogger.h>
#include "FrameProcessorDefinitions.h"
#include "GapFillPlugin.h"
#include "IpcMessage.h"
#include "DataBlockFrame.h"

class GapFillPluginTestFixture {
public:
    GapFillPluginTestFixture() {
        set_debug_level(3);
        unsigned short img[12] = {1, 2, 3, 4,
                                  5, 6, 7, 8,
                                  9, 10, 11, 12};
        dimensions_t img_dims(2);
        img_dims[0] = 3;
        img_dims[1] = 4;
        dimensions_t chunk_dims(3);
        chunk_dims[0] = 1;
        chunk_dims[1] = 3;
        chunk_dims[2] = 4;

        FrameProcessor::IFrameMetaData frame_meta(7, "data", FrameProcessor::raw_16bit, "scan1", img_dims, FrameProcessor::no_compression);

        frame = boost::shared_ptr<FrameProcessor::DataBlockFrame>(
                new FrameProcessor::DataBlockFrame(frame_meta, static_cast<void *>(img), 24));

        unsigned short img_2[16] = {1, 1, 2, 2,
                                    1, 1, 2, 2,
                                    3, 3, 4, 4,
                                    3, 3, 4, 4};
        img_dims[0] = 4;
        img_dims[1] = 4;
        chunk_dims[0] = 1;
        chunk_dims[1] = 4;
        chunk_dims[2] = 4;

        FrameProcessor::IFrameMetaData frame_2_meta(
                7, "data", FrameProcessor::raw_16bit, "scan1", img_dims, FrameProcessor::no_compression
        );

        frame_2 = boost::shared_ptr<FrameProcessor::DataBlockFrame>(
                new FrameProcessor::DataBlockFrame(frame_2_meta, static_cast<void *>(img_2), 32));

    }

    ~GapFillPluginTestFixture() {}

    boost::shared_ptr <FrameProcessor::IFrame> frame;
    boost::shared_ptr <FrameProcessor::IFrame> frame_2;
    FrameProcessor::GapFillPlugin gap_fill_plugin;
};

BOOST_FIXTURE_TEST_SUITE(GapFillPluginUnitTest, GapFillPluginTestFixture
);

BOOST_AUTO_TEST_CASE( GapFillPlugin_process_frame )
{
    // Test a chip and grid config that will fail with the incoming frame
    OdinData::IpcMessage reply_bad_cfg_1;
    OdinData::IpcMessage bad_cfg_1;
    BOOST_REQUIRE_NO_THROW(bad_cfg_1.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_SIZE + "[]", 5));
    BOOST_REQUIRE_NO_THROW(bad_cfg_1.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_SIZE + "[]", 4));
    BOOST_REQUIRE_NO_THROW(bad_cfg_1.set_param(FrameProcessor::GapFillPlugin::CONFIG_CHIP_SIZE + "[]", 2));
    BOOST_REQUIRE_NO_THROW(bad_cfg_1.set_param(FrameProcessor::GapFillPlugin::CONFIG_CHIP_SIZE + "[]", 2));
    BOOST_REQUIRE_NO_THROW(bad_cfg_1.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_X_GAPS + "[]", 1));
    BOOST_REQUIRE_NO_THROW(bad_cfg_1.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_X_GAPS + "[]", 2));
    BOOST_REQUIRE_NO_THROW(bad_cfg_1.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_X_GAPS + "[]", 3));
    BOOST_REQUIRE_NO_THROW(bad_cfg_1.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_X_GAPS + "[]", 2));
    BOOST_REQUIRE_NO_THROW(bad_cfg_1.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_X_GAPS + "[]", 1));
    BOOST_REQUIRE_NO_THROW(bad_cfg_1.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_Y_GAPS + "[]", 1));
    BOOST_REQUIRE_NO_THROW(bad_cfg_1.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_Y_GAPS + "[]", 2));
    BOOST_REQUIRE_NO_THROW(bad_cfg_1.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_Y_GAPS + "[]", 2));
    BOOST_REQUIRE_NO_THROW(bad_cfg_1.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_Y_GAPS + "[]", 1));

    gap_fill_plugin.configure(bad_cfg_1, reply_bad_cfg_1);

    // The following call should produce no return frame
    BOOST_CHECK(!gap_fill_plugin.configuration_valid(frame));

    // Test an inconsistent gap array compared with the grid config
    OdinData::IpcMessage reply_bad_cfg_2;
    OdinData::IpcMessage bad_cfg_2;
    BOOST_REQUIRE_NO_THROW(bad_cfg_2.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_SIZE + "[]", 3));
    BOOST_REQUIRE_NO_THROW(bad_cfg_2.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_SIZE + "[]", 4));
    BOOST_REQUIRE_NO_THROW(bad_cfg_2.set_param(FrameProcessor::GapFillPlugin::CONFIG_CHIP_SIZE + "[]", 1));
    BOOST_REQUIRE_NO_THROW(bad_cfg_2.set_param(FrameProcessor::GapFillPlugin::CONFIG_CHIP_SIZE + "[]", 1));
    BOOST_REQUIRE_NO_THROW(bad_cfg_2.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_X_GAPS + "[]", 1));
    BOOST_REQUIRE_NO_THROW(bad_cfg_2.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_X_GAPS + "[]", 2));
    BOOST_REQUIRE_NO_THROW(bad_cfg_2.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_X_GAPS + "[]", 1));
    BOOST_REQUIRE_NO_THROW(bad_cfg_2.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_Y_GAPS + "[]", 1));

    gap_fill_plugin.configure(bad_cfg_2, reply_bad_cfg_2);

    // The following call should produce no return frame
    BOOST_CHECK(!gap_fill_plugin.configuration_valid(frame));

    // Setup the plugin ready to insert gaps in both x and y direction
    OdinData::IpcMessage reply;
    OdinData::IpcMessage cfg;
    BOOST_REQUIRE_NO_THROW(cfg.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_SIZE + "[]", 3));
    BOOST_REQUIRE_NO_THROW(cfg.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_SIZE + "[]", 4));
    BOOST_REQUIRE_NO_THROW(cfg.set_param(FrameProcessor::GapFillPlugin::CONFIG_CHIP_SIZE + "[]", 1));
    BOOST_REQUIRE_NO_THROW(cfg.set_param(FrameProcessor::GapFillPlugin::CONFIG_CHIP_SIZE + "[]", 1));
    BOOST_REQUIRE_NO_THROW(cfg.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_X_GAPS + "[]", 1));
    BOOST_REQUIRE_NO_THROW(cfg.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_X_GAPS + "[]", 2));
    BOOST_REQUIRE_NO_THROW(cfg.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_X_GAPS + "[]", 3));
    BOOST_REQUIRE_NO_THROW(cfg.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_X_GAPS + "[]", 2));
    BOOST_REQUIRE_NO_THROW(cfg.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_X_GAPS + "[]", 1));
    BOOST_REQUIRE_NO_THROW(cfg.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_Y_GAPS + "[]", 1));
    BOOST_REQUIRE_NO_THROW(cfg.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_Y_GAPS + "[]", 2));
    BOOST_REQUIRE_NO_THROW(cfg.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_Y_GAPS + "[]", 2));
    BOOST_REQUIRE_NO_THROW(cfg.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_Y_GAPS + "[]", 1));

    gap_fill_plugin.configure(cfg, reply);

    // Push the frame through the plugin to force the gap fill
    boost::shared_ptr<FrameProcessor::IFrame> gap_frame = gap_fill_plugin.insert_gaps(frame);

    unsigned short gap_img[117] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 1, 0, 0, 2, 0, 0, 0, 3, 0, 0, 4, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 5, 0, 0, 6, 0, 0, 0, 7, 0, 0, 8, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 9, 0, 0, 10, 0, 0, 0, 11, 0, 0, 12, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    // Verify the resultant frame has the correct dimensions and the gaps have been inserted correctly
    unsigned short *ptr;
    BOOST_REQUIRE_NO_THROW(ptr = (unsigned short *)gap_frame->get_data_ptr());
    BOOST_CHECK_EQUAL(gap_frame->get_meta_data().get_dimensions()[0], 9);
    BOOST_CHECK_EQUAL(gap_frame->get_meta_data().get_dimensions()[1], 13);
    int index = 0;
    for (int y = 0; y < gap_frame->get_meta_data().get_dimensions()[0]; y++){
        for (int x = 0; x < gap_frame->get_meta_data().get_dimensions()[1]; x++) {
            BOOST_CHECK_EQUAL(ptr[index], gap_img[index]);
            index++;
        }
    }

    // Now test with a chip size of 2x2 and gaps of 2 in both directions
    OdinData::IpcMessage reply_2;
    OdinData::IpcMessage cfg_2;
    BOOST_REQUIRE_NO_THROW(cfg_2.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_SIZE + "[]", 2));
    BOOST_REQUIRE_NO_THROW(cfg_2.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_SIZE + "[]", 2));
    BOOST_REQUIRE_NO_THROW(cfg_2.set_param(FrameProcessor::GapFillPlugin::CONFIG_CHIP_SIZE + "[]", 2));
    BOOST_REQUIRE_NO_THROW(cfg_2.set_param(FrameProcessor::GapFillPlugin::CONFIG_CHIP_SIZE + "[]", 2));
    BOOST_REQUIRE_NO_THROW(cfg_2.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_X_GAPS + "[]", 1));
    BOOST_REQUIRE_NO_THROW(cfg_2.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_X_GAPS + "[]", 1));
    BOOST_REQUIRE_NO_THROW(cfg_2.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_X_GAPS + "[]", 1));
    BOOST_REQUIRE_NO_THROW(cfg_2.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_Y_GAPS + "[]", 1));
    BOOST_REQUIRE_NO_THROW(cfg_2.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_Y_GAPS + "[]", 1));
    BOOST_REQUIRE_NO_THROW(cfg_2.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_Y_GAPS + "[]", 1));

    gap_fill_plugin.configure(cfg_2, reply_2);

    // Push the frame through the plugin to force the gap fill
    boost::shared_ptr<FrameProcessor::IFrame> gap_frame_2 = gap_fill_plugin.insert_gaps(frame_2);

    unsigned short gap_img_2[49] = {
            0, 0, 0, 0, 0, 0, 0,
            0, 1, 1, 0, 2, 2, 0,
            0, 1, 1, 0, 2, 2, 0,
            0, 0, 0, 0, 0, 0, 0,
            0, 3, 3, 0, 4, 4, 0,
            0, 3, 3, 0, 4, 4, 0,
            0, 0, 0, 0, 0, 0, 0
    };


    // Verify the resultant frame has the correct dimensions and the gaps have been inserted correctly
    BOOST_REQUIRE_NO_THROW(ptr = (unsigned short *)gap_frame_2->get_data_ptr());
    BOOST_CHECK_EQUAL(gap_frame_2->get_meta_data().get_dimensions()[0], 7);
    BOOST_CHECK_EQUAL(gap_frame_2->get_meta_data().get_dimensions()[1], 7);
    index = 0;
    for (int y = 0; y < gap_frame_2->get_meta_data().get_dimensions()[0]; y++){
        for (int x = 0; x < gap_frame_2->get_meta_data().get_dimensions()[1]; x++) {
            BOOST_CHECK_EQUAL(ptr[index], gap_img_2[index]);
            index++;
        }
    }

};

BOOST_AUTO_TEST_SUITE_END(); //GapFillPluginUnitTest

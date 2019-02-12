/*
 * GapFillPluginTest.cpp
 *
 */

#include <boost/test/unit_test.hpp>
#include <DebugLevelLogger.h>
#include "FrameProcessorDefinitions.h"
#include "GapFillPlugin.h"
#include "IpcMessage.h"

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

        frame = boost::shared_ptr<FrameProcessor::Frame>(new FrameProcessor::Frame("data"));
        frame->set_frame_number(7);
        frame->set_dimensions(img_dims);
        frame->copy_data(static_cast<void *>(img), 24);
        frame->set_data_type(1); // 0: UINT8, 1: UINT16, 2: UINT32
        frame->set_acquisition_id("scan1");

    }

    ~GapFillPluginTestFixture() {}

    boost::shared_ptr <FrameProcessor::Frame> frame;
    FrameProcessor::GapFillPlugin gap_fill_plugin;
};

BOOST_FIXTURE_TEST_SUITE(GapFillPluginUnitTest, GapFillPluginTestFixture
);

BOOST_AUTO_TEST_CASE( GapFillPlugin_process_frame )
        {
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
        boost::shared_ptr<FrameProcessor::Frame> gap_frame = gap_fill_plugin.insert_gaps(frame);

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
        BOOST_REQUIRE_NO_THROW(ptr = (unsigned short *)gap_frame->get_data());
        BOOST_CHECK_EQUAL(gap_frame->get_dimensions()[0], 9);
        BOOST_CHECK_EQUAL(gap_frame->get_dimensions()[1], 13);
        int index = 0;
        for (int y = 0; y < gap_frame->get_dimensions()[0]; y++){
            for (int x = 0; x < gap_frame->get_dimensions()[1]; x++) {
                BOOST_CHECK_EQUAL(ptr[index], gap_img[index]);
                index++;
            }
        }
        };

BOOST_AUTO_TEST_SUITE_END(); //GapFillPluginUnitTest

/*
 * RawFileWriterPluginTest.cpp
 *
 *  Created on: 23 Feb 2026
 *      Author: Famous Alele
 */

#define BOOST_TEST_MODULE "RawFileWritePluginTests"
#define BOOST_TEST_MAIN

#include "RawFileWriterPlugin.h"
#include "Fixtures.h"

BOOST_GLOBAL_FIXTURE(GlobalConfig);

class RawFileWriterPluginTestFixture {
public:
    RawFileWriterPluginTestFixture() :
        img { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 },
        frame { boost::make_shared<FrameProcessor::DataBlockFrame>(
            FrameProcessor::FrameMetaData { 7, "data", FrameProcessor::raw_16bit, "scan1" /*Acq_ID*/, { 3, 4 } },
            static_cast<void*>(img),
            24
        ) }
    {
        set_debug_level(3);
        rfw_plugin.set_name("rfwplugin");
    }

    static bool is_not_critical(const std::runtime_error& er)
    {
        return true;
    }

    unsigned short img[12];
    boost::shared_ptr<FrameProcessor::Frame> frame;
    FrameProcessor::RawFileWriterPlugin rfw_plugin;
};

BOOST_FIXTURE_TEST_SUITE(RawFileWriterPluginUnitTest, RawFileWriterPluginTestFixture);

BOOST_AUTO_TEST_CASE(RawFileWriterPlugin_invalid_path)
{
    using RFWP = RawFileWriterPluginTestFixture;
    OdinData::IpcMessage reply_bad_;
    OdinData::IpcMessage bad_cfg_1;
    BOOST_CHECK_NO_THROW(bad_cfg_1.set_param(FrameProcessor::RawFileWriterPlugin::CONFIG_ENABLED, true));
    BOOST_CHECK_NO_THROW(
        bad_cfg_1.set_param(FrameProcessor::RawFileWriterPlugin::CONFIG_FILE_PATH, std::string("/sys/dummy_dir/"))
    );
    BOOST_REQUIRE_EXCEPTION(rfw_plugin.configure(bad_cfg_1, reply_bad_), std::runtime_error, RFWP::is_not_critical);
    BOOST_REQUIRE_NO_THROW(rfw_plugin.requestConfiguration(reply_bad_));
    BOOST_CHECK_EQUAL(
        reply_bad_.get_param<bool>(rfw_plugin.get_name() + '/' + FrameProcessor::RawFileWriterPlugin::CONFIG_ENABLED),
        false
    );
}

BOOST_AUTO_TEST_CASE(RawFileWriterPlugin_existing_path)
{
    using RFWP = RawFileWriterPluginTestFixture;
    OdinData::IpcMessage reply_bad_;
    OdinData::IpcMessage bad_cfg_2;
    BOOST_CHECK_NO_THROW(bad_cfg_2.set_param(FrameProcessor::RawFileWriterPlugin::CONFIG_ENABLED, true));
    BOOST_CHECK_NO_THROW(bad_cfg_2.set_param(FrameProcessor::RawFileWriterPlugin::CONFIG_FILE_PATH, std::string("/")));
    BOOST_REQUIRE_EXCEPTION(rfw_plugin.configure(bad_cfg_2, reply_bad_), std::runtime_error, RFWP::is_not_critical);
    BOOST_REQUIRE_NO_THROW(rfw_plugin.requestConfiguration(reply_bad_));
    BOOST_CHECK_EQUAL(
        reply_bad_.get_param<bool>(rfw_plugin.get_name() + '/' + FrameProcessor::RawFileWriterPlugin::CONFIG_ENABLED),
        false
    );
}

BOOST_AUTO_TEST_CASE(RawFileWriterPlugin_with_acq_id)
{
    OdinData::IpcMessage reply_;
    OdinData::IpcMessage cfg_;
    BOOST_CHECK_NO_THROW(cfg_.set_param(FrameProcessor::RawFileWriterPlugin::CONFIG_ENABLED, true));
    BOOST_CHECK_NO_THROW(
        cfg_.set_param(FrameProcessor::RawFileWriterPlugin::CONFIG_FILE_PATH, std::string("/opt/RFW_valid_dir"))
    );
    BOOST_REQUIRE_NO_THROW(rfw_plugin.configure(cfg_, reply_));
    BOOST_REQUIRE_NO_THROW(frame->meta_data().set_acquisition_ID("scan1")); // VALID acquisition ID
    BOOST_REQUIRE_NO_THROW(rfw_plugin.process_frame(frame));

    BOOST_REQUIRE_NO_THROW(rfw_plugin.status(reply_));
    BOOST_CHECK_EQUAL(
        reply_.get_param<size_t>(
            rfw_plugin.get_name() + '/' + FrameProcessor::RawFileWriterPlugin::STATUS_DROPPED_FRAMES
        ),
        0
    );
    BOOST_REQUIRE_NO_THROW(boost::filesystem::remove_all("/opt/RFW_valid_dir"));
}

BOOST_AUTO_TEST_CASE(RawFileWriterPlugin_no_acq_id)
{
    OdinData::IpcMessage reply_;
    OdinData::IpcMessage cfg_;
    BOOST_CHECK_NO_THROW(cfg_.set_param(FrameProcessor::RawFileWriterPlugin::CONFIG_ENABLED, true));
    BOOST_CHECK_NO_THROW(
        cfg_.set_param(FrameProcessor::RawFileWriterPlugin::CONFIG_FILE_PATH, std::string("/opt/RFW_valid_dir_2"))
    );
    BOOST_REQUIRE_NO_THROW(rfw_plugin.configure(cfg_, reply_));
    BOOST_REQUIRE_NO_THROW(frame->meta_data().set_acquisition_ID("")); // VALID acquisition ID!
    BOOST_REQUIRE_NO_THROW(rfw_plugin.process_frame(frame));

    BOOST_REQUIRE_NO_THROW(rfw_plugin.status(reply_));
    BOOST_CHECK_EQUAL(
        reply_.get_param<size_t>(
            rfw_plugin.get_name() + '/' + FrameProcessor::RawFileWriterPlugin::STATUS_DROPPED_FRAMES
        ),
        0
    );
    BOOST_REQUIRE_NO_THROW(boost::filesystem::remove_all("/opt/RFW_valid_dir_2"));
}

BOOST_AUTO_TEST_CASE(RawFileWriterPlugin_dropped_frames)
{
    OdinData::IpcMessage reply_;
    OdinData::IpcMessage cfg_1; // path configuration to be set once!
    OdinData::IpcMessage cfg_2; // 'enabled' state configuration, to be set all the time!

    BOOST_CHECK_NO_THROW(
        cfg_1.set_param(FrameProcessor::RawFileWriterPlugin::CONFIG_FILE_PATH, std::string("/opt/RFW_testdir"))
    );
    BOOST_CHECK_NO_THROW(cfg_2.set_param(FrameProcessor::RawFileWriterPlugin::CONFIG_ENABLED, true));
    BOOST_REQUIRE_NO_THROW(rfw_plugin.configure(cfg_1, reply_));
    BOOST_REQUIRE_NO_THROW(frame->meta_data().set_acquisition_ID("dummy1/dummy2")
    ); // mkdir will NOT create this directory and process_frame will increment the dropped_frames_ counter
    constexpr int process_times = 4;
    for (int i = 0; i < process_times; ++i) {
        BOOST_REQUIRE_NO_THROW(rfw_plugin.configure(cfg_2, reply_)
        ); // reset the plugin's parameter to be ENABLED before processing the frame
        BOOST_REQUIRE_NO_THROW(rfw_plugin.process_frame(frame));
    }

    BOOST_REQUIRE_NO_THROW(rfw_plugin.status(reply_));
    BOOST_CHECK_EQUAL(
        reply_.get_param<size_t>(
            rfw_plugin.get_name() + '/' + FrameProcessor::RawFileWriterPlugin::STATUS_DROPPED_FRAMES
        ),
        process_times
    );
    BOOST_REQUIRE_NO_THROW(boost::filesystem::remove_all("/opt/RFW_testdir"));
}

BOOST_AUTO_TEST_SUITE_END();

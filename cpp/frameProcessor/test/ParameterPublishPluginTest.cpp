/*
 * ParameterPublishPluginTest.cpp
 *
 *  Created on: 25 Mar 2026
 *      Author: Famous Alele
 */

#define BOOST_TEST_MODULE "ParameterPublishPluginTests"
#define BOOST_TEST_MAIN

#include "ParameterPublishPlugin.h"
#include "Fixtures.h"

BOOST_GLOBAL_FIXTURE(GlobalConfig);

class ParameterPublishPluginTestFixture {
public:
    ParameterPublishPluginTestFixture() :
        img { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 },
        frame { boost::make_shared<FrameProcessor::DataBlockFrame>(
            FrameProcessor::FrameMetaData { 7, "data", FrameProcessor::raw_16bit, "scan1" /*Acq_ID*/, { 3, 4 } },
            static_cast<void*>(img),
            24
        ) }
    {
        /**
         * create dummy frame with nominal value data.
         * Add some parameter values to be published
         */
        FrameProcessor::FrameMetaData frame_meta(
            1, "raw", FrameProcessor::raw_16bit, "test", { 3, 4 }, FrameProcessor::no_compression
        );
        frame_meta.set_parameter<uint64_t>("low2", 1);
        frame_meta.set_parameter<uint64_t>("low1", 4);
        frame_meta.set_parameter<uint64_t>("high1", 2);
        frame_meta.set_parameter<uint64_t>("high2", 3);
        frame = boost::make_shared<FrameProcessor::DataBlockFrame>(frame_meta, static_cast<void*>(img), 24);
        set_debug_level(3);
        plugin.set_name("paramPubPlugin");
    }
    unsigned short img[12];
    boost::shared_ptr<FrameProcessor::Frame> frame;
    FrameProcessor::ParameterPublishPlugin plugin;
};

BOOST_FIXTURE_TEST_SUITE(ParameterPublishPluginUnitTest, ParameterPublishPluginTestFixture);

BOOST_AUTO_TEST_CASE(ParameterPublishPlugin_Publish)
{

    // Configure thresholds
    OdinData::IpcMessage reply;
    OdinData::IpcMessage cfg;
    BOOST_CHECK_NO_THROW(
        cfg.set_param(FrameProcessor::ParameterPublishPlugin::CONFIG_ADD_PARAMETER, std::string("low1"))
    );
    BOOST_CHECK_NO_THROW(
        cfg.set_param(FrameProcessor::ParameterPublishPlugin::CONFIG_ADD_PARAMETER, std::string("low2"))
    );
    BOOST_CHECK_NO_THROW(
        cfg.set_param(FrameProcessor::ParameterPublishPlugin::CONFIG_ADD_PARAMETER, std::string("high1"))
    );
    BOOST_CHECK_NO_THROW(
        cfg.set_param(FrameProcessor::ParameterPublishPlugin::CONFIG_ENDPOINT, std::string("tcp://*:10000"))
    );
    BOOST_CHECK_NO_THROW(
        cfg.set_param(FrameProcessor::ParameterPublishPlugin::CONFIG_ADD_PARAMETER, std::string("high2"))
    );
    BOOST_CHECK_NO_THROW(plugin.configure(cfg, reply));
    usleep(200000); // Give the listener a chance to connect
    plugin.process_frame(frame);
}

BOOST_AUTO_TEST_CASE(ParameterPublishPlugin_endpoint_change)
{
    OdinData::IpcMessage reply;
    OdinData::IpcMessage cfg;
    using FPPP = FrameProcessor::ParameterPublishPlugin;
    BOOST_CHECK_NO_THROW(cfg.set_param(FPPP::CONFIG_ENDPOINT, std::string("tcp://*:17000")));
    BOOST_CHECK_NO_THROW(plugin.configure(cfg, reply));
    BOOST_REQUIRE_NO_THROW(plugin.requestConfiguration(reply));
    BOOST_CHECK_EQUAL(
        cfg.get_param<std::string>(FPPP::CONFIG_ENDPOINT),
        reply.get_param<std::string>(plugin.get_name() + '/' + FPPP::CONFIG_ENDPOINT)
    );
    BOOST_CHECK_NO_THROW(cfg.set_param(FPPP::CONFIG_ENDPOINT, std::string("tcp://*:12340")));
    BOOST_CHECK_NO_THROW(plugin.configure(cfg, reply));
    BOOST_REQUIRE_NO_THROW(plugin.requestConfiguration(reply));
    BOOST_CHECK_EQUAL(
        cfg.get_param<std::string>(FPPP::CONFIG_ENDPOINT),
        reply.get_param<std::string>(plugin.get_name() + '/' + FPPP::CONFIG_ENDPOINT)
    );
}

BOOST_AUTO_TEST_SUITE_END(); // ParameterPublishPluginTest

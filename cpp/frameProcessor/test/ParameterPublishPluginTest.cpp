/*
 * ParameterPublishPluginTest.cpp
 *
 *  Created on: 25 Mar 2026
 *      Author: Famous Alele
 */

#define BOOST_TEST_MODULE "RawFileWritePluginTests"
#define BOOST_TEST_MAIN

#include "ParameterPublishPlugin.h"
#include "Fixtures.h"

BOOST_AUTO_TEST_SUITE(ParameterPublishPluginTest);

BOOST_AUTO_TEST_CASE(Publish)
{
    FrameProcessor::ParameterPublishPlugin plugin;
    // Configure thresholds
    OdinData::IpcMessage reply;
    OdinData::IpcMessage cfg;
    cfg.set_param(FrameProcessor::ParameterPublishPlugin::CONFIG_ADD_PARAMETER, std::string("low1"));
    plugin.configure(cfg, reply);
    cfg.set_param(FrameProcessor::ParameterPublishPlugin::CONFIG_ADD_PARAMETER, std::string("low2"));
    plugin.configure(cfg, reply);
    cfg.set_param(FrameProcessor::ParameterPublishPlugin::CONFIG_ADD_PARAMETER, std::string("high1"));
    plugin.configure(cfg, reply);
    cfg.set_param(std::string("endpoint"), std::string("tcp://*:10000"));
    cfg.set_param(FrameProcessor::ParameterPublishPlugin::CONFIG_ADD_PARAMETER, std::string("high2"));
    plugin.configure(cfg, reply);
    usleep(200000); // Give the listener a chance to connect
    uint16_t img[12] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };

    dimensions_t img_dims(2);
    img_dims[0] = 3;
    img_dims[1] = 4;
    FrameProcessor::FrameMetaData frame_meta(
        1, "raw", FrameProcessor::raw_16bit, "test", img_dims, FrameProcessor::no_compression
    );
    frame_meta.set_parameter<uint64_t>("low2", 1);
    frame_meta.set_parameter<uint64_t>("low1", 4);
    frame_meta.set_parameter<uint64_t>("high1", 2);
    frame_meta.set_parameter<uint64_t>("high2", 3);
    boost::shared_ptr<FrameProcessor::DataBlockFrame> frame(
        new FrameProcessor::DataBlockFrame(frame_meta, static_cast<void*>(img), 24)
    );
    plugin.process_frame(frame);
}
BOOST_AUTO_TEST_SUITE_END(); // ParameterPublishPluginTest

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
        parameters { { { 1, "low2" }, { 4, "low1" }, { 2, "high1" }, { 3, "high2" } } },
        img { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 }
    {
        /**
         * create dummy frame with nominal value data.
         * Add some parameter values to be published
         */
        FrameProcessor::FrameMetaData frame_meta(
            1, "raw", FrameProcessor::raw_64bit, "test", { 3, 4 }, FrameProcessor::no_compression
        );
        for (size_t i = 0; i < parameters.size(); ++i) {
            frame_meta.set_parameter<uint64_t>(parameters[i].param, parameters[i].val);
        }
        frame = boost::make_shared<FrameProcessor::DataBlockFrame>(frame_meta, static_cast<void*>(img), 24);
        set_debug_level(3);
        plugin.set_name("paramPubPlugin");
    }

    //
    struct param_pair {
        const uint64_t val;
        const char* param;
    };
    const std::array<param_pair, 4> parameters;
    unsigned short img[12];
    boost::shared_ptr<FrameProcessor::Frame> frame;
    FrameProcessor::ParameterPublishPlugin plugin;
};

BOOST_FIXTURE_TEST_SUITE(ParameterPublishPluginUnitTest, ParameterPublishPluginTestFixture);

BOOST_AUTO_TEST_CASE(ParameterPublishPlugin_Publish)
{
    using FPPP = FrameProcessor::ParameterPublishPlugin;
    using PPPT = ParameterPublishPluginTestFixture;
    // Configure thresholds
    OdinData::IpcMessage reply;
    OdinData::IpcMessage cfg;
    OdinData::IpcChannel listen_ch(ZMQ_SUB);
    constexpr const char* inproc_endpoint = "inproc://testcase1";
    listen_ch.subscribe("");
    listen_ch.connect(inproc_endpoint);
    for (size_t i = 0; i < parameters.size(); ++i) {
        BOOST_CHECK_NO_THROW(cfg.set_param(FPPP::CONFIG_ADD_PARAMETER, std::string(PPPT::parameters[i].param)));
        BOOST_CHECK_NO_THROW(plugin.configure(cfg, reply));
    }
    BOOST_CHECK_NO_THROW(cfg.set_param(FPPP::CONFIG_ENDPOINT, std::string(inproc_endpoint)));
    BOOST_REQUIRE_NO_THROW(plugin.configure(cfg, reply));
    BOOST_REQUIRE_NO_THROW(plugin.process_frame(frame));

    rapidjson::Document d;
    BOOST_REQUIRE_NO_THROW(d.Parse(listen_ch.recv().c_str()));
    BOOST_CHECK(d.HasMember(FPPP::DATA_PARAMETERS.c_str()) && d[FPPP::DATA_PARAMETERS.c_str()].IsString());
    rapidjson::Document nested_params;
    BOOST_REQUIRE_NO_THROW(nested_params.Parse(d[FPPP::DATA_PARAMETERS.c_str()].GetString()));
    for (size_t i = 0; i < parameters.size(); ++i) {
        BOOST_CHECK(
            nested_params.HasMember(PPPT::parameters[i].param) && nested_params[PPPT::parameters[i].param].IsInt()
        );
        BOOST_CHECK_EQUAL(nested_params[PPPT::parameters[i].param].GetInt(), PPPT::parameters[i].val);
    }
}

BOOST_AUTO_TEST_CASE(ParameterPublishPlugin_endpointConfigure)
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

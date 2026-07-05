#define BOOST_TEST_MODULE "FrameProcessorPluginConfigureTests"
#define BOOST_TEST_MAIN

#include <rapidjson/document.h>

#include "Fixtures.h"

#include "BloscPlugin.h"
#include "DummyUDPProcessPlugin.h"
#include "FrameProcessorController.h"
#include "GapFillPlugin.h"
#include "ParameterAdjustmentPlugin.h"
#include "ParameterPublishPlugin.h"

BOOST_GLOBAL_FIXTURE(GlobalConfig);

BOOST_AUTO_TEST_CASE(BloscPluginConfigureUpdatesRequestedSettings)
{
    FrameProcessor::BloscPlugin plugin;
    plugin.set_name("blosc");

    OdinData::IpcMessage reply;
    OdinData::IpcMessage cfg;
    cfg.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_MODE, std::string(FrameProcessor::BloscPlugin::BLOSC_COMPRESS_MODE_STR));
    cfg.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_LEVEL, 7);
    cfg.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_SHUFFLE, std::string(FrameProcessor::BloscPlugin::BLOSC_SHUFFLE_STR));
    cfg.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_THREADS, 4u);
    cfg.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_COMPRESSOR, std::string("zlib"));

    plugin.configure(cfg, reply);
    plugin.requestConfiguration(reply);

    BOOST_CHECK_EQUAL(
        reply.get_param<std::string>(plugin.get_name() + "/" + FrameProcessor::BloscPlugin::CONFIG_BLOSC_MODE),
        std::string(FrameProcessor::BloscPlugin::BLOSC_COMPRESS_MODE_STR)
    );
    BOOST_CHECK_EQUAL(reply.get_param<int>(plugin.get_name() + "/" + FrameProcessor::BloscPlugin::CONFIG_BLOSC_LEVEL), 7);
    BOOST_CHECK_EQUAL(
        reply.get_param<std::string>(plugin.get_name() + "/" + FrameProcessor::BloscPlugin::CONFIG_BLOSC_SHUFFLE),
        std::string(FrameProcessor::BloscPlugin::BLOSC_SHUFFLE_STR)
    );
    BOOST_CHECK_EQUAL(reply.get_param<unsigned int>(plugin.get_name() + "/" + FrameProcessor::BloscPlugin::CONFIG_BLOSC_THREADS), 4u);
    BOOST_CHECK_EQUAL(
        reply.get_param<std::string>(plugin.get_name() + "/" + FrameProcessor::BloscPlugin::CONFIG_BLOSC_COMPRESSOR),
        std::string("zlib")
    );
}

BOOST_AUTO_TEST_CASE(DummyUDPProcessPluginConfigureUpdatesConfiguration)
{
    FrameProcessor::DummyUDPProcessPlugin plugin;
    plugin.set_name("dummy");

    OdinData::IpcMessage reply;
    OdinData::IpcMessage cfg;
    cfg.set_param(FrameProcessor::DummyUDPProcessPlugin::CONFIG_IMAGE_WIDTH, 128);
    cfg.set_param(FrameProcessor::DummyUDPProcessPlugin::CONFIG_IMAGE_HEIGHT, 64);
    cfg.set_param(FrameProcessor::DummyUDPProcessPlugin::CONFIG_COPY_FRAME, true);

    plugin.configure(cfg, reply);
    plugin.requestConfiguration(reply);

    BOOST_CHECK_EQUAL(reply.get_param<int>(plugin.get_name() + "/" + FrameProcessor::DummyUDPProcessPlugin::CONFIG_IMAGE_WIDTH), 128);
    BOOST_CHECK_EQUAL(reply.get_param<int>(plugin.get_name() + "/" + FrameProcessor::DummyUDPProcessPlugin::CONFIG_IMAGE_HEIGHT), 64);
    BOOST_CHECK_EQUAL(reply.get_param<bool>(plugin.get_name() + "/" + FrameProcessor::DummyUDPProcessPlugin::CONFIG_COPY_FRAME), true);
}

BOOST_AUTO_TEST_CASE(GapFillPluginConfigureEnablesValidConfiguration)
{
    FrameProcessor::GapFillPlugin plugin;
    plugin.set_name("gapfill");

    rapidjson::Document grid;
    grid.SetArray();
    grid.PushBack(rapidjson::Value(2), grid.GetAllocator());
    grid.PushBack(rapidjson::Value(2), grid.GetAllocator());

    rapidjson::Document chip;
    chip.SetArray();
    chip.PushBack(rapidjson::Value(2), chip.GetAllocator());
    chip.PushBack(rapidjson::Value(2), chip.GetAllocator());

    rapidjson::Document gaps_x;
    gaps_x.SetArray();
    gaps_x.PushBack(rapidjson::Value(1), gaps_x.GetAllocator());
    gaps_x.PushBack(rapidjson::Value(2), gaps_x.GetAllocator());
    gaps_x.PushBack(rapidjson::Value(1), gaps_x.GetAllocator());

    rapidjson::Document gaps_y;
    gaps_y.SetArray();
    gaps_y.PushBack(rapidjson::Value(1), gaps_y.GetAllocator());
    gaps_y.PushBack(rapidjson::Value(2), gaps_y.GetAllocator());
    gaps_y.PushBack(rapidjson::Value(1), gaps_y.GetAllocator());

    OdinData::IpcMessage reply;
    OdinData::IpcMessage cfg;
    cfg.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_SIZE, grid);
    cfg.set_param(FrameProcessor::GapFillPlugin::CONFIG_CHIP_SIZE, chip);
    cfg.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_X_GAPS, gaps_x);
    cfg.set_param(FrameProcessor::GapFillPlugin::CONFIG_GRID_Y_GAPS, gaps_y);

    plugin.configure(cfg, reply);

    dimensions_t dims(2);
    dims[0] = 4;
    dims[1] = 4;
    FrameProcessor::FrameMetaData frame_meta(1, "data", FrameProcessor::raw_16bit, "test", dims, FrameProcessor::no_compression);
    char dummy_data[16] = { 0 };
    boost::shared_ptr<FrameProcessor::DataBlockFrame> frame(
        new FrameProcessor::DataBlockFrame(frame_meta, static_cast<void*>(dummy_data), sizeof(dummy_data))
    );

    BOOST_CHECK(plugin.configuration_valid(frame));
}

BOOST_AUTO_TEST_CASE(ParameterAdjustmentPluginConfigureAddsConfiguredParameter)
{
    FrameProcessor::ParameterAdjustmentPlugin plugin;
    plugin.set_name("param_adjust");

    rapidjson::Document params;
    params.SetObject();
    rapidjson::Value param_name(rapidjson::kStringType);
    param_name.SetString("energy", params.GetAllocator());
    rapidjson::Value param_cfg(rapidjson::kObjectType);
    rapidjson::Value adjustment(2);
    rapidjson::Value input(rapidjson::kStringType);
    input.SetString("frame_number", params.GetAllocator());
    param_cfg.AddMember("adjustment", adjustment, params.GetAllocator());
    param_cfg.AddMember("input", input, params.GetAllocator());
    params.AddMember(param_name, param_cfg, params.GetAllocator());

    OdinData::IpcMessage reply;
    OdinData::IpcMessage cfg;
    cfg.set_param(FrameProcessor::PARAMETER_NAME_CONFIG, params);

    plugin.configure(cfg, reply);

    dimensions_t dims(2);
    dims[0] = 1;
    dims[1] = 1;
    FrameProcessor::FrameMetaData frame_meta(7, "data", FrameProcessor::raw_16bit, "test", dims, FrameProcessor::no_compression);
    char dummy_data[2] = { 0 };
    boost::shared_ptr<FrameProcessor::DataBlockFrame> frame(
        new FrameProcessor::DataBlockFrame(frame_meta, static_cast<void*>(dummy_data), sizeof(dummy_data))
    );
    frame->set_frame_number(3);

    plugin.process_frame(frame);

    BOOST_CHECK(frame->meta_data().has_parameter("energy"));
    BOOST_CHECK_EQUAL(frame->meta_data().get_parameter<uint64_t>("energy"), 5u);
}

BOOST_AUTO_TEST_CASE(ParameterPublishPluginConfigureStoresEndpointAndParameters)
{
    FrameProcessor::ParameterPublishPlugin plugin;
    plugin.set_name("param_publish");

    OdinData::IpcMessage reply;
    OdinData::IpcMessage cfg;
    rapidjson::Document params;
    params.SetArray();
    rapidjson::Value value(rapidjson::kStringType);
    value.SetString("temperature", params.GetAllocator());
    params.PushBack(value, params.GetAllocator());

    cfg.set_param(FrameProcessor::ParameterPublishPlugin::CONFIG_ENDPOINT, std::string("inproc://param-publish"));
    cfg.set_param(FrameProcessor::ParameterPublishPlugin::CONFIG_ADD_PARAMETER, params);

    plugin.configure(cfg, reply);
    plugin.requestConfiguration(reply);

    BOOST_CHECK_EQUAL(
        reply.get_param<std::string>(plugin.get_name() + "/" + FrameProcessor::ParameterPublishPlugin::CONFIG_ENDPOINT),
        std::string("inproc://param-publish")
    );
    BOOST_CHECK_EQUAL(reply.get_param<std::string>(plugin.get_name() + "/parameters[]"), std::string("temperature"));
}

BOOST_AUTO_TEST_CASE(FrameProcessorControllerConfigureRoutesPluginConfiguration)
{
    FrameProcessor::FrameProcessorController controller(1);
    OdinData::IpcMessage reply;
    OdinData::IpcMessage cfg;

    rapidjson::Document plugin_cfg;
    plugin_cfg.SetObject();
    plugin_cfg.AddMember("load", rapidjson::Value(rapidjson::kObjectType), plugin_cfg.GetAllocator());
    plugin_cfg.AddMember("connect", rapidjson::Value(rapidjson::kObjectType), plugin_cfg.GetAllocator());

    cfg.set_param("plugin", plugin_cfg);

    BOOST_CHECK_NO_THROW(controller.configure(cfg, reply));
}

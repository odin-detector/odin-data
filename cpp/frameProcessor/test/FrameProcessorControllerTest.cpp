#define BOOST_TEST_MODULE "FrameProcessorControllerTests"
#define BOOST_TEST_MAIN

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>

#include "DebugLevelLogger.h"
#include "FrameProcessorController.h"
#include "MetaMessagePublisher.h"
#include "logging.h"
#include "version.h"

namespace {

struct ControllerFixture {
    ControllerFixture() :
        original_debug_level(debug_level),
        original_app_path(OdinData::app_path)
    {
        OdinData::app_path = "unit_fp_FrameProcessorController";
        controller = boost::make_shared<FrameProcessor::FrameProcessorController>();
    }

    ~ControllerFixture()
    {
        controller->shutdown();
        controller.reset();
        set_debug_level(original_debug_level);
        OdinData::app_path = original_app_path;
    }

    DebugLevel original_debug_level;
    std::string original_app_path;
    boost::shared_ptr<FrameProcessor::FrameProcessorController> controller;
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(FrameProcessorControllerUnitTest, ControllerFixture)

BOOST_AUTO_TEST_CASE(DefaultConfigurationAndVersionAreReported)
{
    OdinData::IpcMessage config_reply;
    controller->requestConfiguration(config_reply, false);

    BOOST_CHECK_EQUAL(config_reply.get_param<std::string>("ctrl_endpoint"), "");
    BOOST_CHECK_EQUAL(config_reply.get_param<std::string>("meta_endpoint"), "");
    BOOST_CHECK_EQUAL(config_reply.get_param<std::string>("fr_setup/fr_ready_cnxn"), "tcp://127.0.0.1:5001");
    BOOST_CHECK_EQUAL(config_reply.get_param<std::string>("fr_setup/fr_release_cnxn"), "tcp://127.0.0.1:5002");
    BOOST_CHECK_EQUAL(config_reply.get_param<int64_t>("config_ts"), -1);

    OdinData::IpcMessage version_reply;
    controller->provideVersion(version_reply);
    BOOST_CHECK_EQUAL(version_reply.get_param<int>("version/odin-data/major"), ODIN_DATA_VERSION_MAJOR);
    BOOST_CHECK_EQUAL(version_reply.get_param<int>("version/odin-data/minor"), ODIN_DATA_VERSION_MINOR);
    BOOST_CHECK_EQUAL(version_reply.get_param<int>("version/odin-data/patch"), ODIN_DATA_VERSION_PATCH);
}

BOOST_AUTO_TEST_SUITE_END()

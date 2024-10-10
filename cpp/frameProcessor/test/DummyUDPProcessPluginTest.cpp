/*
 * DummyUDPProcessPluginTest.cpp
 *
 *  Created on: 25 Sep 2024
 *      Author: Alan Greer
 */

#include <boost/test/unit_test.hpp>
#include <DebugLevelLogger.h>
#include "FrameProcessorDefinitions.h"
#include "DummyUDPProcessPlugin.h"
#include "IpcMessage.h"

class DummyUDPProcessPluginTestFixture {
public:
    DummyUDPProcessPluginTestFixture() {
        set_debug_level(3);

        dummy_plugin.set_name("dummy");
    }

    ~DummyUDPProcessPluginTestFixture() {}

    FrameProcessor::DummyUDPProcessPlugin dummy_plugin;
};

BOOST_FIXTURE_TEST_SUITE(DummyUDPProcessPluginUnitTest, DummyUDPProcessPluginTestFixture);

BOOST_AUTO_TEST_CASE( DummyUDPProcessPlugin_commands )
{
    std::vector<std::string> commands_reply;
    OdinData::IpcMessage command_reply;

    // Request the command set of the DummyUDPProcessPlugin
    BOOST_REQUIRE_NO_THROW(commands_reply = dummy_plugin.requestCommands());

    // Verify the returned command set is as expected
    BOOST_CHECK_EQUAL(commands_reply[0], std::string("print"));

    // Verify that an incorrect commmand request is rejected
    BOOST_CHECK_THROW(dummy_plugin.execute("bad_command", command_reply), std::runtime_error);

    // Verify that a supported commmand request is accepted
    BOOST_REQUIRE_NO_THROW(dummy_plugin.execute("print", command_reply));
};

BOOST_AUTO_TEST_SUITE_END(); //DummyUDPProcessPluginUnitTest

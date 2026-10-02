/*
 * SharedMemoryPluginTest.cpp
 *
 *  Created on: 10 Sep 2026
 *      Author: Alan Greer
 */

#define BOOST_TEST_MODULE "SharedMemoryPluginTests"
#define BOOST_TEST_MAIN

#include "SharedMemoryPlugin.h"
#include "Fixtures.h"
#include "IFrameCallback.h"
#include "SharedBufferManager.h"
#include "version.h"

BOOST_GLOBAL_FIXTURE(GlobalConfig);

class SharedMemoryPluginTestFixture {
public:
    SharedMemoryPluginTestFixture()
    {
        set_debug_level(3);
        shm_plugin.set_name("shm");
    }

    FrameProcessor::SharedMemoryPlugin shm_plugin;
};

class SharedMemoryPluginTestCallback : public FrameProcessor::IFrameCallback {
public:
    SharedMemoryPluginTestCallback()
    {
    }

    void callback(boost::shared_ptr<FrameProcessor::Frame> frame)
    {
        frame_ = frame;
    }

    boost::shared_ptr<FrameProcessor::Frame> frame_;
};

BOOST_FIXTURE_TEST_SUITE(SharedMemoryPluginUnitTest, SharedMemoryPluginTestFixture);

BOOST_AUTO_TEST_CASE(SharedMemoryPlugin_version_and_status)
{
    using SHM = SharedMemoryPluginTestFixture;
    OdinData::IpcMessage status_reply;

    BOOST_CHECK_EQUAL(shm_plugin.get_version_major(), ODIN_DATA_VERSION_MAJOR);
    BOOST_CHECK_EQUAL(shm_plugin.get_version_minor(), ODIN_DATA_VERSION_MINOR);
    BOOST_CHECK_EQUAL(shm_plugin.get_version_patch(), ODIN_DATA_VERSION_PATCH);
    BOOST_CHECK_EQUAL(shm_plugin.get_version_short(), ODIN_DATA_VERSION_STR_SHORT);
    BOOST_CHECK_EQUAL(shm_plugin.get_version_long(), ODIN_DATA_VERSION_STR);

    BOOST_REQUIRE_NO_THROW(shm_plugin.status(status_reply));
    BOOST_CHECK_EQUAL(
        status_reply.has_param(shm_plugin.get_name() + '/' + FrameProcessor::SharedMemoryPlugin::STATUS_SHB_NAME), false
    );
    BOOST_CHECK_EQUAL(
        status_reply.has_param(shm_plugin.get_name() + '/' + FrameProcessor::SharedMemoryPlugin::STATUS_SHB_CONFIGURED),
        false
    );
}

BOOST_AUTO_TEST_CASE(SharedMemoryPlugin_requestconfiguration)
{
    using SHM = SharedMemoryPluginTestFixture;
    OdinData::IpcMessage config_reply;

    BOOST_REQUIRE_NO_THROW(shm_plugin.requestConfiguration(config_reply));
    BOOST_CHECK_EQUAL(
        config_reply.get_param<std::string>(
            shm_plugin.get_name() + '/' + FrameProcessor::SharedMemoryPlugin::CONFIG_FR_READY
        ),
        ""
    );
    BOOST_CHECK_EQUAL(
        config_reply.get_param<std::string>(
            shm_plugin.get_name() + '/' + FrameProcessor::SharedMemoryPlugin::CONFIG_FR_RELEASE
        ),
        ""
    );
}

BOOST_AUTO_TEST_CASE(SharedMemoryPlugin_full_shm_test)
{
    using SHM = SharedMemoryPluginTestFixture;
    boost::shared_ptr<SharedMemoryPluginTestCallback> cb = boost::make_shared<SharedMemoryPluginTestCallback>();
    // cb->start();
    OdinData::IpcMessage cfg;
    OdinData::IpcMessage cfg_reply;
    OdinData::IpcMessage status_reply;

    // Create a temporary shared buffer manager
    // This must drop out of scope to release the file handle but leave the file present in the filesystem
    {
        OdinData::SharedBufferManager sbm("test_buffer", 1000, 20, false);
    }

    // Verify that file exists
    struct stat buffer;
    BOOST_CHECK_EQUAL(stat("/dev/shm/test_buffer", &buffer), 0);

    // Register a callback with the shared memory plugin
    shm_plugin.register_callback("temp_index", cb, true);

    // Create the two IPC channels required for ready and release
    OdinData::IpcChannel ready(ZMQ_PUB);
    OdinData::IpcChannel release(ZMQ_SUB);

    ready.bind("inproc://ready");
    release.bind("inproc://release");
    release.subscribe("");

    // Configure the shared memory plugin to connect to these channels
    BOOST_CHECK_NO_THROW(
        cfg.set_param(FrameProcessor::SharedMemoryPlugin::CONFIG_FR_READY, std::string("inproc://ready"))
    );
    BOOST_CHECK_NO_THROW(
        cfg.set_param(FrameProcessor::SharedMemoryPlugin::CONFIG_FR_RELEASE, std::string("inproc://release"))
    );
    BOOST_REQUIRE_NO_THROW(shm_plugin.configure(cfg, cfg_reply));

    // Check that the FR_READY string has been set
    BOOST_REQUIRE_NO_THROW(shm_plugin.requestConfiguration(cfg_reply));
    BOOST_CHECK_EQUAL(
        cfg_reply.get_param<std::string>(
            shm_plugin.get_name() + '/' + FrameProcessor::SharedMemoryPlugin::CONFIG_FR_READY
        ),
        "inproc://ready"
    );
    BOOST_CHECK_EQUAL(
        cfg_reply.get_param<std::string>(
            shm_plugin.get_name() + '/' + FrameProcessor::SharedMemoryPlugin::CONFIG_FR_RELEASE
        ),
        "inproc://release"
    );

    // The release channel should get a message requesting the shared memory access filename
    OdinData::IpcMessage msg(release.recv().c_str(), true);
    BOOST_CHECK_EQUAL(msg.get_msg_type(), OdinData::IpcMessage::MsgTypeCmd);
    BOOST_CHECK_EQUAL(msg.get_msg_val(), OdinData::IpcMessage::MsgValCmdBufferConfigRequest);
    sleep(1);

    // Now send the response with the name of the temp shared memory buffer
    OdinData::IpcMessage buffer_reply_msg(
        OdinData::IpcMessage::MsgTypeNotify, OdinData::IpcMessage::MsgValNotifyBufferConfig
    );
    BOOST_CHECK_NO_THROW(buffer_reply_msg.set_param("shared_buffer_name", std::string("test_buffer")));
    BOOST_CHECK_NO_THROW(ready.send(buffer_reply_msg.encode()));

    // Here we need to wait to allow the message to be received
    sleep(1);

    // Verify the status now confirms the shared memory buffer is set up correctly
    BOOST_REQUIRE_NO_THROW(shm_plugin.status(status_reply));
    BOOST_CHECK_EQUAL(
        status_reply.get_param<std::string>(
            shm_plugin.get_name() + '/' + FrameProcessor::SharedMemoryPlugin::STATUS_SHB_NAME
        ),
        std::string("test_buffer")
    );
    BOOST_CHECK_EQUAL(
        status_reply.get_param<bool>(
            shm_plugin.get_name() + '/' + FrameProcessor::SharedMemoryPlugin::STATUS_SHB_CONFIGURED
        ),
        true
    );

    // Construct and send a message notifying that buffer 2 is ready
    OdinData::IpcMessage frame_ready_msg(
        OdinData::IpcMessage::MsgTypeNotify, OdinData::IpcMessage::MsgValNotifyFrameReady, true
    );
    frame_ready_msg.set_param<int>("frame", 1);
    frame_ready_msg.set_param<int>("buffer_id", 2);
    ready.send(frame_ready_msg.encode());

    // Here we need to wait to allow the message to be received
    sleep(1);

    // The plugin should receive the frame notification, create a frame push it to the registered callback
    BOOST_CHECK(cb->frame_);
    BOOST_CHECK_EQUAL(cb->frame_->get_frame_number(), 1);

    // Destroy the frame
    cb->frame_.reset();

    // Verify the release mechanism works for the frame
    OdinData::IpcMessage msg_rel(release.recv().c_str(), true);
    BOOST_CHECK_EQUAL(msg_rel.get_msg_type(), OdinData::IpcMessage::MsgTypeNotify);
    BOOST_CHECK_EQUAL(msg_rel.get_msg_val(), OdinData::IpcMessage::MsgValNotifyFrameRelease);
    BOOST_CHECK_EQUAL(msg_rel.get_param<int>("buffer_id"), 2);

    ready.unbind("inproc://ready");
    release.unbind("inproc://release");
}

BOOST_AUTO_TEST_SUITE_END();

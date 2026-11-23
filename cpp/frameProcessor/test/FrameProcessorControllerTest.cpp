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

class ControllerTestFrame : public FrameProcessor::Frame {
public:
    ControllerTestFrame() :
        Frame(
            FrameProcessor::FrameMetaData(
                1,
                "data",
                FrameProcessor::raw_8bit,
                "controller-test",
                dimensions_t(1, 1),
                FrameProcessor::no_compression
            ),
            sizeof(data_)
        ),
        data_(1)
    {
    }

    void* get_data_ptr() const override
    {
        return const_cast<uint8_t*>(&data_);
    }

private:
    uint8_t data_;
};

class ControllerTestPlugin : public FrameProcessor::FrameProcessorPlugin {
public:
    ControllerTestPlugin() :
        frames_(0),
        eoa_(0)
    {
        ++instances;
        created.push_back(this);
        set_error("test error");
        set_warning("test warning");
        add_config_param_metadata("value", PMDD::INT_T, PMDA::READ_WRITE);
        add_status_param_metadata("ready", PMDD::BOOL_T, PMDA::READ_ONLY);
    }

    static void reset()
    {
        instances = 0;
        configure_calls = 0;
        execute_calls = 0;
        request_configuration_calls = 0;
        status_calls = 0;
        reset_calls = 0;
        configured_value = 0;
        reset_result = true;
        last_command.clear();
        created.clear();
    }

    int get_version_major() override
    {
        return 1;
    }
    int get_version_minor() override
    {
        return 2;
    }
    int get_version_patch() override
    {
        return 3;
    }
    std::string get_version_short() override
    {
        return "1.2.3";
    }
    std::string get_version_long() override
    {
        return "controller-test-plugin 1.2.3";
    }

    void configure(OdinData::IpcMessage& config, OdinData::IpcMessage&) override
    {
        ++configure_calls;
        configured_value = config.get_param<int>("value");
        update_config_ts();
    }

    void requestConfiguration(OdinData::IpcMessage& reply) override
    {
        ++request_configuration_calls;
        reply.set_param(get_name() + "/value", configured_value.load());
    }

    void execute(const std::string& command, OdinData::IpcMessage&) override
    {
        ++execute_calls;
        last_command = command;
    }

    std::vector<std::string> requestCommands() override
    {
        return { "test" };
    }

    void status(OdinData::IpcMessage& reply) override
    {
        ++status_calls;
        reply.set_param(get_name() + "/ready", true);
        update_status_ts();
    }

    bool reset_statistics() override
    {
        ++reset_calls;
        return reset_result;
    }

    void emit_frame()
    {
        push(boost::make_shared<ControllerTestFrame>());
    }

    int frames() const
    {
        return frames_;
    }

    int eoa() const
    {
        return eoa_;
    }

    static std::atomic<int> instances;
    static std::atomic<int> configure_calls;
    static std::atomic<int> execute_calls;
    static std::atomic<int> request_configuration_calls;
    static std::atomic<int> status_calls;
    static std::atomic<int> reset_calls;
    static std::atomic<int> configured_value;
    static bool reset_result;
    static std::string last_command;
    static std::vector<ControllerTestPlugin*> created;

private:
    void process_frame(boost::shared_ptr<FrameProcessor::Frame> frame) override
    {
        ++frames_;
        push(frame);
    }

    void process_end_of_acquisition() override
    {
        ++eoa_;
    }

    std::atomic<int> frames_;
    std::atomic<int> eoa_;
};

std::atomic<int> ControllerTestPlugin::instances(0);
std::atomic<int> ControllerTestPlugin::configure_calls(0);
std::atomic<int> ControllerTestPlugin::execute_calls(0);
std::atomic<int> ControllerTestPlugin::request_configuration_calls(0);
std::atomic<int> ControllerTestPlugin::status_calls(0);
std::atomic<int> ControllerTestPlugin::reset_calls(0);
std::atomic<int> ControllerTestPlugin::configured_value(0);
bool ControllerTestPlugin::reset_result = true;
std::string ControllerTestPlugin::last_command;
std::vector<ControllerTestPlugin*> ControllerTestPlugin::created;

namespace controller_test_plugin_registration {
    REGISTER(FrameProcessor::FrameProcessorPlugin, ControllerTestPlugin, "ControllerTestPlugin");
}

namespace file_writer_test_plugin_registration {
    REGISTER(FrameProcessor::FrameProcessorPlugin, ControllerTestPlugin, "FileWriterPlugin");
}

std::string unique_endpoint(const std::string& purpose)
{
    static std::atomic<unsigned int> endpoint_number(0);
    return "inproc://frame-processor-controller-test-" + purpose + "-" + std::to_string(++endpoint_number);
}

template <typename Predicate> bool wait_until(Predicate predicate, unsigned int timeout_ms = 1000)
{
    const std::chrono::steady_clock::time_point deadline
        = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (!predicate() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return predicate();
}

struct ControllerFixture {
    ControllerFixture() :
        original_debug_level(debug_level),
        original_app_path(OdinData::app_path)
    {
        ControllerTestPlugin::reset();
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

    void configure(OdinData::IpcMessage& config)
    {
        OdinData::IpcMessage reply;
        controller->configure(config, reply);
    }

    void load_plugin(const std::string& index, const std::string& name = "ControllerTestPlugin")
    {
        const size_t existing_instances = ControllerTestPlugin::created.size();
        OdinData::IpcMessage config;
        config.set_param("plugin/load/index", index);
        config.set_param("plugin/load/name", name);
        config.set_param("plugin/load/library", std::string(""));
        configure(config);
        if (ControllerTestPlugin::created.size() > existing_instances && !wait_until([existing_instances] {
                return ControllerTestPlugin::created[existing_instances]->isWorking();
            })) {
            throw std::runtime_error("test plugin worker did not start");
        }
    }

    void connect_plugin(const std::string& index, const std::string& source)
    {
        OdinData::IpcMessage config;
        config.set_param("plugin/connect/index", index);
        config.set_param("plugin/connect/connection", source);
        configure(config);
    }

    void disconnect_plugin(const std::string& index, const std::string& source)
    {
        OdinData::IpcMessage config;
        config.set_param("plugin/disconnect/index", index);
        config.set_param("plugin/disconnect/connection", source);
        configure(config);
    }

    DebugLevel original_debug_level;
    std::string original_app_path;
    boost::shared_ptr<FrameProcessor::FrameProcessorController> controller;
};

OdinData::IpcMessage
exchange_control_message(OdinData::IpcChannel& client, OdinData::IpcMessage& request, unsigned int timeout_ms = 2000)
{
    std::string encoded = request.encode();
    client.send(encoded);
    BOOST_REQUIRE_MESSAGE(client.poll(timeout_ms), "timed out waiting for control-channel reply");
    const std::string reply = client.recv();
    return OdinData::IpcMessage(reply.c_str());
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(FrameProcessorControllerUnitTest, ControllerFixture)

BOOST_AUTO_TEST_CASE(DefaultConfigurationAndVersionAreReported)
{
    OdinData::IpcMessage config_reply;
    controller->requestConfiguration(config_reply, false);

    BOOST_CHECK_EQUAL(config_reply.get_param<std::string>("ctrl_endpoint"), "");
    BOOST_CHECK_EQUAL(config_reply.get_param<std::string>("meta_endpoint"), "");
    BOOST_CHECK_EQUAL(config_reply.get_param<int64_t>("config_ts"), -1);

    OdinData::IpcMessage version_reply;
    controller->provideVersion(version_reply);
    BOOST_CHECK_EQUAL(version_reply.get_param<int>("version/odin-data/major"), ODIN_DATA_VERSION_MAJOR);
    BOOST_CHECK_EQUAL(version_reply.get_param<int>("version/odin-data/minor"), ODIN_DATA_VERSION_MINOR);
    BOOST_CHECK_EQUAL(version_reply.get_param<int>("version/odin-data/patch"), ODIN_DATA_VERSION_PATCH);
}

BOOST_AUTO_TEST_CASE(PluginLoadsThroughConfigurationAndReportsConfigurationAndVersion)
{
    load_plugin("test");
    load_plugin("test");
    BOOST_CHECK_EQUAL(ControllerTestPlugin::instances, 1);

    OdinData::IpcMessage config;
    config.set_param("test/value", 42);
    configure(config);
    BOOST_CHECK_EQUAL(ControllerTestPlugin::configure_calls, 1);
    BOOST_CHECK_EQUAL(ControllerTestPlugin::configured_value, 42);

    OdinData::IpcMessage config_reply;
    controller->requestConfiguration(config_reply, false);
    BOOST_CHECK_EQUAL(ControllerTestPlugin::request_configuration_calls, 1);
    BOOST_CHECK_EQUAL(config_reply.get_param<int>("test/value"), 42);
    BOOST_CHECK_GT(config_reply.get_param<int64_t>("config_ts"), 0);

    const rapidjson::Value& plugins = config_reply.get_param<const rapidjson::Value&>("plugins");
    const rapidjson::Value& names = plugins["names"];
    BOOST_REQUIRE_EQUAL(names.Size(), 1);
    const std::string plugin_name(names[0].GetString());
    BOOST_CHECK_EQUAL(plugin_name, "test");

    OdinData::IpcMessage version_reply;
    controller->provideVersion(version_reply);
    BOOST_CHECK_EQUAL(version_reply.get_param<std::string>("version/test/short"), "1.2.3");
}

BOOST_AUTO_TEST_CASE(StatusMetadataAndClearingErrorsAreIsolated)
{
    load_plugin("test");

    OdinData::IpcMessage status_reply;
    controller->provideStatus(status_reply, true);
    BOOST_CHECK(status_reply.get_param<bool>("test/ready"));
    BOOST_CHECK_EQUAL(status_reply.get_param<std::string>("metadata/test/ready/type"), "bool");
    BOOST_CHECK_EQUAL(status_reply.get_param<std::string>("metadata/test/ready/access_mode"), "r");
    BOOST_CHECK_GT(status_reply.get_param<int64_t>("status_ts"), 0);
    BOOST_REQUIRE_EQUAL(status_reply.get_param<const rapidjson::Value&>("error").Size(), 1);
    BOOST_REQUIRE_EQUAL(status_reply.get_param<const rapidjson::Value&>("warning").Size(), 1);

    OdinData::IpcMessage config_reply;
    controller->requestConfiguration(config_reply, true);
    BOOST_CHECK_EQUAL(config_reply.get_param<std::string>("metadata/test/value/type"), "int");
    BOOST_CHECK_EQUAL(config_reply.get_param<std::string>("metadata/test/value/access_mode"), "rw");

    OdinData::IpcMessage clear;
    clear.set_param("clear_errors", true);
    configure(clear);

    OdinData::IpcMessage cleared_status;
    controller->provideStatus(cleared_status, false);
    BOOST_CHECK(!cleared_status.has_param("error"));
    BOOST_CHECK(!cleared_status.has_param("warning"));
}

BOOST_AUTO_TEST_CASE(CommandsAndStatisticsAreIsolated)
{
    OdinData::IpcMessage empty_command;
    OdinData::IpcMessage empty_reply;
    controller->execute(empty_command, empty_reply);
    BOOST_CHECK_EQUAL(empty_reply.get_msg_type(), OdinData::IpcMessage::MsgTypeNack);
    BOOST_CHECK_EQUAL(empty_reply.get_param<std::string>("error"), "No valid commands found");

    load_plugin("test");
    OdinData::IpcMessage commands_reply;
    controller->requestCommands(commands_reply);
    const rapidjson::Value& test_commands = commands_reply.get_param<const rapidjson::Value&>("test");
    const rapidjson::Value& commands = test_commands["supported"];
    BOOST_REQUIRE_EQUAL(commands.Size(), 1);
    const std::string command_name(commands[0].GetString());
    BOOST_CHECK_EQUAL(command_name, "test");

    OdinData::IpcMessage command;
    command.set_param("test/command", std::string("run"));
    OdinData::IpcMessage command_reply;
    controller->execute(command, command_reply);
    BOOST_CHECK_EQUAL(ControllerTestPlugin::execute_calls, 1);
    BOOST_CHECK_EQUAL(ControllerTestPlugin::last_command, "run");

    OdinData::IpcMessage reset_reply(OdinData::IpcMessage::MsgTypeAck, OdinData::IpcMessage::MsgValCmdResetStatistics);
    controller->resetStatistics(reset_reply);
    BOOST_CHECK_EQUAL(ControllerTestPlugin::reset_calls, 1);
    BOOST_CHECK_EQUAL(reset_reply.get_msg_type(), OdinData::IpcMessage::MsgTypeAck);

    ControllerTestPlugin::reset_result = false;
    controller->resetStatistics(reset_reply);
    BOOST_CHECK_EQUAL(reset_reply.get_msg_type(), OdinData::IpcMessage::MsgTypeNack);
    BOOST_CHECK_EQUAL(reset_reply.get_param<std::string>("error"), "Failed to reset statistics on plugin test");
}

BOOST_AUTO_TEST_CASE(PluginConnectionsAndDisconnectionsUseConfigurationInterface)
{
    load_plugin("source");
    load_plugin("sink");
    BOOST_REQUIRE_EQUAL(ControllerTestPlugin::created.size(), 2);
    ControllerTestPlugin* source = ControllerTestPlugin::created[0];
    ControllerTestPlugin* sink = ControllerTestPlugin::created[1];

    connect_plugin("sink", "source");
    source->emit_frame();
    BOOST_REQUIRE(wait_until([sink] { return sink->frames() == 1; }));

    disconnect_plugin("sink", "source");
    source->emit_frame();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    BOOST_CHECK_EQUAL(sink->frames(), 1);

    connect_plugin("sink", "source");
    OdinData::IpcMessage disconnect_all;
    disconnect_all.set_param("plugin/disconnect", std::string("all"));
    configure(disconnect_all);
    source->emit_frame();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    BOOST_CHECK_EQUAL(sink->frames(), 1);
}

BOOST_AUTO_TEST_CASE(EoaInjectionTargetsOnlyRootPlugins)
{
    load_plugin("source");
    load_plugin("sink");
    BOOST_REQUIRE_EQUAL(ControllerTestPlugin::created.size(), 2);
    ControllerTestPlugin* source = ControllerTestPlugin::created[0];
    ControllerTestPlugin* sink = ControllerTestPlugin::created[1];

    // With sink connected to source, only source is a root plugin. The sink
    // receives the EOA once, through the plugin chain.
    connect_plugin("sink", "source");
    OdinData::IpcMessage inject;
    inject.set_param("inject_eoa", true);
    configure(inject);
    BOOST_REQUIRE(wait_until([source, sink] { return source->eoa() == 1 && sink->eoa() == 1; }));
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    BOOST_CHECK_EQUAL(source->eoa(), 1);
    BOOST_CHECK_EQUAL(sink->eoa(), 1);

    // After the disconnect, both plugins are root plugins and each receives
    // the EOA directly.
    disconnect_plugin("sink", "source");
    configure(inject);
    BOOST_REQUIRE(wait_until([source, sink] { return source->eoa() == 2 && sink->eoa() == 2; }));
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    BOOST_CHECK_EQUAL(source->eoa(), 2);
    BOOST_CHECK_EQUAL(sink->eoa(), 2);
}

BOOST_AUTO_TEST_CASE(StoredConfigurationFrameLimitAndDebugLevelBranchesAreConfigured)
{
    load_plugin("test");

    OdinData::IpcMessage config;
    config.set_param("frames", static_cast<unsigned int>(3));
    config.set_param("debug_level", static_cast<unsigned int>(2));
    config.set_param("hdf/master", std::string("data"));
    config.set_param("store/index", std::string("saved"));
    config.set_param("store/value/test/value", 73);
    configure(config);
    BOOST_CHECK_EQUAL(debug_level, 2);
    BOOST_CHECK_EQUAL(ControllerTestPlugin::configure_calls, 0);

    OdinData::IpcMessage execute;
    execute.set_param("execute/index", std::string("saved"));
    configure(execute);
    BOOST_CHECK_EQUAL(ControllerTestPlugin::configure_calls, 1);
    BOOST_CHECK_EQUAL(ControllerTestPlugin::configured_value, 73);
}

BOOST_AUTO_TEST_CASE(ControlChannelDispatchesRequestsAndRunWaitsForShutdown)
{
    load_plugin("test");
    const std::string endpoint = unique_endpoint("control");
    OdinData::IpcMessage control_config;
    control_config.set_param("ctrl_endpoint", endpoint);
    configure(control_config);

    OdinData::IpcChannel client(ZMQ_DEALER, "controller-test-client");
    client.connect(endpoint.c_str());

    OdinData::IpcMessage status_request(OdinData::IpcMessage::MsgTypeCmd, OdinData::IpcMessage::MsgValCmdStatus);
    status_request.set_msg_id(17);
    status_request.set_param("metadata", true);
    OdinData::IpcMessage status_reply = exchange_control_message(client, status_request);
    BOOST_CHECK_EQUAL(status_reply.get_msg_type(), OdinData::IpcMessage::MsgTypeAck);
    BOOST_CHECK_EQUAL(status_reply.get_msg_id(), 17);
    BOOST_CHECK(status_reply.get_param<bool>("test/ready"));
    BOOST_CHECK_EQUAL(status_reply.get_param<std::string>("metadata/test/ready/type"), "bool");

    OdinData::IpcMessage config_request(
        OdinData::IpcMessage::MsgTypeCmd, OdinData::IpcMessage::MsgValCmdRequestConfiguration
    );
    config_request.set_msg_id(18);
    config_request.set_param("metadata", true);
    OdinData::IpcMessage config_reply = exchange_control_message(client, config_request);
    BOOST_CHECK_EQUAL(config_reply.get_msg_id(), 18);
    BOOST_CHECK_EQUAL(config_reply.get_param<std::string>("metadata/test/value/type"), "int");

    boost::thread run_thread(boost::bind(&FrameProcessor::FrameProcessorController::run, controller.get()));
    BOOST_REQUIRE(wait_until([this] { return controller->isWorking(); }));

    OdinData::IpcMessage shutdown_request(OdinData::IpcMessage::MsgTypeCmd, OdinData::IpcMessage::MsgValCmdShutdown);
    shutdown_request.set_msg_id(19);
    OdinData::IpcMessage shutdown_reply = exchange_control_message(client, shutdown_request);
    BOOST_CHECK_EQUAL(shutdown_reply.get_msg_type(), OdinData::IpcMessage::MsgTypeAck);
    BOOST_CHECK_EQUAL(shutdown_reply.get_msg_id(), 19);
    BOOST_REQUIRE_MESSAGE(run_thread.timed_join(boost::posix_time::seconds(2)), "controller run() did not shut down");
}

BOOST_AUTO_TEST_CASE(MetadataReceiveChannelForwardsMultipartMessages)
{
    const std::string endpoint = unique_endpoint("metadata");
    OdinData::IpcMessage config;
    config.set_param("meta_endpoint", endpoint);
    configure(config);

    OdinData::IpcChannel listener(ZMQ_SUB);
    listener.subscribe("");
    listener.connect(endpoint.c_str());
    FrameProcessor::MetaMessagePublisher publisher;
    publisher.connect_meta_channel();

    int32_t value = 0;
    std::string header;
    bool received = false;
    for (int attempt = 0; attempt < 20 && !received; ++attempt) {
        publisher.publish_meta("test", "counter", static_cast<int32_t>(123), "{\"source\":\"unit\"}");
        if (listener.poll(50)) {
            header = listener.recv();
            BOOST_REQUIRE(!listener.eom());
            BOOST_REQUIRE_EQUAL(listener.recv_raw(&value), sizeof(value));
            BOOST_CHECK(listener.eom());
            received = true;
        }
    }
    BOOST_REQUIRE_MESSAGE(received, "metadata was not forwarded to the configured PUB endpoint");

    rapidjson::Document document;
    BOOST_REQUIRE(!document.Parse(header.c_str()).HasParseError());
    BOOST_CHECK_EQUAL(document["plugin"].GetString(), "test");
    BOOST_CHECK_EQUAL(document["parameter"].GetString(), "counter");
    BOOST_CHECK_EQUAL(document["type"].GetString(), "integer");
    BOOST_CHECK_EQUAL(document["header"]["source"].GetString(), "unit");
    BOOST_CHECK_EQUAL(value, 123);
}

BOOST_AUTO_TEST_CASE(FrameCountTriggersShutdownThroughFileWriterCallback)
{
    load_plugin("writer", "FileWriterPlugin");
    BOOST_REQUIRE_EQUAL(ControllerTestPlugin::created.size(), 1);
    ControllerTestPlugin* writer = ControllerTestPlugin::created[0];

    OdinData::IpcMessage frame_limit;
    frame_limit.set_param("frames", static_cast<unsigned int>(1));
    configure(frame_limit);

    boost::thread run_thread(boost::bind(&FrameProcessor::FrameProcessorController::run, controller.get()));
    BOOST_REQUIRE(wait_until([this, writer] { return controller->isWorking() && writer->isWorking(); }));
    writer->getWorkQueue()->add(boost::make_shared<ControllerTestFrame>());

    BOOST_REQUIRE_MESSAGE(
        run_thread.timed_join(boost::posix_time::seconds(2)), "frame-count-triggered shutdown did not finish run()"
    );
}

BOOST_AUTO_TEST_CASE(PluginLoadFailuresAndMissingConnectionsThrow)
{
    OdinData::IpcMessage missing_plugin;
    missing_plugin.set_param("plugin/load/index", std::string("missing"));
    missing_plugin.set_param("plugin/load/name", std::string("DefinitelyMissingPlugin"));
    missing_plugin.set_param("plugin/load/library", std::string(""));
    OdinData::IpcMessage reply;
    BOOST_CHECK_THROW(controller->configure(missing_plugin, reply), std::runtime_error);

    BOOST_CHECK_THROW(controller->connectPlugin("missing", "frame_receiver"), std::runtime_error);
    BOOST_CHECK_THROW(controller->disconnectPlugin("missing", "frame_receiver"), std::runtime_error);

    load_plugin("test");
    BOOST_CHECK_THROW(controller->connectPlugin("test", "frame_receiver"), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()

#define BOOST_TEST_MODULE "BloscPluginTests"
#define BOOST_TEST_MAIN

#include "BloscPlugin.h"
#include "Fixtures.h"
#include <random>

BOOST_GLOBAL_FIXTURE(GlobalConfig);

class BloscPluginTestFixture {
public:
    BloscPluginTestFixture()
    {
        set_debug_level(3);
        // First create an instance of an engine.
        std::random_device rnd_device;
        // Specify the engine and distribution.
        std::mt19937 mersenne_engine { rnd_device() }; // Generates random integers
        std::uniform_int_distribution<int> dist { 1, 28650 };

        auto gen = [&]() { return dist(mersenne_engine); };

        std::array<int, 64000> vec {}; // 256kb data size
        std::generate(vec.begin(), vec.end(), gen);

        blosc_plugin.set_name("BloscPluginTest");
        frame = boost::make_shared<FrameProcessor::DataBlockFrame>(
            FrameProcessor::FrameMetaData { 7,
                                            "data",
                                            FrameProcessor::raw_32bit,
                                            "scan1",
                                            { vec.size() / 20, 20 },
                                            FrameProcessor::no_compression },
            static_cast<void*>(vec.data()), sizeof(vec[0]) * vec.size()
        );
    }
    ~BloscPluginTestFixture()
    {
    }
    boost::shared_ptr<FrameProcessor::Frame> frame;
    FrameProcessor::BloscPlugin blosc_plugin;
};

BOOST_FIXTURE_TEST_SUITE(BloscPluginUnitTest, BloscPluginTestFixture);

BOOST_AUTO_TEST_CASE(BloscPlugin_compress_decompress)
{
    // process_frame() ought to be a virtual private member method
    // from it's base class FrameProcessorPlugin. This needs to be consolidated!
    OdinData::IpcMessage cfg_;
    OdinData::IpcMessage reply;
    BOOST_CHECK_NO_THROW(cfg_.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_LEVEL, 9));
    BOOST_CHECK_NO_THROW(cfg_.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_SHUFFLE, 2));
    BOOST_CHECK_NO_THROW(cfg_.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_COMPRESSOR, 4));

    BOOST_REQUIRE_NO_THROW(blosc_plugin.configure(cfg_, reply));
    BOOST_REQUIRE_NO_THROW(blosc_plugin.register_callback(
        blosc_plugin.get_name(),
        boost::shared_ptr<FrameProcessor::IFrameCallback>(&blosc_plugin, [](FrameProcessor::IFrameCallback*) { })
    ));
    BOOST_REQUIRE_NO_THROW(blosc_plugin.process_frame(frame));
    boost::shared_ptr<FrameProcessor::Frame> compressed_frame = blosc_plugin.getWorkQueue()->remove();
    int compressed_frame_sz = compressed_frame->get_image_size();
    BOOST_CHECK(compressed_frame_sz < frame->get_image_size());

    // Decompress Test
    BOOST_CHECK_NO_THROW(cfg_.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_MODE, std::string("decompress")));
    BOOST_REQUIRE_NO_THROW(blosc_plugin.configure(cfg_, reply));
    BOOST_REQUIRE_NO_THROW(blosc_plugin.process_frame(compressed_frame));
    boost::shared_ptr<FrameProcessor::Frame> decompressed_frame = blosc_plugin.getWorkQueue()->remove();
    int decompressed_frame_sz = decompressed_frame->get_image_size();
    BOOST_CHECK(decompressed_frame_sz > compressed_frame->get_image_size());
}

BOOST_AUTO_TEST_CASE(BloscPlugin_off)
{
    // OFF Mode Test
    OdinData::IpcMessage cfg_;
    OdinData::IpcMessage reply;
    BOOST_CHECK_NO_THROW(cfg_.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_LEVEL, 9));
    BOOST_CHECK_NO_THROW(cfg_.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_SHUFFLE, 2));
    BOOST_CHECK_NO_THROW(cfg_.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_COMPRESSOR, 4));
    BOOST_CHECK_NO_THROW(cfg_.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_MODE, std::string("off")));
    BOOST_REQUIRE_NO_THROW(blosc_plugin.configure(cfg_, reply));
    BOOST_REQUIRE_NO_THROW(blosc_plugin.register_callback(
        blosc_plugin.get_name(),
        boost::shared_ptr<FrameProcessor::IFrameCallback>(&blosc_plugin, [](FrameProcessor::IFrameCallback*) { })
    ));
    BOOST_REQUIRE_NO_THROW(blosc_plugin.process_frame(frame));
    boost::shared_ptr<FrameProcessor::Frame> same_frame = blosc_plugin.getWorkQueue()->remove();
    int compressed_frame_sz = same_frame->get_image_size();
    BOOST_CHECK(compressed_frame_sz == frame->get_image_size() && frame->get_data_ptr() == same_frame->get_data_ptr());
}

BOOST_AUTO_TEST_CASE(BloscPlugin_request_metadata)
{
    OdinData::IpcMessage reply; // IpcMessage to be populated with metadata
    blosc_plugin.request_configuration_metadata(reply);
    BOOST_CHECK(reply.has_param("metadata")); // !false == true;
    BOOST_CHECK(reply.has_param(
        "metadata/" + blosc_plugin.get_name() + '/' + FrameProcessor::BloscPlugin::CONFIG_BLOSC_COMPRESSOR
    ));
    BOOST_CHECK(
        reply.has_param("metadata/" + blosc_plugin.get_name() + '/' + FrameProcessor::BloscPlugin::CONFIG_BLOSC_THREADS)
    );
    BOOST_CHECK(
        reply.has_param("metadata/" + blosc_plugin.get_name() + '/' + FrameProcessor::BloscPlugin::CONFIG_BLOSC_LEVEL)
    );
    BOOST_CHECK(
        reply.has_param("metadata/" + blosc_plugin.get_name() + '/' + FrameProcessor::BloscPlugin::CONFIG_BLOSC_SHUFFLE)
    );
}
BOOST_AUTO_TEST_SUITE_END(); // BloscPluginUnitTest

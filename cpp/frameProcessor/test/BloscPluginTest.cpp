#define BOOST_TEST_MODULE "BloscPluginTests"
#define BOOST_TEST_MAIN

#include "BloscPlugin.h"
#include "DataBlockFrame.h"
#include "Frame.h"
#include "FrameProcessorDefinitions.h"
#include <DebugLevelLogger.h>
#include <boost/test/unit_test.hpp>
#include <random>
#include "Fixtures.h"

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

        std::array<int, 64000> vec {}; // 64 kb data size
        std::generate(vec.begin(), vec.end(), gen);
        dimensions_t img_dims { 3200, 20 };
        dimensions_t chunk_dims { 1, 3, 4 };

        dset_def.name = "data";
        dset_def.num_frames = 2; // unused?
        dset_def.data_type = FrameProcessor::raw_16bit;
        dset_def.frame_dimensions = dimensions_t(2);
        dset_def.frame_dimensions[0] = 3;
        dset_def.frame_dimensions[1] = 4;
        dset_def.chunks = chunk_dims;

        blosc_plugin.set_name("BloscPluginTest");

        FrameProcessor::FrameMetaData frame_meta(
            7, "data", FrameProcessor::raw_16bit, "scan1", img_dims, FrameProcessor::no_compression
        );
        frame = boost::shared_ptr<FrameProcessor::DataBlockFrame>(
            new FrameProcessor::DataBlockFrame(frame_meta, static_cast<void*>(vec.data()), 256000)
        );
    }
    ~BloscPluginTestFixture()
    {
    }
    boost::shared_ptr<FrameProcessor::Frame> frame;
    std::vector<boost::shared_ptr<FrameProcessor::Frame>> frames;
    FrameProcessor::BloscPlugin blosc_plugin;
    FrameProcessor::DatasetDefinition dset_def;
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
        "BloscPluginTest",
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

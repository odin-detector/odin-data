#define BOOST_TEST_MODULE "BloscPluginTests"
#define BOOST_TEST_MAIN

#include <blosc.h>

#include "BloscPlugin.h"
#include "Fixtures.h"

BOOST_GLOBAL_FIXTURE(GlobalConfig);

class BloscPluginTestFixture {
public:
    BloscPluginTestFixture()
    {
        set_debug_level(3);
        std::array<int, 64000> vec {}; // 256kb data size
        vec.fill(2398); // fill with random value

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
    void setup_blosc_config(OdinData::IpcMessage& cfg_, OdinData::IpcMessage& reply)
    {
        // set compression level to the highest - 9
        BOOST_CHECK_NO_THROW(cfg_.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_LEVEL, 9));
        // set shuffle state to "BLOSC_BITSHUFFLE" - 2
        BOOST_CHECK_NO_THROW(cfg_.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_SHUFFLE, std::string("shuffle")));
        // set the compressor to be - "zlib"
        BOOST_CHECK_NO_THROW(
            cfg_.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_COMPRESSOR, std::string(BLOSC_ZLIB_COMPNAME))
        );
        // set compression mode to "compress". This is the default too however
        BOOST_CHECK_NO_THROW(cfg_.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_MODE, std::string("compress")));
        BOOST_REQUIRE_NO_THROW(blosc_plugin.configure(cfg_, reply));
        // register the frame to itself so we can get the
        BOOST_REQUIRE_NO_THROW(blosc_plugin.register_callback(
            blosc_plugin.get_name(),
            boost::shared_ptr<FrameProcessor::IFrameCallback>(&blosc_plugin, [](FrameProcessor::IFrameCallback*) { })
        ));
    }
    ~BloscPluginTestFixture() = default;
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
    setup_blosc_config(cfg_, reply);
    // Compress Test
    BOOST_REQUIRE_NO_THROW(blosc_plugin.process_frame(frame));
    boost::shared_ptr<FrameProcessor::Frame> compressed_frame = blosc_plugin.getWorkQueue()->remove();
    BOOST_CHECK(compressed_frame->get_image_size() < frame->get_image_size());
    BOOST_TEST_MESSAGE(
        "Compressed frame size = " << compressed_frame->get_image_size()
                                   << " vs Original frame size = " << frame->get_image_size()
    );

    // Decompress Test
    BOOST_CHECK_NO_THROW(cfg_.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_MODE, std::string("decompress")));
    BOOST_REQUIRE_NO_THROW(blosc_plugin.configure(cfg_, reply));
    BOOST_REQUIRE_NO_THROW(blosc_plugin.process_frame(compressed_frame));
    boost::shared_ptr<FrameProcessor::Frame> decompressed_frame = blosc_plugin.getWorkQueue()->remove();
    BOOST_CHECK(
        decompressed_frame->get_image_size() > compressed_frame->get_image_size()
        && decompressed_frame->get_image_size() == frame->get_image_size()
    );
    BOOST_TEST_MESSAGE(
        "Decompressed frame size = " << decompressed_frame->get_image_size()
                                     << " vs Original frame size = " << compressed_frame->get_image_size()
    );
}

BOOST_AUTO_TEST_CASE(BloscPlugin_off)
{
    // OFF Mode Test
    OdinData::IpcMessage cfg_;
    OdinData::IpcMessage reply;
    setup_blosc_config(cfg_, reply);

    // reset the compression mode as "off" - no compression should occur and the source frame should be pushed on queue
    BOOST_CHECK_NO_THROW(cfg_.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_MODE, std::string("off")));
    BOOST_REQUIRE_NO_THROW(blosc_plugin.configure(cfg_, reply));
    BOOST_REQUIRE_NO_THROW(blosc_plugin.process_frame(frame));
    boost::shared_ptr<FrameProcessor::Frame> same_frame = blosc_plugin.getWorkQueue()->remove();
    BOOST_CHECK(
        same_frame->get_image_size() == frame->get_image_size() && frame->get_data_ptr() == same_frame->get_data_ptr()
    );
    BOOST_TEST_MESSAGE(
        "No compression: Original frame size = " << same_frame->get_image_size() << ", Destination "
                                                 << frame->get_image_size()
    );
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

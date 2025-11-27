/*
 * BloscPluginTest.cpp
 *
 */

#include <boost/test/unit_test.hpp>
#include <DebugLevelLogger.h>
#include "FrameProcessorDefinitions.h"
#include "BloscPlugin.h"
#include "Frame.h"
#include "DataBlockFrame.h"

class BloscPluginTestFixture
{
public:
  BloscPluginTestFixture()
  {
    set_debug_level(3);
    unsigned short img[12] =  { 1, 2, 3, 4,
                                5, 6, 7, 8,
                                9,10,11,12 };
    dimensions_t img_dims(2); img_dims[0] = 3; img_dims[1] = 4;
    dimensions_t chunk_dims(3); chunk_dims[0] = 1; chunk_dims[1] = 3; chunk_dims[2] = 4;

    dset_def.name = "data";
    dset_def.num_frames = 2; //unused?
    dset_def.data_type = FrameProcessor::raw_16bit;
    dset_def.frame_dimensions = dimensions_t(2);
    dset_def.frame_dimensions[0] = 3;
    dset_def.frame_dimensions[1] = 4;
    dset_def.chunks = chunk_dims;

    FrameProcessor::FrameMetaData frame_meta(7, "data", FrameProcessor::raw_16bit, "scan1", img_dims, FrameProcessor::no_compression);
    frame = boost::shared_ptr<FrameProcessor::DataBlockFrame>(new FrameProcessor::DataBlockFrame(frame_meta, static_cast<void*>(img), 24));

    for (int i = 1; i<6; i++)
    {
      img[0] = i;
      FrameProcessor::FrameMetaData tmp_frame_meta = frame->get_meta_data_copy();
      tmp_frame_meta.set_frame_number(i);
      tmp_frame_meta.set_acquisition_ID("scan2");
      tmp_frame_meta.set_data_type(FrameProcessor::raw_32bit);
      boost::shared_ptr<FrameProcessor::DataBlockFrame> tmp_frame(new FrameProcessor::DataBlockFrame(tmp_frame_meta, static_cast<void*>(img), 24));
      frames.push_back(tmp_frame);
    }
  }
  ~BloscPluginTestFixture() {}
  boost::shared_ptr<FrameProcessor::Frame> frame;
  std::vector< boost::shared_ptr<FrameProcessor::Frame> >frames;
  FrameProcessor::BloscPlugin blosc_plugin;
  FrameProcessor::DatasetDefinition dset_def;
};

BOOST_FIXTURE_TEST_SUITE(BloscPluginUnitTest, BloscPluginTestFixture);

BOOST_AUTO_TEST_CASE( BloscPlugin_process_frame )
{
  // TODO: simply push a frame through the BloscPlugin to trigger the process_frame call and test the compression
  // Unfortunately process_frame is private and can't be called like this. So how?
  BOOST_REQUIRE_NO_THROW(blosc_plugin.compress_frame(frame));
  BOOST_REQUIRE_NO_THROW(blosc_plugin.compress_frame(frame));
  BOOST_REQUIRE_NO_THROW(blosc_plugin.compress_frame(frames[0]));

}

BOOST_AUTO_TEST_CASE( BloscPlugin_check_config )
{
  OdinData::IpcMessage reply;
  OdinData::IpcMessage cfg;
  blosc_plugin.set_name("BloscPlugin_check_config");
  cfg.set_param<std::string>(FrameProcessor::BloscPlugin::CONFIG_BLOSC_COMPRESSOR, "snappy");
  cfg.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_THREADS, 3);
  cfg.set_param(FrameProcessor::BloscPlugin::CONFIG_BLOSC_LEVEL, 1);
  cfg.set_param<std::string>(FrameProcessor::BloscPlugin::CONFIG_BLOSC_SHUFFLE, "shuffle");

  OdinData::IpcMessage request;
  blosc_plugin.configure(cfg, reply);
  blosc_plugin.requestConfiguration(request);
  BOOST_CHECK_EQUAL(cfg.get_param<std::string>(FrameProcessor::BloscPlugin::CONFIG_BLOSC_COMPRESSOR), \
                    request.get_param<std::string>(blosc_plugin.get_name() + "/" + FrameProcessor::BloscPlugin::CONFIG_BLOSC_COMPRESSOR));
  BOOST_CHECK_EQUAL(cfg.get_param<unsigned int>(FrameProcessor::BloscPlugin::CONFIG_BLOSC_THREADS), \
                    request.get_param<unsigned int>(blosc_plugin.get_name() + "/" + FrameProcessor::BloscPlugin::CONFIG_BLOSC_THREADS));
  BOOST_CHECK_EQUAL(cfg.get_param<int>(FrameProcessor::BloscPlugin::CONFIG_BLOSC_LEVEL), 
                    request.get_param<int>(blosc_plugin.get_name() + "/" + FrameProcessor::BloscPlugin::CONFIG_BLOSC_LEVEL));
  BOOST_CHECK_EQUAL(cfg.get_param<std::string>(FrameProcessor::BloscPlugin::CONFIG_BLOSC_SHUFFLE), \
                    request.get_param<std::string>(blosc_plugin.get_name() + "/" + FrameProcessor::BloscPlugin::CONFIG_BLOSC_SHUFFLE));
}

BOOST_AUTO_TEST_SUITE_END(); //BloscPluginUnitTest

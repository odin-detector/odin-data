/*
 * BloscPluginTest.cpp
 *
 */

#include <boost/test/unit_test.hpp>

#include "FrameProcessorDefinitions.h"
#include "BloscPlugin.h"

class BloscPluginTestFixture
{
public:
  BloscPluginTestFixture()
  {
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

    frame = boost::shared_ptr<FrameProcessor::Frame>(new FrameProcessor::Frame("data"));
    frame->set_frame_number(7);
    frame->set_dimensions("data", img_dims);
    frame->copy_data(static_cast<void*>(img), 24);
    frame->set_data_type(1); // 0: UINT8, 1: UINT16, 2: UINT32
    frame->set_acquisition_id("scan1");

    for (int i = 1; i<6; i++)
    {
      boost::shared_ptr<FrameProcessor::Frame> tmp_frame(new FrameProcessor::Frame("data")); //2, img_dims));
      tmp_frame->set_frame_number(i);
      img[0] = i;
      tmp_frame->copy_data(static_cast<void*>(img), 24);
      tmp_frame->set_data_type(sizeof(img[0]));
      tmp_frame->set_acquisition_id("scan2");
      tmp_frame->set_dimensions("data", img_dims);
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

BOOST_AUTO_TEST_SUITE_END(); //BloscPluginUnitTest

/*
 * LiveViewUnitTest.cpp
 *
 *  Created on: 28 Sep 2018
 *      Author: wbd45595
 */

#include <boost/test/unit_test.hpp>

#include "LiveViewPlugin.h"

class LiveViewPluginTestFixture
{
public:
  LiveViewPluginTestFixture():
    recv_socket(ZMQ_SUB),
    recv_socket_other(ZMQ_SUB)
  {

    for(int i = 0; i < 12; i++){
      img_8[i] = i+1;
      img_16[i] = i+1;
    }

    DATA_TYPES[0]  = "uint8";
    DATA_TYPES[1]  = "uint16";
    DATA_TYPES[2]  = "uint32";

    COMPRESS_TYPES[0] = "none";
    COMPRESS_TYPES[1] = "LZ4";
    COMPRESS_TYPES[2] = "BSLZ4";

    dimensions_t img_dims(2); img_dims[0] = 3; img_dims[1] = 4;

    dset_def.data_type = FrameProcessor::raw_8bit;
    dset_def.frame_dimensions = dimensions_t(2);
    dset_def.frame_dimensions[0] = 3;
    dset_def.frame_dimensions[1] = 4;

    frame = boost::shared_ptr<FrameProcessor::Frame>(new FrameProcessor::Frame("data"));
    frame->set_frame_number(2);
    frame->set_dimensions(dset_def.frame_dimensions);
    frame->set_data_type(dset_def.data_type);
    frame->set_compression(0);
    frame->copy_data(static_cast<void*>(img_8), 12);

    frame_16 = boost::shared_ptr<FrameProcessor::Frame>(new FrameProcessor::Frame("data"));
    frame_16->set_frame_number(2);
    frame_16->set_dimensions(dset_def.frame_dimensions);
    frame_16->set_data_type(FrameProcessor::raw_16bit);
    frame_16->set_compression(0);
    frame_16->copy_data(static_cast<void*>(img_16), 24);

    for(int i = 0; i < 10; i++)
      {
        boost::shared_ptr<FrameProcessor::Frame> tmp_frame(new FrameProcessor::Frame("data"));
        tmp_frame->set_frame_number(i);
        tmp_frame->set_dimensions(dset_def.frame_dimensions);
        tmp_frame->set_data_type(dset_def.data_type);
        tmp_frame->set_dataset_name(i % 4 ? "data" : "not_data");

        tmp_frame->copy_data(static_cast<void*>(img_8), 12);

        frames.push_back(tmp_frame);
      }
    new_socket_recieve = false;


    recv_socket.subscribe("");
    recv_socket_other.subscribe("");
    recv_socket.connect("tcp://127.0.0.1:5020");
    recv_socket_other.connect("tcp://127.0.0.1:5021");

  }
  ~LiveViewPluginTestFixture() {
    BOOST_TEST_MESSAGE("Live View Fixture Teardown");
    recv_socket.close();
    recv_socket_other.close();
  }
  boost::shared_ptr<FrameProcessor::Frame> frame;
  boost::shared_ptr<FrameProcessor::Frame> frame_16;
  std::vector< boost::shared_ptr<FrameProcessor::Frame> >frames;
  FrameProcessor::DatasetDefinition dset_def;

  uint8_t img_8[12];
  uint16_t img_16[12];

  uint8_t  pbuf[12]; //create basic array to store data
  uint16_t pbuf_16[12];
//  const std::string DATA_TYPES[3] = {"uint8","uint16","uint32"};
//  const std::string COMPRESS_TYPES[3] = {"none","LZ4","BSLZ4"};
  std::map<int, std::string> DATA_TYPES;
  std::map<int, std::string> COMPRESS_TYPES;
  bool new_socket_recieve; //we have to declare this here, or we get a memory access error around line 825
  std::string message;

  OdinData::IpcChannel recv_socket;
  OdinData::IpcChannel recv_socket_other;
  FrameProcessor::LiveViewPlugin plugin;
  OdinData::IpcMessage reply;
  OdinData::IpcMessage cfg;
  rapidjson::Document doc;

};

BOOST_FIXTURE_TEST_SUITE(LiveViewPluginUnitTest, LiveViewPluginTestFixture);

BOOST_AUTO_TEST_CASE(LiveViewBasicSendTest)
{
  frame->set_frame_number(FrameProcessor::LiveViewPlugin::DEFAULT_FRAME_FREQ);
  //test we can output frame
  while(!recv_socket.poll(100)) //to avoid issue with slow joiners, using a while loop here
  {
    plugin.process_frame(frame);
  }
  message = recv_socket.recv();
  BOOST_TEST_MESSAGE(message);
  doc.Parse(message.c_str());

  //TEST HEADER CONTENTS
  BOOST_CHECK_EQUAL(doc["frame_num"].GetInt(), frame->get_frame_number());
  BOOST_CHECK_EQUAL(doc["acquisition_id"].GetString(), frame->get_acquisition_id());
  BOOST_CHECK_EQUAL(doc["dtype"].GetString(), DATA_TYPES[frame->get_data_type()]);
  BOOST_CHECK_EQUAL(doc["dsize"].GetInt() , frame->get_data_size());
  BOOST_CHECK_EQUAL(doc["compression"].GetString(), COMPRESS_TYPES[frame->get_compression()]);
  BOOST_CHECK_EQUAL(atoi(doc["shape"][0].GetString()), frame->get_dimensions()[0]);
  BOOST_CHECK_EQUAL(atoi(doc["shape"][1].GetString()), frame->get_dimensions()[1]);


  //TEST DATA CONTENTS
  recv_socket.recv_raw(&pbuf); //put data into arary
  std::vector<uint8_t> buf(frame->get_data_size()); //create vector of size of data
  std::copy(pbuf, pbuf + frame->get_data_size(), buf.begin()); //copy data from array to vector, so it can be compared
  std::vector<uint8_t> original(img_8, img_8 + sizeof img_8 / sizeof img_8[0]);
  BOOST_CHECK_EQUAL_COLLECTIONS(buf.begin(), buf.end() , original.begin(), original.end()); //collection comparison needs beginning and end of each vector

}

BOOST_AUTO_TEST_CASE(LiveViewDownscaleTest)
{

  //TEST DOWNSCALE OPTION
  while(recv_socket.poll(10))
  {
    recv_socket.recv(); //clear any extra data from the above while loop
  }
  //process all frames. with a downscale factor of 2, this should return all the even numbered frames.
  for(int i = 0; i < frames.size(); i++)
  {
    plugin.process_frame(frames[i]);
  }

  std::vector<std::string> processed_frames;
  while(recv_socket.poll(10))
  {
    std::string tmp_string = recv_socket.recv();
    processed_frames.push_back(tmp_string);
    recv_socket.recv_raw(pbuf); //we dont need the data for this test but still need to read from the socket to clear it

  }
  BOOST_CHECK_EQUAL(processed_frames.size(), frames.size()/FrameProcessor::LiveViewPlugin::DEFAULT_FRAME_FREQ);

}

BOOST_AUTO_TEST_CASE(LiveViewConfigTest)
{
  //TEST CONFIGURATION
  cfg.set_param(FrameProcessor::LiveViewPlugin::CONFIG_FRAME_FREQ, 1);
  cfg.set_param(FrameProcessor::LiveViewPlugin::CONFIG_DATASET_NAME, std::string("data"));
  cfg.set_param(FrameProcessor::LiveViewPlugin::CONFIG_SOCKET_ADDR, std::string("tcp://*:5021"));

  BOOST_CHECK_NO_THROW(plugin.configure(cfg, reply));

  //send frame again to check its going to a different socket

  for(int i = 0; i < 10; i ++)
  {
    plugin.process_frame(frame);
    BOOST_CHECK(!recv_socket.poll(100));
    new_socket_recieve = recv_socket_other.poll(100);
    if(new_socket_recieve)
    {
      break;
    }
  }
  BOOST_CHECK(new_socket_recieve);

}

BOOST_AUTO_TEST_CASE(LiveViewDatasetFilterTest)
{
  //TEST DATASET FILTERING
  cfg.set_param(FrameProcessor::LiveViewPlugin::CONFIG_FRAME_FREQ, 1);
  cfg.set_param(FrameProcessor::LiveViewPlugin::CONFIG_DATASET_NAME, std::string("data"));

  plugin.configure(cfg, reply);

  while(recv_socket.poll(10))
  {
    recv_socket.recv(); //clear any extra data from the above while loop
  }

  for(int i = 0; i < frames.size(); i++)
  {
    plugin.process_frame(frames[i]);
  }

  std::vector<std::string> dataset_processed_frames;
  while(recv_socket.poll(10))
  {
    std::string tmp_string = recv_socket.recv();
    BOOST_TEST_MESSAGE( "Received Header: " << tmp_string);
    dataset_processed_frames.push_back(tmp_string);
    recv_socket.recv_raw(pbuf); //we dont need the data for this test but still need to read from the socket to clear it from the queue
  }
  BOOST_CHECK_EQUAL(dataset_processed_frames.size(), 7);
}

BOOST_AUTO_TEST_CASE(LiveViewOtherDatatypeTest)
{
  //test other data types
  while(!recv_socket.poll(10))
  {
    plugin.process_frame(frame_16);
  }
  message = recv_socket.recv();
  BOOST_TEST_MESSAGE(message);
  recv_socket.recv_raw(pbuf_16);
  std::vector<uint16_t> buf_16(12);
  std::copy(pbuf_16, pbuf_16 + frame_16->get_data_size()/2, buf_16.begin());
  std::vector<uint16_t> original_16(img_16, img_16 + sizeof img_16 / sizeof img_16[0]);
  BOOST_CHECK_EQUAL_COLLECTIONS(buf_16.begin(), buf_16.end(), original_16.begin(), original_16.end());

  doc.Parse(message.c_str());
  BOOST_CHECK_EQUAL(doc["dsize"].GetInt() , frame_16->get_data_size());
  BOOST_CHECK_EQUAL(doc["dtype"].GetString(), DATA_TYPES[frame_16->get_data_type()]);

}

BOOST_AUTO_TEST_SUITE_END(); //LiveViewPluginUnitTest

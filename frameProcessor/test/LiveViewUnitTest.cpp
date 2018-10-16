/*
 * LiveViewUnitTest.cpp
 *
 *  Created on: 28 Sep 2018
 *      Author: Adam Neaves - wbd45595
 */

#include <boost/test/unit_test.hpp>

#include "LiveViewPlugin.h"

std::string global_socket_addr= "tcp://127.0.0.1:";
uint32_t global_socket_port = 5020;

/**
 * Test fixture for the Live View Unit Tests. sets up the plugin and other things required by the tests.
 */
class LiveViewPluginTestFixture
{

public:
  LiveViewPluginTestFixture():
    recv_socket(ZMQ_SUB),
    recv_socket_other(ZMQ_SUB)
  {

    //set up the recieve sockets so we can read data from the plugin's live output.
    recv_socket.subscribe("");
    recv_socket_other.subscribe("");
    std::string addr = global_socket_addr + boost::to_string(global_socket_port);
    BOOST_TEST_MESSAGE("Address: " + addr);
    recv_socket.connect(addr);
    recv_socket_other.connect("tcp://127.0.0.1:5050");
    OdinData::IpcMessage tmp_cfg;
    tmp_cfg.set_param(FrameProcessor::LiveViewPlugin::CONFIG_SOCKET_ADDR, std::string(addr));
    plugin.configure(tmp_cfg, reply);

    //create dummy data for the test frames
    for(int i = 0; i < 12; i++)
    {
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

    //create test frame
    frame = boost::shared_ptr<FrameProcessor::Frame>(new FrameProcessor::Frame("data"));
    frame->set_frame_number(2);
    frame->set_dimensions(img_dims);
    frame->set_data_type(FrameProcessor::raw_8bit);
    frame->set_compression(0);
    frame->copy_data(static_cast<void*>(img_8), 12);

    //create test frame with uint16 data
    frame_16 = boost::shared_ptr<FrameProcessor::Frame>(new FrameProcessor::Frame("data"));
    frame_16->set_frame_number(2);
    frame_16->set_dimensions(img_dims);
    frame_16->set_data_type(FrameProcessor::raw_16bit);
    frame_16->set_compression(0);
    frame_16->copy_data(static_cast<void*>(img_16), 24);

    //create multiple test frames
    for(int i = 0; i < 10; i++)
    {
      boost::shared_ptr<FrameProcessor::Frame> tmp_frame(new FrameProcessor::Frame("data"));
      tmp_frame->set_frame_number(i);
      tmp_frame->set_dimensions(img_dims);
      tmp_frame->set_data_type(FrameProcessor::raw_8bit);
      tmp_frame->set_dataset_name(i % 4 ? "data" : "not_data");

      tmp_frame->copy_data(static_cast<void*>(img_8), 12);

      frames.push_back(tmp_frame);
    }

    new_socket_recieve = false;

    //need to make sure the recv socket has finished connecting
    uint32_t attempts_left = 10;
    while(!recv_socket.poll(100) && attempts_left > 0) //to avoid issue with slow subscribers, keep sending the frame until the subscriber has received it
      {
        //send the frame to the plugin
        plugin.process_frame(frame);
        attempts_left --;
      }
    while(recv_socket.poll(10))
    {
      recv_socket.recv();
    }
    BOOST_TEST_MESSAGE("FIXTURE SETUP COMPELTE");
  }

  ~LiveViewPluginTestFixture() {
    BOOST_TEST_MESSAGE("Live View Fixture Teardown");
    recv_socket.close();
    recv_socket_other.close();
    global_socket_port ++;

  }

  boost::shared_ptr<FrameProcessor::Frame> frame;
  boost::shared_ptr<FrameProcessor::Frame> frame_16;
  std::vector< boost::shared_ptr<FrameProcessor::Frame> >frames;

  uint8_t img_8[12];
  uint16_t img_16[12];

  uint8_t  pbuf[12]; //create basic array to store data
  uint16_t pbuf_16[12];

  std::map<int, std::string> DATA_TYPES;
  std::map<int, std::string> COMPRESS_TYPES;

  bool new_socket_recieve;
  std::string message;

  OdinData::IpcChannel recv_socket;
  OdinData::IpcChannel recv_socket_other;
  FrameProcessor::LiveViewPlugin plugin;
  OdinData::IpcMessage reply;
  OdinData::IpcMessage cfg;
  rapidjson::Document doc;

};

BOOST_FIXTURE_TEST_SUITE(LiveViewPluginUnitTest, LiveViewPluginTestFixture);

/**
 * Tests to ensure the configuration works, including testing to make sure we can change the address of the live view socket
 */
BOOST_AUTO_TEST_CASE(LiveViewConfigTest)
{
  //TEST CONFIGURATION
  cfg.set_param(FrameProcessor::LiveViewPlugin::CONFIG_FRAME_FREQ, 1);
  cfg.set_param(FrameProcessor::LiveViewPlugin::CONFIG_DATASET_NAME, std::string("data"));
  cfg.set_param(FrameProcessor::LiveViewPlugin::CONFIG_SOCKET_ADDR, std::string("tcp://127.0.0.1:5050"));

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

/**
 * tests the basic functionality, passing the plugin a single frame and seeing if it appears the same on the other end of the socket
 */
BOOST_AUTO_TEST_CASE(LiveViewBasicSendTest)
{
  //setting the frame number here guarantees that it'll be passed to the live view socket
  frame->set_frame_number(FrameProcessor::LiveViewPlugin::DEFAULT_FRAME_FREQ);
  //test we can output frame

  uint32_t attempts_left = 10;
  while(!recv_socket.poll(100) && attempts_left > 0) //to avoid issue with slow subscribers, keep sending the frame until the subscriber has received it
  {
    //send the frame to the plugin
    plugin.process_frame(frame);
    attempts_left --;
  }
  if(attempts_left){
    message = recv_socket.recv();
    BOOST_TEST_MESSAGE(message);
  }
  doc.Parse(message.c_str());

  //TEST HEADER CONTENTS
  BOOST_CHECK_EQUAL(doc["frame_num"].GetInt(), frame->get_frame_number());
  BOOST_CHECK_EQUAL(doc["acquisition_id"].GetString(), frame->get_acquisition_id());
  BOOST_CHECK_EQUAL(doc["dtype"].GetString(), DATA_TYPES[frame->get_data_type()]);
  BOOST_CHECK_EQUAL(doc["dsize"].GetInt() , frame->get_data_size());
  BOOST_CHECK_EQUAL(doc["compression"].GetString(), COMPRESS_TYPES[frame->get_compression()]);
  char *end;
  BOOST_CHECK_EQUAL(strtol(doc["shape"][0].GetString(), &end, 10), frame->get_dimensions()[0]);
  BOOST_CHECK_EQUAL(strtol(doc["shape"][1].GetString(), &end, 10), frame->get_dimensions()[1]);


  //TEST DATA CONTENTS
  recv_socket.recv_raw(&pbuf); //put data into arary
  std::vector<uint8_t> buf(frame->get_data_size()); //create vector of size of data
  std::copy(pbuf, pbuf + frame->get_data_size(), buf.begin()); //copy data from array to vector, so it can be compared
  std::vector<uint8_t> original(img_8, img_8 + sizeof img_8 / sizeof img_8[0]);
  BOOST_CHECK_EQUAL_COLLECTIONS(buf.begin(), buf.end() , original.begin(), original.end());

}

/**
 * tests to make sure the downscale factor works. Checks that only those frames with the needed frame number are passed to the live view socket
 */
BOOST_AUTO_TEST_CASE(LiveViewDownscaleTest)
{

  //TEST DOWNSCALE OPTION
  cfg.set_param(FrameProcessor::LiveViewPlugin::CONFIG_FRAME_FREQ, 2);
  plugin.configure(cfg, reply);
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
  BOOST_CHECK_EQUAL(processed_frames.size(), 5);

}

/**
 * test the filtering by dataset option works
 */
BOOST_AUTO_TEST_CASE(LiveViewDatasetFilterTest)
{
  //TEST DATASET FILTERING
  cfg.set_param(FrameProcessor::LiveViewPlugin::CONFIG_FRAME_FREQ, 1);
  cfg.set_param(FrameProcessor::LiveViewPlugin::CONFIG_DATASET_NAME, std::string("data"));

  plugin.configure(cfg, reply); //configure the plugin to push any frame with the "data" dataset to the live view socket

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
  for(int i = 0; i < dataset_processed_frames.size(); i++)
  {
    doc.Parse(dataset_processed_frames[i].c_str());
    BOOST_CHECK_EQUAL(doc["dataset"].GetString(), "data"); //check to make sure only "data" frames were passed through
  }
  BOOST_CHECK_EQUAL(dataset_processed_frames.size(), 7); // check to make sure all the frames expected were passed through
}

/**
 * test to make sure the live view plugin can work with data types of more than a byte, and that the data is still preserved when it gets passed through
 */
BOOST_AUTO_TEST_CASE(LiveViewOtherDatatypeTest)
{

  //send the frame of uint16 data until the receiver socket can receive something
  uint32_t attempts_left = 10;
  while(!recv_socket.poll(10) && attempts_left > 0)
  {
    plugin.process_frame(frame_16);
    attempts_left --;
  }
  if(attempts_left)
  {
    message = recv_socket.recv();
    BOOST_TEST_MESSAGE(message);
    recv_socket.recv_raw(pbuf_16);
  }
  std::vector<uint16_t> buf_16(12); //create vector to store the data
  std::copy(pbuf_16, pbuf_16 + frame_16->get_data_size()/2, buf_16.begin()); //Divide the data size in half, as each byte is only half a point of data
  std::vector<uint16_t> original_16(img_16, img_16 + sizeof img_16 / sizeof img_16[0]);
  BOOST_CHECK_EQUAL_COLLECTIONS(buf_16.begin(), buf_16.end(), original_16.begin(), original_16.end()); //test that the data is the same

  doc.Parse(message.c_str());
  BOOST_CHECK_EQUAL(doc["dsize"].GetInt() , frame_16->get_data_size()); //test that the plugin got the correct size for the frame
  BOOST_CHECK_EQUAL(doc["dtype"].GetString(), DATA_TYPES[frame_16->get_data_type()]); //test that the plugin got the correct data type for the frame

}

BOOST_AUTO_TEST_SUITE_END(); //LiveViewPluginUnitTest

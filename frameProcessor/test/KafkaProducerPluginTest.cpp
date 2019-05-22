#include <boost/test/unit_test.hpp>
#include <cstring>
#include <cstdlib>
#include "rapidjson/document.h"
#include "KafkaProducerPlugin.h"
#include "DataBlockFrame.h"

class KafkaProducerPluginTestFixture {
public:
  KafkaProducerPluginTestFixture()
  {

    for (int i = 0; i < 12; i++) {
      test_data[i] = i + 1;
    }
    dimensions_t test_dims(2); test_dims[0] = 3; test_dims[1] = 4;

    FrameProcessor::FrameMetaData frame_meta(
            7, "data", FrameProcessor::raw_16bit, "test", test_dims, FrameProcessor::no_compression
    );
    frame = boost::shared_ptr<FrameProcessor::DataBlockFrame>(
            new FrameProcessor::DataBlockFrame(frame_meta, static_cast<void *>(test_data), 24));
  }

  ~KafkaProducerPluginTestFixture() { }

  unsigned short test_data[12];
  dimensions_t test_dims;
  FrameProcessor::KafkaProducerPlugin plugin;
  boost::shared_ptr<FrameProcessor::Frame> frame;
};

BOOST_FIXTURE_TEST_SUITE(KafkaProducerPluginUnitTest, KafkaProducerPluginTestFixture);

BOOST_AUTO_TEST_CASE(KafkaProducerPluginCheckMessageContent)
{

  size_t msg_size;
  void *data = plugin.create_message(frame, msg_size);

  BOOST_CHECK(data != NULL);

  uint16_t header_size = *(static_cast<uint16_t *>(data));

  // there is frame data and it's the same as TEST_DATA
  BOOST_CHECK_EQUAL(0,
                    memcmp(test_data,
                           static_cast<char *>(data) + sizeof(uint16_t)
                             + header_size,
                           sizeof(test_data)));
  free(data);
}

BOOST_AUTO_TEST_CASE(KafkaProducerPluginCheckMessageSize)
{
  size_t msg_size;
  void *data = plugin.create_message(frame, msg_size);
  uint16_t header_size = *(static_cast<uint16_t *>(data));

  BOOST_CHECK(data != NULL);

  // Total size is the sum of each part size: [header size] + [header] + [data]
  BOOST_CHECK(msg_size == sizeof(uint16_t) + header_size + sizeof(test_data));
  free(data);
}

BOOST_AUTO_TEST_CASE(KafkaProducerPluginCheckMessageHeader)
{
  size_t msg_size;
  void *data = plugin.create_message(frame, msg_size);

  BOOST_CHECK(data != NULL);

  uint16_t header_size = *(static_cast<uint16_t *>(data));
  char* header_data = static_cast<char *>(data) + sizeof(uint16_t);
  // Check that the header ends with a null byte
  BOOST_CHECK(header_data[header_size - 1] == 0);
  rapidjson::Document document;
  document.Parse(header_data);
  BOOST_CHECK(document[MSG_HEADER_FRAME_NUMBER_KEY].GetInt() == 7);
  BOOST_CHECK(document[MSG_HEADER_DATA_TYPE_KEY].GetInt() == FrameProcessor::raw_16bit);
  BOOST_CHECK(document[MSG_HEADER_FRAME_SIZE_KEY].GetInt() == sizeof(test_data));
  rapidjson::Value &json_dims = document[MSG_HEADER_FRAME_DIMENSIONS_KEY];
  for (int i = 0; i < test_dims.size(); i++) {
    BOOST_CHECK(test_dims[i] == json_dims[i].GetUint());
  }
  free(data);
}

BOOST_AUTO_TEST_SUITE_END(); //KafkaProducerPluginUnitTest

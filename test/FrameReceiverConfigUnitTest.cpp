/*!
 * FrameRecevierConfigTest.cpp
 *
 *  Created on: Feb 4, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include <boost/test/unit_test.hpp>

#include "FrameReceiverConfig.h"

namespace FrameReceiver
{
    // This class, which is a friend of FrameReceiverConfig, allows testing of the
    // correct initialization of FrameReceiverConfig
    class FrameReceiverConfigTestProxy
    {
    public:
        FrameReceiverConfigTestProxy(FrameReceiver::FrameReceiverConfig& config) :
            mConfig(config)
        {  }
        void test_config(void)
        {
            BOOST_CHECK_EQUAL(mConfig.max_buffer_mem_, FrameReceiver::Defaults::default_max_buffer_mem);
            BOOST_CHECK_EQUAL(mConfig.sensor_type_, FrameReceiver::Defaults::SensorTypeIllegal);
            std::vector<uint16_t> port_list;
            mConfig.tokenize_port_list(port_list, FrameReceiver::Defaults::default_rx_port_list);
            BOOST_CHECK_EQUAL(mConfig.rx_ports_.size(), port_list.size());
            for (int i = 0; i < mConfig.rx_ports_.size(); i++)
            {
                BOOST_CHECK_EQUAL(mConfig.rx_ports_[i], port_list[i]);
            }
            BOOST_CHECK_EQUAL(mConfig.rx_address_, FrameReceiver::Defaults::default_rx_address);
        }
    private:
        FrameReceiver::FrameReceiverConfig& mConfig;
    };
}

BOOST_AUTO_TEST_SUITE(FrameReceiverConfigUnitTest);

BOOST_AUTO_TEST_CASE( ValidConfigWithDefaults )
{
    // Instantiate a configuration and a test proxy
    FrameReceiver::FrameReceiverConfig theConfig;
    FrameReceiver::FrameReceiverConfigTestProxy testProxy(theConfig);

    // Use the test proxy to test that the configuration object has the correct default
    // private configuration variables
    testProxy.test_config();
}

BOOST_AUTO_TEST_CASE( ValidSensorNameToTypeMapping )
{
    FrameReceiver::FrameReceiverConfig theConfig;
    std::string p2mName         = "percival2m";
    std::string p13mName        = "percival13m";
    std::string excalibur3mName = "excalibur3m";
    std::string badName         = "foo";

    // Check that the sensor name mapping to type works for all known values, and returns illegal for a bad name
    BOOST_CHECK_EQUAL(theConfig.map_sensor_name_to_type(p2mName), FrameReceiver::Defaults::SensorTypePercival2M);
    BOOST_CHECK_EQUAL(theConfig.map_sensor_name_to_type(p13mName), FrameReceiver::Defaults::SensorTypePercival13M);
    BOOST_CHECK_EQUAL(theConfig.map_sensor_name_to_type(excalibur3mName), FrameReceiver::Defaults::SensorTypeExcalibur3M);
    BOOST_CHECK_EQUAL(theConfig.map_sensor_name_to_type(badName), FrameReceiver::Defaults::SensorTypeIllegal);
}

BOOST_AUTO_TEST_SUITE_END();



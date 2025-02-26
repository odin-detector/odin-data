/*
 * MetaMessageUnitTest.cpp
 *
 *  Created on: 8 May 2017
 *      Author: gnx91527
 */

#include <iostream>

#include <boost/test/unit_test.hpp>
#include <memory>

#include <log4cxx/logger.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/simplelayout.h>

#include "MetaMessage.h"

class MetaMessageUnitTestFixture
{
public:
  MetaMessageUnitTestFixture() :
      logger(log4cxx::Logger::getLogger("MetaMessageUnitTest"))
  {

  }
  log4cxx::LoggerPtr logger;
};
BOOST_FIXTURE_TEST_SUITE(MetaMessageUnitTest, MetaMessageUnitTestFixture);

BOOST_AUTO_TEST_CASE( MetaMessageTest )
{
  int v1 = 12345;
  std::shared_ptr<FrameProcessor::MetaMessage> mm1(new FrameProcessor::MetaMessage("name1", "item1", "integer", "header1", sizeof(int), &v1));

  BOOST_CHECK_EQUAL(mm1->getName(), "name1");
  BOOST_CHECK_EQUAL(mm1->getItem(), "item1");
  BOOST_CHECK_EQUAL(mm1->getType(), "integer");
  BOOST_CHECK_EQUAL(mm1->getHeader(), "header1");
  BOOST_CHECK_EQUAL(mm1->getSize(), sizeof(int));
  BOOST_CHECK_EQUAL(*((int *)mm1->getDataPtr()), v1);

}

BOOST_AUTO_TEST_SUITE_END();

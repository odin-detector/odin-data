/*
 * JSONMessage.h
 *
 *  Created on: 26 May 2016
 *      Author: gnx91527
 */

#ifndef TOOLS_FILEWRITER_JSONMESSAGE_H_
#define TOOLS_FILEWRITER_JSONMESSAGE_H_

#include <stdint.h>
#include <stdexcept>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
using namespace rapidjson;

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

namespace filewriter
{

  /**
   * The JSONMessage class provides a wrapper around a rapid JSON document
   * and exposes some useful methods, including the [] operator and toString
   * method.
   *
   * A JSONMessage object can either be created from a string JSON representation
   * of from a Value of an existing JSONMessage object.
   */
  class JSONMessage
  {
  public:
    JSONMessage(const std::string& msgString);
    JSONMessage(const Value& value);
    virtual ~JSONMessage();
    Value& operator[](const std::string& name);
    bool HasMember(const std::string& name);
    std::string toString();

  private:
    LoggerPtr logger_;
    Document *messageDocument_;
  };

} /* namespace filewriter */

#endif /* TOOLS_FILEWRITER_JSONMESSAGE_H_ */

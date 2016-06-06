/*
 * JSONMessage.cpp
 *
 *  Created on: 26 May 2016
 *      Author: gnx91527
 */

#include <JSONMessage.h>

namespace filewriter
{

  /** JSONMessage
   * Constructor.  Creates and stores a rapid JSON Document instance.
   *
   * \param[in] msgString - String representation of the JSON object.
   */
  JSONMessage::JSONMessage(const std::string& msgString)
  {
    // Setup logging for the class
    logger_ = Logger::getLogger("JSONMessage");
    logger_->setLevel(Level::getAll());
    LOG4CXX_TRACE(logger_, "JSONMessage constructor.");

    // Create a new Document
    messageDocument_ = new Document;
    // Parse the JSON string into a Document DOM object
    messageDocument_->Parse(msgString.c_str());
  }

  /** JSONMessage
   * Constructor.  Creates and stores a rapid JSON Document instance.
   *
   * \param[in] value - Value reference from a rapid JSON document.
   */
  JSONMessage::JSONMessage(const Value& value)
  {
    // Setup logging for the class
    logger_ = Logger::getLogger("JSONMessage");
    logger_->setLevel(Level::getAll());
    LOG4CXX_TRACE(logger_, "JSONMessage constructor.");

    // Create a new Document
    messageDocument_ = new Document;

    // Setup a string buffer and a writer so we can stringify the Document DOM object
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);

    // Encode the JSON Document DOM object using the writer
    value.Accept(writer);
    // Create a copy of the JSON string
    std::string sValue(buffer.GetString());

    // Parse the JSON string into a Document DOM object
    messageDocument_->Parse(sValue.c_str());
  }

  /** ~JSONMessage
   * Destructor.
   */
  JSONMessage::~JSONMessage()
  {
    LOG4CXX_TRACE(logger_, "JSONMessage destructor.");
    // Delete resources
    delete messageDocument_;
  }

  /** operator[]
   * Index operator.  Checks for existence of the name and then returns
   * a reference to the Value obtained from the rapid JSON document.
   *
   * \param[in] name - String name of the JSON value.
   * \return - Reference to the rapid JSON value.
   */
  Value& JSONMessage::operator[](const std::string& name)
  {
    // Check if the value exists
    if (!messageDocument_->HasMember(name.c_str())){
      // No value of specified name, raise an exception
      throw std::range_error("No attribute name found");
    }
    // Return a reference to the value
    Value& val = (*messageDocument_)[name.c_str()];
    return val;
  }

  /** HasMember
   * Check if the JSON object has the requrested value.
   *
   * \param[in] name - String name of the JSON value.
   * \return - Boolean true if the value exists.
   */
  bool JSONMessage::HasMember(const std::string& name)
  {
    return messageDocument_->HasMember(name.c_str());
  }

  /** toString
   * Generates a string representation of the JSON object.
   *
   * \return - String representation of the JSON object.
   */
  std::string JSONMessage::toString()
  {
    // Setup a string buffer and a writer so we can stringify the Document DOM object
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);

    // Encode the JSON Document DOM object using the writer
    messageDocument_->Accept(writer);
    // Create a copy of the JSON string
    std::string sMessage(buffer.GetString());
    // Return the string
    return sMessage;
  }

} /* namespace filewriter */

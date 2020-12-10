/*
 * Json.cpp
 *
 *  Created on: 10 Dec 2020
 *      Author: Gary Yendell
 */

#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "Json.h"

namespace OdinData
{

JsonDict::JsonDict()
{
  document_.SetObject();
}

/** Add a key value pair to the underlying rapidjson::Document
 *
 * \param[in] key - The key of the item to add
 * \param[in] value - The value of the item to add
 */
void JsonDict::add(const std::string& key, rapidjson::Value& json_value)
{
  rapidjson::Value json_key(key.c_str(), document_.GetAllocator());
  document_.AddMember(json_key, json_value, document_.GetAllocator());
}

void JsonDict::add(const std::string& key, const std::string& value) {
  rapidjson::Value json_value;
  json_value.SetString(value.c_str(), value.length(), document_.GetAllocator());
  add(key, json_value);
}

/** Generate a string from the rapidjson::Document
 *
 * \return - The string
 */
std::string JsonDict::str() const
{
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> docWriter(buffer);
  document_.Accept(docWriter);

  // Return our own copy, as the original could be lost when it goes out of scope
  std::string message(buffer.GetString());

  return message;
}

} /* namespace OdinData */

/*
 * Json.h
 *
 *  Created on: 10 Dec 2020
 *      Author: Gary Yendell
 */

#ifndef JSON_H_
#define JSON_H_

#include <string>
#include <vector>

#include "rapidjson/document.h"

namespace OdinData
{

class JsonDict
{
public:
  JsonDict();

  /** Add a scalar value to the underlying rapidjson::Document
   *
   * \param[in] key - The key of the item to add
   * \param[in] value - The value of the item to add
   */
  void add(const std::string& key, const std::string& json_value);
  void add(const std::string& key, const long long unsigned json_value);
  template <typename T> void add(const std::string& key, T value)
  {
    rapidjson::Value json_value(value);
    add(key, json_value);
  }

  /** Add a vector value to the underlying rapidjson::Document
   *
   * \param[in] key - The key of the item to add
   * \param[in] value - The value of the item to add
   */
  void add(const std::string& key, const std::vector<long long unsigned>& value);
  template <typename T> void add(const std::string& key, std::vector<T> value)
  {
    rapidjson::Value json_value;
    json_value.SetArray();
    typename std::vector<T>::iterator it;
    for (it = value.begin(); it != value.end(); it++) {
      json_value.PushBack(*it, document_.GetAllocator());
    }
    add(key, json_value);
  }

  /** Return a json representation of the dictionary
   *
   */
  std::string str() const;

private:
  void add(const std::string& key, rapidjson::Value& json_value);

  // RapidJSON document object
  rapidjson::Document document_;
};

} /* namespace OdinData */

#endif /* JSON_H_ */

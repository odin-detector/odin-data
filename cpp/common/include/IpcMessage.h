/*!
 * IpcMessage.h - Frame Receiver Inter-Process Communication JSON Message format class
 *
 *  Created on: Jan 26, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef IPCMESSAGE_H_
#define IPCMESSAGE_H_

#include <iostream>
#include <string>
#include <exception>
#include <algorithm>
#include <map>
#include <sstream>
#include <time.h>

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/bimap.hpp"
#include "boost/variant.hpp"

namespace OdinData
{
/** This struct is a representation of the metadata
*/
struct ParamMetadata
{
  const std::string path;
  const std::string type;
  const std::string access_mode;
  const std::vector<boost::variant<std::string, int, float>> allowed_values;
  const int32_t min;
  const int32_t max;
  const bool has_min;
  const bool has_max;
};
//! IpcMessageException - custom exception class implementing "what" for error string
class IpcMessageException : public std::exception
{
public:

  //! Create IpcMessageException with no message
  IpcMessageException(void) throw() :
      what_("")
  { };

  //! Creates IpcMessageExcetpion with informational message
  IpcMessageException(const std::string what) throw() :
      what_(what)
  {};

  //! Returns the content of the informational message
  virtual const char* what(void) const throw()
  {
    return what_.c_str();
  };

  //! Destructor
  ~IpcMessageException(void) throw() {};

private:

  // Member variables
  const std::string what_;  //!< Informational message about the exception

}; // IpcMessageException

} // namespace OdinData

// Override rapidsjon assertion mechanism before including appropriate headers
#ifdef RAPIDJSON_ASSERT
#undef RAPIDJSON_ASSERT
#endif
#define RAPIDJSON_ASSERT(x) if (!(x)) throw OdinData::IpcMessageException("rapidjson assertion thrown");
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"
#include "rapidjson/pointer.h"

namespace OdinData
{

//! IpcMessage - inter-process communication JSON message format class
class IpcMessage
{

public:

  //! Type attribute of IPC message
  enum MsgType {
    MsgTypeIllegal = -1,  //!< Illegal message
    MsgTypeCmd,           //!< Command
    MsgTypeAck,           //!< Message acknowledgement
    MsgTypeNack,          //!< Message no-ackonowledgement
    MsgTypeNotify,        //!< Notify message
  };

  //! Value attribute of IPC message
  enum MsgVal {
    MsgValIllegal = -1,              //!< Illegal value
    MsgValCmdReset,                  //!< Reset command message
    MsgValCmdStatus,                 //!< Status command message
    MsgValCmdConfigure,              //!< Configure command message
    MsgValCmdRequestConfiguration,   //!< Request configuration command message
    MsgValCmdExecute,                //!< Execute a command message
    MsgValCmdRequestCommands,        //!< Request available commands message
    MsgValCmdRequestVersion,         //!< Request version information message
    MsgValCmdBufferConfigRequest,    //!< Buffer configuration request
    MsgValCmdBufferPrechargeRequest, //!< Buffer precharge request
    MsgValCmdResetStatistics,        //!< Reset statistics command
    MsgValCmdShutdown,               //!< Process shutdown request
    MsgValNotifyIdentity,            //!< Identity notification message
    MsgValNotifyFrameReady,          //!< Frame ready notification message
    MsgValNotifyFrameRelease,        //!< Frame release notification message
    MsgValNotifyBufferConfig,        //!< Buffer configuration notification
    MsgValNotifyBufferPrecharge,     //!< Buffer precharge notification
    MsgValNotifyStatus,              //!< Status notification 
  };

  //! Internal bi-directional mapping of message type from string to enumerated MsgType
  typedef boost::bimap<std::string, MsgType> MsgTypeMap;
  //! Internal bi-directional mapping of message type from string to enumerated MsgType
  typedef MsgTypeMap::value_type MsgTypeMapEntry;
  //! Internal bi-directional mapping of message value from string to enumerated MsgVal
  typedef boost::bimap<std::string, MsgVal> MsgValMap;
  //! Internal bi-directional mapping of message value from string to enumerated MsgVal
  typedef MsgValMap::value_type MsgValMapEntry;

  IpcMessage(MsgType msg_type=MsgTypeIllegal, MsgVal msg_val=MsgValIllegal, bool strict_validation=true);

  IpcMessage(const char* json_msg, bool strict_validation=true);

  IpcMessage(const rapidjson::Value& value,
             MsgType msg_type=MsgTypeIllegal,
             MsgVal msg_val=MsgValIllegal,
             bool strict_validation=true);

  //! Updates parameters from another IPCMessage.
  void update(const IpcMessage& other);

  //! Updates parameters from a rapidJSON object.
  void update(const rapidjson::Value& param_val, std::string param_prefix="");

  //! Gets the value of a named parameter in the message.
  //!
  //! This template method returns the value of the specified parameter stored in the
  //! params block of the message. If the block or parameter is missing, an exception
  //! of type IpcMessageException is thrown.
  //!
  //! \param param_name - string name of the parameter to return
  //! \return The value of the parameter if present, otherwise an exception is thrown

  template<typename T> T get_param(std::string const& param_name) const
  {
    // Locate the params block and throw exception if absent
    rapidjson::Value::ConstMemberIterator itr = doc_.FindMember("params");
    if (itr == doc_.MemberEnd())
    {
      throw IpcMessageException("Missing params block in message");
    }

    if (param_name.find('/') != param_name.npos) {
      std::string nodeName = param_name.substr(0, param_name.find('/'));
      std::string subParam = param_name.substr(param_name.find('/') + 1, param_name.npos);

      OdinData::IpcMessage node(this->get_param<const rapidjson::Value &>(nodeName));
      return node.get_param<T>(subParam);
    }
    else {
      // Locate parameter within block and throw exception if absent, otherwise return value
      rapidjson::Value::ConstMemberIterator param_itr = itr->value.FindMember(param_name.c_str());
      if (param_itr == itr->value.MemberEnd())
      {
        std::stringstream ss;
        throw IpcMessageException(ss.str());
      }
      return get_value<T>(param_itr);
    }

  }

  //! Gets the value of a named parameter in the message.
  //!
  //! This template method returns the value of the specified parameter stored in the
  //! params block of the message. if the block or parameter is missing, the specified
  //! default value is returned in its place.
  //!
  //! \param param_name - string name of the parameter to return
  //! \param default_value - default value to return if parameter not present in message
  //! \return The value of the parameter if present, otherwise the specified default value

  template<typename T> T get_param(std::string const& param_name, T const& default_value)
  {
    T the_value = default_value;

    // Locate the params block and find parameter within in if present
    rapidjson::Value::ConstMemberIterator itr = doc_.FindMember("params");
    if (itr != doc_.MemberEnd())
    {
      rapidjson::Value::ConstMemberIterator param_itr = itr->value.FindMember(param_name.c_str());
      if (param_itr != itr->value.MemberEnd())
      {
        the_value = get_value<T>(param_itr);
      }
    }

    return the_value;
  }

  //! Returns a vector of all parameter names contained in the message.
  //!
  //! \return A vector of all parameter names in this message
  std::vector<std::string> get_param_names() const;

  //! Returns true if the parameter is found within the message
  bool has_param(const std::string& param_name) const;

  //! Sets the value of a named parameter in the message.
  //!
  //! This template method sets the value of a named parameter in the message,
  //! creating that block and/orparameter if necessary.  Complex names can be
  //! supplied to generate complex parameter structures.
  //!
  //! \param param_name - string name of the parameter to set
  //! \param param_value - value of parameter to set

  template<typename T> void set_param(const std::string& param_name, T const& param_value)
  {
    bool found_array = false;
    std::vector<std::string> names;
    // Split the name by / character
    std::stringstream ss(param_name);
    std::string item;
    while (getline(ss, item, '/')) {
      names.push_back(item);
    }
    // Get the top level name
    std::string tl_param_name = names[0];
    names.erase(names.begin());

    // Pointer to the rapidjson value
    rapidjson::Value *next;
    // Check for array notation
    std::string array_end = "[]";
    if (0 == tl_param_name.compare(tl_param_name.length() - array_end.length(), array_end.length(), array_end)){
      found_array = true;
    }
    if (found_array){
      tl_param_name = tl_param_name.substr(0, tl_param_name.size()-2);
    }
    // Does the param exist
    if (!this->has_param(tl_param_name)){
      this->internal_set_param(tl_param_name, rapidjson::Value().SetObject());
    }
    // Get the reference to the parameter
    next = &(doc_["params"][rapidjson::Value().SetObject().SetString(tl_param_name.c_str(), doc_.GetAllocator())]);

    std::vector<std::string>::iterator iter;
    for (iter = names.begin(); iter != names.end(); ++iter){
      if (*iter == names.back()){
        // Check for array notation
        std::string array_end = "[]";
        if (0 == (*iter).compare((*iter).length() - array_end.length(), array_end.length(), array_end)){
          found_array = true;
        }
      }
      std::string index = *iter;
      if (found_array){
        index = index.substr(0, index.size()-2);
      }
      // For each entry check to see if it already exists
      rapidjson::Value::MemberIterator param_itr = next->FindMember(index.c_str());
      if (param_itr == next->MemberEnd()){
        // This one doesn't exist so create it
        rapidjson::Value newval;
        newval.SetObject();
        next->AddMember(rapidjson::Value().SetObject().SetString(index.c_str(), doc_.GetAllocator()), newval, doc_.GetAllocator());
      }
      next = &(*next)[index.c_str()];
    }
    if (!found_array){
      set_value(*next, param_value);
    } else {
      if (!next->IsArray()){
        next->SetArray();
      }
      rapidjson::Value val;
      set_value(val, param_value);
      next->PushBack(val, doc_.GetAllocator());
    }
  }

  //! Sets the nack message type with a reason parameter
  void set_nack(const std::string& reason);

  //! Indicates if message has necessary attributes with legal values
  bool is_valid(void);

  //! Returns type attribute of message
  const MsgType get_msg_type(void) const;

  //! Returns value attribute of message
  const MsgVal get_msg_val(void) const;

  //! Returns id attribute of message
  const unsigned int get_msg_id(void) const;

  //! Returns message timestamp as a string in ISO8601 extended format
  const std::string get_msg_timestamp(void) const;

  //! Returns message timstamp as tm structure
  const struct tm get_msg_datetime(void) const;

  //! Sets the message type attribute
  void set_msg_type(MsgType const msg_type);

  //! Sets the message value attribute
  void set_msg_val(MsgVal const msg_val);

  //! Sets the message id attribute
  void set_msg_id(unsigned int msg_id);

  //! Returns a JSON-encoded string of the message
  const char* encode(void);

  //! Returns a JSON-encoded string of the message parameters at a specified path
  const char* encode_params(const std::string& param_path = std::string());

  //! Copies parameters at a specified into the specified JSON value object
  void copy_params(rapidjson::Value& param_obj, const std::string& param_path = std::string());

  //! Overloaded equality relational operator
  friend bool operator ==(IpcMessage const& lhs_msg, IpcMessage const& rhs_msg);

  //! Overloaded inequality relational operator
  friend bool operator !=(IpcMessage const& lhs_msg, IpcMessage const& rhs_msg);

  //! Overloaded stream insertion operator
  friend std::ostream& operator <<(std::ostream& os, IpcMessage& the_msg);

private:

  //! Sets the value of a named parameter in the message.
  //!
  //! This template method sets the value of a named parameter in the message,
  //! creating that block and/orparameter if necessary.
  //!
  //! \param param_name - string name of the parameter to set
  //! \param param_value - value of parameter to set

  template<typename T> void internal_set_param(std::string const& param_name, T const& param_value)
  {
    rapidjson::Document::AllocatorType& allocator = doc_.GetAllocator();

    // Create the params block if it doesn't exist
    rapidjson::Value::ConstMemberIterator itr = doc_.FindMember("params");
    if (itr == doc_.MemberEnd())
    {
      rapidjson::Value params;
      params.SetObject();
      doc_.AddMember("params", params, allocator);
    }
    rapidjson::Value& params = doc_["params"];

    // Now search through the params block for the parameter, creating it if missing, and set
    // the value appropriately
    rapidjson::Value::MemberIterator param_itr = params.FindMember(param_name.c_str());
    if (param_itr == params.MemberEnd())
    {
      rapidjson::Value param_name_val(param_name.c_str(), allocator);
      rapidjson::Value param_value_val;
      set_value(param_value_val, param_value);
      params.AddMember(param_name_val, param_value_val, allocator);
    }
    else
    {
      set_value(const_cast<rapidjson::Value&>(param_itr->value), param_value);
    }
  }


  //! Gets the value of a message attribute.
  //!
  //! This private template method returns the value of a message attribute. If the
  //! attribute is mssing, an IpcMessageException is thrown
  //!
  //! \param attr_name - name of the attribte to return
  //! \return value of the attribute if present, otherwise an exception is thrown

  template<typename T> T get_attribute(std::string const& attr_name)
  {
    rapidjson::Value::ConstMemberIterator itr = doc_.FindMember(attr_name.c_str());

    if (itr == doc_.MemberEnd())
    {
      std::stringstream ss;
      ss << "Missing attribute " << attr_name;
      throw IpcMessageException(ss.str());
    }
    return get_value<T>(itr);
  }

  //! Gets the value of a message attribute.
  //!
  //! This private template method returns the value of a message attribute. If the
  //! attribute is mssing, the default value specified in the arguments is returned.
  //!
  //! \param attr_name - name of the attribute to return
  //! \param default_value - default value to return if attribute missing
  //! \return value of the attribute if present, otherwise an exception is thrown

  template<typename T> T get_attribute(std::string const& attr_name, T const& default_value)
  {
    rapidjson::Value::ConstMemberIterator itr = doc_.FindMember(attr_name.c_str());
    return itr == doc_.MemberEnd() ? default_value : this->get_value<T>(itr);
  }

  //! Sets the value of a message attribute.
  //!
  //! The private template method sets the value of a message attribute, creating the
  //! attribute if not already present in the message.
  //!
  //! \param attr_name - name of the attribute to set
  //! \param attr_value - value to set

  template<typename T> void set_attribute(std::string const& attr_name, T const& attr_value)
  {
    rapidjson::Document::AllocatorType& allocator = doc_.GetAllocator();

    // Search through the document for the attribute, creating it if missing, and set
    // the value appropriately
    rapidjson::Value::MemberIterator itr = doc_.FindMember(attr_name.c_str());
    if (itr == doc_.MemberEnd())
    {
      rapidjson::Value attr_name_val(attr_name.c_str(), allocator);
      rapidjson::Value attr_value_val;
      set_value(attr_value_val, attr_value);
      doc_.AddMember(attr_name_val, attr_value_val, allocator);
    }
    else
    {
      set_value(const_cast<rapidjson::Value&>(itr->value), attr_value);
    }
  }

  //! Gets the value of a message attribute.
  //!
  //! This private template method gets the value of a message attribute referenced by
  //! the iterator provided as an argument. Explicit specializations of this method
  //! are provided, mapping each of the RapidJSON attribute types onto the appropriate
  //! return value.
  //!
  //! \param itr - RapidJSON const member iterator referencing the attribute to access
  //! \return - value of the attribute, with the appropriate type

  template<typename T> T get_value(rapidjson::Value::ConstMemberIterator& itr) const;

  //! Sets the value of a message attribute.
  //!
  //! This private template method sets the value of a message attribute referenced by
  //! the RapidJSON value object passed as an argument. Explicit specialisations of this
  //! method are provided below, mapping the value type specified onto each of the
  //! RapidJSON atttribute types.
  //!
  //! \param value_obj - RapidJSON value object to set value of
  //! \param value - value to set

  template<typename T> void set_value(rapidjson::Value& value_obj, T const& value);

  //! Initialise the internal message type bidirectional map
  void msg_type_map_init();

  //! Maps a message type string to a valid enumerated MsgType
  MsgType valid_msg_type(std::string msg_type_name);

  //! Maps an enumerated MsgType message type to the equivalent string
  std::string valid_msg_type(MsgType msg_type);

  //! Initialise the internal message value bidirectional map
  void msg_val_map_init();

  //! Maps a message value string to a valid enumerated MsgVal
  MsgVal valid_msg_val(std::string msg_val_name);

  //! Maps an enumerated MsgVal message value to the equivalent string
  std::string valid_msg_val(MsgVal msg_val);

  //! Maps a message timestamp onto a the internal timestamp representation
  boost::posix_time::ptime valid_msg_timestamp(std::string msg_timestamp_text);

  //! Maps an internal message timestamp representation to an ISO8601 extended format string
  std::string valid_msg_timestamp(boost::posix_time::ptime msg_timestamp);

  //! Indicates if the message has a params block
  bool has_params(void) const;

  // Private member variables

  bool strict_validation_;                  //!< Strict validation enabled flag
  rapidjson::Document doc_;                 //!< RapidJSON document object
  MsgType msg_type_;                        //!< Message type attribute
  MsgVal msg_val_;                          //!< Message value attribute
  boost::posix_time::ptime msg_timestamp_;  //!< Message timestamp (internal representation)
  unsigned int msg_id_;                     //!< Message id attribute

  rapidjson::StringBuffer encode_buffer_;   //!< Encoding buffer used to encode message to JSON string
  static MsgTypeMap msg_type_map_;          //!< Bi-directional message type map
  static MsgValMap msg_val_map_;            //!< Bi-directional message value map

}; // IpcMessage


} // namespace OdinData
#endif /* IPCMESSAGE_H_ */

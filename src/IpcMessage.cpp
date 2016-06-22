/*
 * IpcMessage.cpp
 *
 *  Created on: Feb 9, 2015
 *      Author: tcn45
 */

#include "IpcMessage.h"


namespace FrameReceiver {

    //! Default constructor - initialises all attributes.
    //!
    //! This consructs an empty IPC message object with initialised, but invalid, attributes and an
    //! empty parameter block. The message can be subsequently populated with valid contents through
    //! calls to the various setter methods provided.
    //!
    //! \param msg_type           - MsgType enumerated message type (default: MsgTypeIllegal)
    //! \param msg_val            - MsgVal enumerated message value (default: MsgValIllegal)
    //! \param strict_validation  - Enforces strict validation of the message contents during subsequent
    //!                             setter calls (default: True)

    IpcMessage::IpcMessage(MsgType msg_type, MsgVal msg_val, bool strict_validation) :
        strict_validation_(strict_validation),
        msg_type_(msg_type),
        msg_val_(msg_val),
        msg_timestamp_(boost::posix_time::microsec_clock::local_time())
    {
        // Intialise empty JSON document
        doc_.SetObject();

        // Create the required params block
        rapidjson::Value params;
        params.SetObject();
        doc_.AddMember("params", params, doc_.GetAllocator());
    };

    //! Constructor taking JSON-formatted text message as argument.
    //!
    //! This constructor takes a JSON-formatted string as an argument to construct a message
    //! based on its contents. If the string is not valid JSON syntax, an IpcMessageException
    //! will be thrown. If strict validation is enabled, an IpcMessgeException will be
    //! thrown if any of the message attributes do not have valid values.
    //!
    //! \param json_msg          - JSON-formatted string containing the message to parse
    //! \param strict_validation - Enforces strict validation of the message contents during subsequent
    //!                            setter calls (default: True)

    IpcMessage::IpcMessage(const char* json_msg, bool strict_validation) :
        strict_validation_(strict_validation)
    {

        // Parse the message, catching any unexpected exceptions from rapidjson
        try {
            doc_.Parse(json_msg);
        }
        catch (...)
        {
            throw FrameReceiver::IpcMessageException("Unknown exception caught during parsing message");
        }

        // Test if the message parsed correctly, otherwise throw an exception
        if (doc_.HasParseError())
        {
            std::stringstream ss;
            ss << "JSON parse error creating message from string at offset " << doc_.GetErrorOffset() ;
            ss << " : " << rapidjson::GetParseError_En(doc_.GetParseError());
            throw FrameReceiver::IpcMessageException(ss.str());
        }

        // Extract required valid attributes from message. If strict validation is enabled, throw an
        // exception if any are illegal
        msg_type_ = valid_msg_type(get_attribute<std::string>("msg_type", "none"));
        if (strict_validation_ && (msg_type_ == MsgTypeIllegal))
        {
            throw FrameReceiver::IpcMessageException("Illegal or missing msg_type attribute in message");
        }

        msg_val_  = valid_msg_val(get_attribute<std::string>("msg_val", "none"));
        if (strict_validation_ && (msg_val_ == MsgValIllegal))
        {
            throw FrameReceiver::IpcMessageException("Illegal or missing msg_val attribute in message");
        }

        msg_timestamp_ = valid_msg_timestamp(get_attribute<std::string>("timestamp", "none"));
        if (strict_validation_ && (msg_timestamp_ == boost::posix_time::not_a_date_time))
        {
            throw FrameReceiver::IpcMessageException("Illegal or missing timestamp attribute in message");
        }

        // Check if a params block is present. If strict validation is enabled, thrown an exception if
        // absent.
        if (strict_validation && !has_params())
        {
            throw FrameReceiver::IpcMessageException("Missing params block in message");
        }

    }

    //! Constructor taking rapidJSON value as argument.
    //!
    //! This constructor takes a rapidJSON value as an argument to construct a message
    //! based on its contents.
    //!
    //! \param value             - rapidJSON value to set as the parameters
    //! \param msg_type          - MsgType enumerated message type (default: MsgTypeIllegal)
    //! \param msg_val           - MsgVal enumerated message value (default: MsgValIllegal)
    //! \param strict_validation - Enforces strict validation of the message contents during subsequent
    //!                            setter calls (default: True)

    IpcMessage::IpcMessage(const rapidjson::Value& value,
                           MsgType msg_type,
                           MsgVal msg_val,
                           bool strict_validation) :
      strict_validation_(strict_validation),
      msg_type_(msg_type),
      msg_val_(msg_val),
      msg_timestamp_(boost::posix_time::microsec_clock::local_time())
    {
      // Intialise empty JSON document
      doc_.SetObject();

      // Make a deep copy of the value
      rapidjson::Value newValue(value, doc_.GetAllocator());
      // Create the required params block
      doc_.AddMember("params", newValue, doc_.GetAllocator());
    }

    //! Searches for the named parameter in the message.
    //!
    //! This method returns true if the parameter is found in the message, or false
    //! if the block or parameter is missing
    //!
    //! \param param_name - string name of the parameter to return
    //! \return true if the parameter is present, otherwise false

    bool IpcMessage::has_param(const std::string& param_name) const
    {
      bool param_found = true;

      // Locate the params block
      rapidjson::Value::ConstMemberIterator itr = doc_.FindMember("params");
      if (itr == doc_.MemberEnd())
      {
        // No params block so we will not find the parameter
        param_found = false;
      } else {
        // Attempt to locate parameter within block
        rapidjson::Value::ConstMemberIterator param_itr = itr->value.FindMember(param_name.c_str());
        if (param_itr == itr->value.MemberEnd())
        {
          // Couldn't find the parameter
          param_found = false;
        }
      }
      return param_found;
    }

    //! Indicates if message has necessary attributes with legal values.
    //!
    //! This method indicates if the message is valid, i.e. that all required attributes
    //! have legal values and that a parameter block is present
    //!
    //! \return bool value, true if the emssage is valid, false otherwise

    bool IpcMessage::is_valid(void)
    {
        return ((msg_type_ != MsgTypeIllegal) &
                (msg_val_ != MsgValIllegal) &
                (msg_timestamp_ != boost::posix_time::not_a_date_time) &
                has_params());
    }

    //! Returns type attribute of message.
    //!
    //! This method returns the "msg_type" type attribute of the message.
    //!
    //! \return MsgType enumerated message type attribute

    const IpcMessage::MsgType IpcMessage::get_msg_type(void) const
    {
        return msg_type_;
    }

    //! Returns value attribute of message.
    //!
    //! This method returns the "msg_val" value attribute of the message.
    //!
    //! \return MsgVal enumerated message value attribute

    const IpcMessage::MsgVal IpcMessage::get_msg_val(void) const
    {
        return msg_val_;
    }

    //! Returns message timestamp as a string in ISO8601 extended format.
    //!
    //! This method returns the message timestamp (generated either at message creation or
    //! from the parsed content of a JSON message). The message is returned as a string
    //! in ISO8601 extended format
    //!
    //! \return std::string message timestamp in ISO8601 extended format

    const std::string IpcMessage::get_msg_timestamp(void) const
    {
        return boost::posix_time::to_iso_extended_string(msg_timestamp_);
    }

    //! Returns message timstamp as tm structure.
    //!
    //! This method returns a standard tm struture derived from the message timestamp.
    //!
    //! \return tm struct containing the message timestamp
    const struct tm IpcMessage::get_msg_datetime(void) const
    {
        return boost::posix_time::to_tm(msg_timestamp_);
    }


    //! Sets the message type attribute.
    //!
    //! This method sets the type attribute of the message. If strict validation is enabled
    //! an IpcMessageException will be thrown if the type is illegal.
    //!
    //! \param msg_type - MsgType enumerated message type

    void IpcMessage::set_msg_type(IpcMessage::MsgType const msg_type)
    {
        // TODO validation check
        msg_type_ = msg_type;
    }

    //! Sets the message type attribute.
    //!
    //! This method sets the value attribute of the message. If strict validation is enabled
    //! an IpcMessageException will be thrown if the value is illegal.
    //!
    //! \param msg_val - MsgVal enumerated message value

    void IpcMessage::set_msg_val(IpcMessage::MsgVal const msg_val)
    {
        // TODO validation check
        msg_val_ = msg_val;

    }

    //! Returns a JSON-encoded string of the message.
    //!
    //! This method returns a JSON-encoded string version of the message, intended for
    //! transmission across an IPC message channel.
    //!
    //! \return JSON encoded message as a null-terminated, string character array

    const char* IpcMessage::encode(void)
    {

        // Copy the validated attributes into the JSON document ready for encoding
        set_attribute("msg_type", valid_msg_type(msg_type_));
        set_attribute("msg_val", valid_msg_val(msg_val_));
        set_attribute("timestamp", valid_msg_timestamp(msg_timestamp_));

        // Clear the encoded output buffer otherwise successive encode() calls append
        // the message to the buffer
        encode_buffer_.Clear();

        // Create a writer and associate with the document
        rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF8<> > writer(encode_buffer_);
        doc_.Accept(writer);

        // Return the encoded buffer string
        return encode_buffer_.GetString();
    }

    //! Overloaded equality relational operator.
    //!
    //! This function overloads the equality relational operator, allowing two
    //! messages to be compared for the same content. All fields in the messages
    //! are compared and tested for equality
    //!
    //! \param lhs_msg - left-hand side message for comparison
    //! \param rhs_msg - right-hand side message for comparison
    //! \return boolean indicating equality

    bool operator ==(IpcMessage const& lhs_msg, IpcMessage const& rhs_msg)
    {
        bool areEqual = true;

        // Test equality of message attributes
        areEqual &= (lhs_msg.msg_type_ == rhs_msg.msg_type_);
        areEqual &= (lhs_msg.msg_val_  == rhs_msg.msg_val_);
        areEqual &= (lhs_msg.msg_timestamp_ == rhs_msg.msg_timestamp_);

        // Check both messages have a params block
        areEqual &= (lhs_msg.has_params() == rhs_msg.has_params());

        // Only continue to test equality if true at this point
        if (areEqual)
        {
            // Iterate over params block if present and test equality of all members
            if (lhs_msg.has_params() && rhs_msg.has_params())
            {
                rapidjson::Value const& lhs_params = lhs_msg.doc_["params"];
                rapidjson::Value const& rhs_params = rhs_msg.doc_["params"];

                areEqual &= (lhs_params.MemberCount() == rhs_params.MemberCount());

                for (rapidjson::Value::ConstMemberIterator lhs_itr = lhs_params.MemberBegin();
                        areEqual && (lhs_itr != lhs_params.MemberEnd()); lhs_itr++)
                {
                    rapidjson::Value::ConstMemberIterator rhs_itr = rhs_params.FindMember(lhs_itr->name.GetString());
                    if (rhs_itr != rhs_params.MemberEnd())
                    {
                        areEqual &= (lhs_itr->value == rhs_itr->value);
                    }
                    else
                    {
                        areEqual = false;
                    }
                }
            }
        }
        return areEqual;
    }

    //! Overloaded inequality relational operator.
    //!
    //! This function overloads the inequality relational operator, allowing two
    //! messages to be compared for the different content. All fields in the messages
    //! are compared and tested for inequality.
    //!
    //! \param lhs_msg - left-hand side message for comparison
    //! \param rhs_msg - righ-hand side message for comparison
    //! \return boolean indicating inequality

    bool operator !=(IpcMessage const& lhs_msg, IpcMessage const& rhs_msg)
    {
        return !(lhs_msg == rhs_msg);
    }

    //! Overloaded stream insertion operator.
    //!
    //! This function overloads the ostream insertion operator, allowing a message
    //! to be inserted into a ostream as a JSON-formatted string.
    //!
    //! \param os      - std::ostream to insert string into
    //! \param the_msg - reference to the IpcMessage object to insert

    std::ostream& operator <<(std::ostream& os, IpcMessage& the_msg)    {
        os << the_msg.encode();
        return os;
    }

    //! \privatesection

    //! Initialise the internal message type bidirectional map.
    //!
    //! This function initialises the interal message type bi-directional map for
    //! use in the valid_msg_type methods.

    void IpcMessage::msg_type_map_init()
    {
        msg_type_map_.insert(MsgTypeMapEntry("cmd",    MsgTypeCmd));
        msg_type_map_.insert(MsgTypeMapEntry("ack",    MsgTypeAck));
        msg_type_map_.insert(MsgTypeMapEntry("nack",   MsgTypeNack));
        msg_type_map_.insert(MsgTypeMapEntry("notify", MsgTypeNotify));
    }

    //! Maps a message type string to a valid enumerated MsgType.
    //!
    //! This private method maps a string message type to an valid enumerated
    //! MsgType. If the string is not valid, MsgTypeIllegal is returned
    //!
    //! \param msg_type_name - string message type
    //! \return MsgType value, MsgTypeIllegal if string is not a valid message type

    IpcMessage::MsgType IpcMessage::valid_msg_type(std::string msg_type_name)
    {

        MsgType msg_type = MsgTypeIllegal;
        if (msg_type_map_.size() == 0)
        {
            msg_type_map_init();
        }

        if (msg_type_map_.left.count(msg_type_name))
        {
            msg_type = msg_type_map_.left.at(msg_type_name);
        }

        return msg_type;
    }

    //! Maps an enumerated MsgType message type to the equivalent string.
    //!
    //! This private method maps an enumnerated MsgType message type to the
    //! equivalent string. If the type is no valid, "illegal" is returned.
    //!
    //! \param msg_type - enumerated MsgType message type
    //! \return string containing the message type

    std::string IpcMessage::valid_msg_type(IpcMessage::MsgType msg_type)
    {
        std::string msg_type_name = "illegal";
        if (msg_type_map_.size() == 0)
        {
            msg_type_map_init();
        }

        if (msg_type_map_.right.count(msg_type))
        {
            msg_type_name = msg_type_map_.right.at(msg_type);
        }

        return msg_type_name;
    }

    //! Initialise the internal message value bidirectional map.
    //!
    //! This function initialises the interal message value bi-directional map for
    //! use in the valid_msg_val methods.

    void IpcMessage::msg_val_map_init()
    {
        msg_val_map_.insert(MsgValMapEntry("reset",         MsgValCmdReset));
        msg_val_map_.insert(MsgValMapEntry("status",        MsgValCmdStatus));
        msg_val_map_.insert(MsgValMapEntry("configure",     MsgValCmdConfigure));
        msg_val_map_.insert(MsgValMapEntry("frame_ready",   MsgValNotifyFrameReady));
        msg_val_map_.insert(MsgValMapEntry("frame_release", MsgValNotifyFrameRelease));
    }

    //! Maps a message value string to a valid enumerated MsgVal.
    //!
    //! This private method maps a string message value to an valid enumerated
    //! MsgVal. If the string is not valid, MsgValIllegal is returned.
    //!
    //! \param msg_val_name - string message value
    //! \return MsgVal value, MsgValIllegal if string is not a valid message value

    IpcMessage::MsgVal IpcMessage::valid_msg_val(std::string msg_val_name)
    {
        MsgVal msg_val = MsgValIllegal;

        if (msg_val_map_.size() == 0)
        {
            msg_val_map_init();
        }

        if (msg_val_map_.left.count(msg_val_name))
        {
            msg_val = msg_val_map_.left.at(msg_val_name);
        }
        return msg_val;
    }

    //! Maps an enumerated MsgVal message value to the equivalent string.
    //!
    //! This private method maps an enumnerated MsgVal message value to the
    //! equivalent string. If the value is no valid, "illegal" is returned.
    //!
    //! \param msg_val - enumerated MsgVal message value
    //! \return string containing the message value

    std::string IpcMessage::valid_msg_val(IpcMessage::MsgVal msg_val)
    {
        std::string msg_val_name = "illegal";
        if (msg_val_map_.size() == 0)
        {
            msg_val_map_init();
        }

        if (msg_val_map_.right.count(msg_val))
        {
            msg_val_name = msg_val_map_.right.at(msg_val);
        }

        return msg_val_name;
    }

    //! Maps a message timestamp onto a the internal timestamp representation.
    //!
    //! This private method maps a valid, ISO8601 extended format timestamp string
    //! to the internal representation. If the string is not valid, the timestamp
    //! returned is set to the boost::posix::not_a_date_time value.
    //!
    //! \param msg_timestamp_text message timestamp string in ISO8601 extneded format
    //! \return boost::posix::ptime internal timestamp representation

    boost::posix_time::ptime IpcMessage::valid_msg_timestamp(std::string msg_timestamp_text)
    {

        boost::posix_time::ptime pt(boost::posix_time::not_a_date_time);

        try {
            pt = boost::date_time::parse_delimited_time<boost::posix_time::ptime>(msg_timestamp_text, 'T');
        }
        catch (...)
        {
            // Do nothing if parse exception occurred - return value of not_a_date_time indicates error
        }
        return pt;
    }

    //! Maps an internal message timestamp representation to an ISO8601 extended format string.
    //!
    //! This private method maps the internal boost::posix_time::ptime timestamp representation
    //! to an ISO8601 extended format string, as used in the encoded JSON message.
    //!
    //! \param msg_timestamp - internal boost::posix_time::ptime timestamp representation
    //! return ISO8601 extended format timestamp string

    std::string IpcMessage::valid_msg_timestamp(boost::posix_time::ptime msg_timestamp)
    {
        // Return message timestamp as string in ISO8601 extended format
        return boost::posix_time::to_iso_extended_string(msg_timestamp_);
    }

    //! Indicates if the message has a params block.
    //!
    //! This private method indicates if the message has a valid params block, which may be
    //! empty but is required for valid message syntax.
    //!
    //! \return boolean value, true if message has valid params block

    bool IpcMessage::has_params(void) const
    {
        bool has_params = false;
        rapidjson::Value::ConstMemberIterator itr = doc_.FindMember("params");
        if (itr != doc_.MemberEnd())
        {
            has_params = itr->value.IsObject();
        }
        return has_params;
    }

    // Explicit specialisations of the the get_value method, mapping native attribute types to the
    // appropriate RapidJSON storage type.

    template<> int IpcMessage::get_value(rapidjson::Value::ConstMemberIterator& itr)
    {
        return itr->value.GetInt();
    }

    template<> unsigned int IpcMessage::get_value(rapidjson::Value::ConstMemberIterator& itr)
    {
        return itr->value.GetUint();
    }

    template<> int64_t IpcMessage::get_value(rapidjson::Value::ConstMemberIterator& itr)
    {
        return itr->value.GetInt64();
    }

    template<> uint64_t IpcMessage::get_value(rapidjson::Value::ConstMemberIterator& itr)
    {
        return itr->value.GetUint64();
    }

    template<> double IpcMessage::get_value(rapidjson::Value::ConstMemberIterator& itr)
    {
        return itr->value.GetDouble();
    }

    template<> std::string IpcMessage::get_value(rapidjson::Value::ConstMemberIterator& itr)
    {
        return itr->value.GetString();
    }

    template<> bool IpcMessage::get_value(rapidjson::Value::ConstMemberIterator& itr)
    {
        return itr->value.GetBool();
    }

    template<> const rapidjson::Value& IpcMessage::get_value(rapidjson::Value::ConstMemberIterator& itr)
    {
        const rapidjson::Value& val = itr->value;
        return val;
    }

    // Explicit specialisations of the the set_value method, mapping  RapidJSON storage types
    // to the appropriate native type.

    //! Sets the value of a message attribute.
    //!
    //! This explicit specialisation of the private template method sets the value of a
    //! message attribute referenced by the RapidJSON value object passed as an argument.
    //!
    //! \param value_obj - RapidJSON value object to set value of
    //! \param value - integer value to set

    template<> void IpcMessage::set_value(rapidjson::Value& value_obj, int const& value)
    {
        value_obj.SetInt(value);
    }

    //! Sets the value of a message attribute.
    //!
    //! This explicit specialisation of the private template method sets the value of a
    //! message attribute referenced by the RapidJSON value object passed as an argument.
    //!
    //! \param value_obj - RapidJSON value object to set value of
    //! \param value - unsigned integer value to set

    template<> void IpcMessage::set_value(rapidjson::Value& value_obj, unsigned int const& value)
    {
        value_obj.SetUint(value);
    }

    //! Sets the value of a message attribute.
    //!
    //! This explicit specialisation of the private template method sets the value of a
    //! message attribute referenced by the RapidJSON value object passed as an argument.
    //!
    //! \param value_obj - RapidJSON value object to set value of
    //! \param value - int64_t value to set


    template<> void IpcMessage::set_value(rapidjson::Value& value_obj, int64_t const& value)
    {
        value_obj.SetInt64(value);
    }

    //! Sets the value of a message attribute.
    //!
    //! This explicit specialisation of the private template method sets the value of a
    //! message attribute referenced by the RapidJSON value object passed as an argument.
    //!
    //! \param value_obj - RapidJSON value object to set value of
    //! \param value - uint64_t value to set

    template<> void IpcMessage::set_value(rapidjson::Value& value_obj, uint64_t const& value)
    {
        value_obj.SetUint64(value);
    }

    //! Sets the value of a message attribute.
    //!
    //! This explicit specialisation of the private template method sets the value of a
    //! message attribute referenced by the RapidJSON value object passed as an argument.
    //!
    //! \param value_obj - RapidJSON value object to set value of
    //! \param value - double value to set

    template<> void IpcMessage::set_value(rapidjson::Value& value_obj, double const& value)
    {
        value_obj.SetDouble(value);
    }

    //! Sets the value of a message attribute.
    //!
    //! This explicit specialisation of the private template method sets the value of a
    //! message attribute referenced by the RapidJSON value object passed as an argument.
    //!
    //! \param value_obj - RapidJSON value object to set value of
    //! \param value - string value to set

    template<> void IpcMessage::set_value(rapidjson::Value& value_obj, std::string const& value)
    {
        value_obj.SetString(value.c_str(), doc_.GetAllocator());
    }

    // Definition of static member variables used for type and value mapping
    IpcMessage::MsgTypeMap IpcMessage::msg_type_map_;
    IpcMessage::MsgValMap IpcMessage::msg_val_map_;

} // namespace FrameReceiver

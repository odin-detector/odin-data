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

namespace FrameReceiver
{
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

} // namespace FrameReceiver

// Override rapidsjon assertion mechanism before including appropriate headers
#define RAPIDJSON_ASSERT(x) if (!(x)) throw FrameReceiver::IpcMessageException("rapidjson assertion thrown");
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"


namespace FrameReceiver
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
			MsgValIllegal = -1,       //!< Illegal value
			MsgValCmdReset,           //!< Reset command message
			MsgValCmdStatus,          //!< Status command message
			MsgValNotifyFrameReady,   //!< Frame ready notification message
			MsgValNotifyFrameRelease, //!< Frame release notification message
		};

		//! Internal bi-directional mapping of message type from string to enumerated MsgType
		typedef boost::bimap<std::string, MsgType> MsgTypeMap;
		//! Internal bi-directional mapping of message type from string to enumerated MsgType
		typedef MsgTypeMap::value_type MsgTypeMapEntry;
		//! Internal bi-directional mapping of message value from string to enumerated MsgVal
		typedef boost::bimap<std::string, MsgVal> MsgValMap;
		//! Internal bi-directional mapping of message value from string to enumerated MsgVal
		typedef MsgValMap::value_type MsgValMapEntry;

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

		IpcMessage(MsgType msg_type=MsgTypeIllegal, MsgVal msg_val=MsgValIllegal, bool strict_validation=true) :
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

		IpcMessage(const char* json_msg, bool strict_validation=true) :
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

		//! Gets the value of a named parameter in the message.
		//!
		//! This template method returns the value of the specified parameter stored in the
		//! params block of the message. If the block or parameter is missing, an exception
		//! of type IpcMessageException is thrown.
		//!
		//! \param param_name - string name of the parameter to return
		//! \return The value of the parameter if present, otherwise an exception is thrown

		template<typename T> T get_param(std::string const& param_name)
		{
			// Locate the params block and throw exception if absent
			rapidjson::Value::ConstMemberIterator itr = doc_.FindMember("params");
			if (itr == doc_.MemberEnd())
			{
				throw IpcMessageException("Missing params block in message");
			}

			// Locate parameter within block and throw exception if absent, otherwise return value
			rapidjson::Value::ConstMemberIterator param_itr = itr->value.FindMember(param_name.c_str());
			if (param_itr == itr->value.MemberEnd())
			{
				std::stringstream ss;
				ss << "Missing parameter " << param_name;
				throw IpcMessageException(ss.str());
			}
			return get_value<T>(param_itr);
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

		//! Sets the value of a named parameter in the message.
		//!
		//! This template method sets the value of a named parameter in the message,
		//! creating that block and/orparameter if necessary.
		//!
		//! \param param_name - string name of the parameter to set
		//! \param param_value - value of parameter to set

		template<typename T> void set_param(std::string const& param_name, T const& param_value)
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

		//! Indicates if message has necessary attributes with legal values.
		//!
		//! This method indicates if the message is valid, i.e. that all required attributes
		//! have legal values and that a parameter block is present
		//!
		//! \return bool value, true if the emssage is valid, false otherwise

		bool is_valid(void)
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

		const MsgType get_msg_type(void) const
		{
			return msg_type_;
		}

		//! Returns type attribute of message.
		//!
		//! This method returns the "msg_val" value attribute of the message.
		//!
		//! \return MsgVal enumerated message value attribute

		const MsgVal get_msg_val(void) const
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

		const std::string get_msg_timestamp(void) const
		{
			return boost::posix_time::to_iso_extended_string(msg_timestamp_);
		}

		//! Returns message timstamp as tm structure
		//!
		//! This method returns a standard tm struture derived from the message timestamp.
		//!
		//! \return tm struct containing the message timestamp
		const struct tm get_msg_datetime(void) const
		{
			return boost::posix_time::to_tm(msg_timestamp_);
		}

		//! Sets the message type attribute
		//!
		//! This method sets the type attribute of the message. If strict validation is enabled
		//! an IpcMessageException will be thrown if the type is illegal.
		//!
		//! \param msg_type - MsgType enumerated message type

		void set_msg_type(MsgType const msg_type)
		{
			// TODO validation check
			msg_type_ = msg_type;
		}

		//! Sets the message type attribute
		//!
		//! This method sets the value attribute of the message. If strict validation is enabled
		//! an IpcMessageException will be thrown if the value is illegal.
		//!
		//! \param msg_val - MsgVal enumerated message value

		void set_msg_val(MsgVal const msg_val)
		{
			// TODO validation check
			msg_val_ = msg_val;

		}

		//! Returns a JSON-encoded string of the message
		//!
		//! This method returns a JSON-encoded string version of the message, intended for
		//! transmission across an IPC message channel.
		//!
		//! \return JSON encoded message as a null-terminated, string character array

		const char* encode(void)
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

		//! Overloaded equality relational operator
		friend bool operator ==(IpcMessage const& lhs_msg, IpcMessage const& rhs_msg);

		//! Overloaded inequality relational operator
		friend bool operator !=(IpcMessage const& lhs_msg, IpcMessage const& rhs_msg);

		//! Overloaded stream insertion operator
		friend std::ostream& operator <<(std::ostream& os, IpcMessage& the_msg);

	private:


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
		//! are provided below, mapping each of the RapidJSON attribute types onto the
		//! the appropriate return value.
		//!
		//! \param itr - RapidJSON const member iterator referencing the attribute to access
		//! \return - value of the attribute, with the appropriate type

		template<typename T> T get_value(rapidjson::Value::ConstMemberIterator& itr);

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

		//! Initialise the internal message type bidirectional map.
		//!
		//! This function initialises the interal message type bi-directional map for
		//! use in the valid_msg_type methods.

		void msg_type_map_init()
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

		MsgType valid_msg_type(std::string msg_type_name)
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

		std::string valid_msg_type(MsgType msg_type)
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

		void msg_val_map_init()
		{
			msg_val_map_.insert(MsgValMapEntry("reset",         MsgValCmdReset));
			msg_val_map_.insert(MsgValMapEntry("status",        MsgValCmdStatus));
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

		MsgVal valid_msg_val(std::string msg_val_name)
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

		std::string valid_msg_val(MsgVal msg_val)
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

		boost::posix_time::ptime valid_msg_timestamp(std::string msg_timestamp_text)
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

		std::string valid_msg_timestamp(boost::posix_time::ptime msg_timestamp)
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

		bool has_params(void) const
		{
			bool has_params = false;
			rapidjson::Value::ConstMemberIterator itr = doc_.FindMember("params");
			if (itr != doc_.MemberEnd())
			{
				has_params = itr->value.IsObject();
			}
			return has_params;
		}

		// Private member variables

		bool strict_validation_;                  //!< Strict validation enabled flag
		rapidjson::Document doc_;                 //!< RapidJSON document object
		MsgType msg_type_;                        //!< Message type attribute
		MsgVal msg_val_;                          //!< Message value attribute
		boost::posix_time::ptime msg_timestamp_;  //!< Message timestamp (internal representation)

		rapidjson::StringBuffer encode_buffer_;   //!< Encoding buffer used to encode message to JSON string
		static MsgTypeMap msg_type_map_;          //!< Bi-directional message type map
		static MsgValMap msg_val_map_;            //!< Bi-directional message value map

	}; // IpcMessage

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

	// Explicit specialisations of the the set_value method, mapping  RapidJSON storage types
	// to the appropriate native type.

	template<> void IpcMessage::set_value(rapidjson::Value& value_obj, int const& value)
	{
		value_obj.SetInt(value);
	}

	template<> void IpcMessage::set_value(rapidjson::Value& value_obj, unsigned int const& value)
	{
		value_obj.SetUint(value);
	}

	template<> void IpcMessage::set_value(rapidjson::Value& value_obj, int64_t const& value)
	{
		value_obj.SetInt64(value);
	}

	template<> void IpcMessage::set_value(rapidjson::Value& value_obj, uint64_t const& value)
	{
		value_obj.SetUint64(value);
	}

	template<> void IpcMessage::set_value(rapidjson::Value& value_obj, double const& value)
	{
		value_obj.SetDouble(value);
	}

	template<> void IpcMessage::set_value(rapidjson::Value& value_obj, std::string const& value)
	{
		value_obj.SetString(value.c_str(), doc_.GetAllocator());
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

	std::ostream& operator <<(std::ostream& os, IpcMessage& the_msg)	{
		os << the_msg.encode();
		return os;
	}

} // namespace frameReceiver

FrameReceiver::IpcMessage::MsgTypeMap FrameReceiver::IpcMessage::msg_type_map_;
FrameReceiver::IpcMessage::MsgValMap FrameReceiver::IpcMessage::msg_val_map_;

#endif /* IPCMESSAGE_H_ */

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

		IpcMessage(MsgType msg_type=MsgTypeIllegal, MsgVal msg_val=MsgValIllegal, bool strict_validation=true);

		IpcMessage(const char* json_msg, bool strict_validation=true);

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

		bool is_valid(void);

		const MsgType get_msg_type(void) const;
		const MsgVal get_msg_val(void) const;
		const std::string get_msg_timestamp(void) const;
		const struct tm get_msg_datetime(void) const;

		void set_msg_type(MsgType const msg_type);
		void set_msg_val(MsgVal const msg_val);
		const char* encode(void);

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
		//! are provided, mapping each of the RapidJSON attribute types onto the appropriate
		//! return value.
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

		void msg_type_map_init();
		MsgType valid_msg_type(std::string msg_type_name);
		std::string valid_msg_type(MsgType msg_type);
		void msg_val_map_init();
		MsgVal valid_msg_val(std::string msg_val_name);
		std::string valid_msg_val(MsgVal msg_val);
		boost::posix_time::ptime valid_msg_timestamp(std::string msg_timestamp_text);
		std::string valid_msg_timestamp(boost::posix_time::ptime msg_timestamp);
		bool has_params(void) const;

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


} // namespace frameReceiver


#endif /* IPCMESSAGE_H_ */

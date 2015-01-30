/*
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
	// IpcMessageException - custom exception class implementing "what" for error string
	class IpcMessageException : public std::exception
	{
	public:
		IpcMessageException(void) throw() :
			what_("")
	    { };
		IpcMessageException(const std::string what) throw() :
			what_(what)
		{};
		virtual const char* what(void) const throw()
		{
			return what_.c_str();
		};
		~IpcMessageException(void) throw() {};

	private:
		const std::string what_;
	};

} // namespace FrameReceiver

// Override rapidsjon assertion mechanism before including appropriate headers
#define RAPIDJSON_ASSERT(x) if (!(x)) throw FrameReceiver::IpcMessageException("rapidjson assertion thrown");
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"


namespace FrameReceiver
{
	class IpcMessage
	{

	public:

		enum MsgType {
			MsgTypeIllegal = -1,
			MsgTypeCmd,
			MsgTypeAck,
			MsgTypeNack,
			MsgTypeNotify,
		};

		enum MsgVal {
			MsgValIllegal = -1,
			MsgValCmdReset,
			MsgValCmdStatus,
			MsgValNotifyFrameReady,
			MsgValNotifyFrameRelease,
		};

		typedef boost::bimap<std::string, MsgType> MsgTypeMap;
		typedef MsgTypeMap::value_type MsgTypeMapEntry;
		typedef boost::bimap<std::string, MsgVal> MsgValMap;
		typedef MsgValMap::value_type MsgValMapEntry;

		// Default constructor - initialises all attributes
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

		// Constructor taking JSON-formatted text message as argument
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
				throw FrameReceiver::IpcMessageException("Illegal or missing timetamp attribute in message");
			}

			// Check if a params block is present. If strict validation is enabled, thrown an exception if
			// absent.
			if (strict_validation && !has_params())
			{
				throw FrameReceiver::IpcMessageException("Missing params block in message");
			}

		}

		template<typename T> T get_attribute(std::string const& attr_name);
		template<typename T> T get_attribute(std::string const& attr_name, T const& default_value);

		template<typename T> void set_attribute(std::string const& attr_name, T const& attr_value);

		template<typename T> T get_param(std::string const& param_name);
		template<typename T> T get_param(std::string const& param_name, T const& default_value);

		template<typename T> void set_param(std::string const& param_name, T const& param_value);

		bool is_valid(void)
		{
			return ((msg_type_ != MsgTypeIllegal) &
					(msg_val_ != MsgValIllegal) &
					(msg_timestamp_ != boost::posix_time::not_a_date_time) &
					has_params());
		}

		const MsgType get_msg_type(void) const
		{
			return msg_type_;
		}

		const MsgVal get_msg_val(void) const
		{
			return msg_val_;
		}

		const std::string get_msg_timestamp(void) const
		{
			return boost::posix_time::to_iso_extended_string(msg_timestamp_);
		}

		const struct tm get_msg_datetime(void) const
		{
			return boost::posix_time::to_tm(msg_timestamp_);
		}

		void set_msg_type(MsgType const msg_type)
		{
			// TODO validation check
			msg_type_ = msg_type;
		}

		void set_msg_val(MsgVal const msg_val)
		{
			// TODO validation check
			msg_val_ = msg_val;

		}
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

		friend bool operator ==(IpcMessage const& lhs_msg, IpcMessage const& rhs_msg);
		friend bool operator !=(IpcMessage const& lhs_msg, IpcMessage const& rhs_msg);
		friend std::ostream& operator <<(std::ostream& os, IpcMessage& the_msg);

	private:

		template<typename T> T get_value(rapidjson::Value::ConstMemberIterator& itr);
		template<typename T> void set_value(rapidjson::Value& value_obj, T const& value);

		void msg_type_map_init()
		{
			msg_type_map_.insert(MsgTypeMapEntry("cmd",    MsgTypeCmd));
			msg_type_map_.insert(MsgTypeMapEntry("ack",    MsgTypeAck));
			msg_type_map_.insert(MsgTypeMapEntry("nack",   MsgTypeNack));
			msg_type_map_.insert(MsgTypeMapEntry("notify", MsgTypeNotify));
		}

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

		void msg_val_map_init()
		{
			msg_val_map_.insert(MsgValMapEntry("reset",         MsgValCmdReset));
			msg_val_map_.insert(MsgValMapEntry("status",        MsgValCmdStatus));
			msg_val_map_.insert(MsgValMapEntry("frame_ready",   MsgValNotifyFrameReady));
			msg_val_map_.insert(MsgValMapEntry("frame_release", MsgValNotifyFrameRelease));
		}

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

		boost::posix_time::ptime valid_msg_timestamp(std::string msg_timestamp_text)
		{

			boost::posix_time::ptime pt(boost::posix_time::not_a_date_time);

			// Use Boost posix_time with input facet to support ISO8601 extended format
			boost::posix_time::time_input_facet* input_facet = new boost::posix_time::time_input_facet;
			input_facet->set_iso_extended_format();

			std::stringstream ss;
			ss.imbue(std::locale(ss.getloc(), input_facet));
			ss.str(msg_timestamp_text);
			ss >> pt;

			return pt;
		}

		std::string valid_msg_timestamp(boost::posix_time::ptime msg_timestamp)
		{
			// Return message timestamp as string in ISO8601 extended format
			return boost::posix_time::to_iso_extended_string(msg_timestamp_);
		}

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

		// Member variables
		bool strict_validation_;
		rapidjson::Document doc_;
		MsgType msg_type_;
		MsgVal msg_val_;
		boost::posix_time::ptime msg_timestamp_;

		rapidjson::StringBuffer encode_buffer_;
		static MsgTypeMap msg_type_map_;
		static MsgValMap msg_val_map_;
	};

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

	template<typename T> T IpcMessage::get_attribute(std::string const& attr_name)
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

	template<typename T> T IpcMessage::get_attribute(std::string const& attr_name, T const& default_value)
	{
		rapidjson::Value::ConstMemberIterator itr = doc_.FindMember(attr_name.c_str());
		return itr == doc_.MemberEnd() ? default_value : this->get_value<T>(itr);
	}

	template<typename T> void IpcMessage::set_attribute(std::string const& attr_name, T const& attr_value)
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

	template<typename T> T IpcMessage::get_param(std::string const& param_name)
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

	template<typename T> T IpcMessage::get_param(std::string const& param_name, T const& default_value)
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

	template<typename T> void IpcMessage::set_param(std::string const& param_name, T const& param_value)
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

	bool operator !=(IpcMessage const& lhs_msg, IpcMessage const& rhs_msg)
	{
		return !(lhs_msg == rhs_msg);
	}

	std::ostream& operator<<(std::ostream& os, IpcMessage& the_msg)	{
		os << the_msg.encode();
		return os;
	}

} // namespace frameReceiver

FrameReceiver::IpcMessage::MsgTypeMap FrameReceiver::IpcMessage::msg_type_map_;
FrameReceiver::IpcMessage::MsgValMap FrameReceiver::IpcMessage::msg_val_map_;

#endif /* IPCMESSAGE_H_ */

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

		// Default constructor leaving all attributes unset (but initialised)
		IpcMessage(bool strict_validation=true) :
			strict_validation_(strict_validation),
			msg_type_(MsgTypeIllegal),
			msg_val_(MsgValIllegal),
			msg_timestamp_(boost::posix_time::microsec_clock::local_time())
			{
				// Intialise empty JSON document
				doc_.SetObject();
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

		template<typename T> T get_param(std::string const& param_name);
		template<typename T> T get_param(std::string const& param_name, T const& default_value);

		void dumpTypes(void)
		{
			static const char* kTypeNames[] =
				{ "Null", "False", "True", "Object", "Array", "String", "Number" };
			for (rapidjson::Value::ConstMemberIterator itr = doc_.MemberBegin();
					itr != doc_.MemberEnd(); ++itr)
			{
				std::cout << "Type of member " << itr->name.GetString() << " is " << kTypeNames[itr->value.GetType()] << std::endl;
			}
		}

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

	private:

		template<typename T> T get_value(rapidjson::Value::ConstMemberIterator& itr);

		MsgType valid_msg_type(std::string msg_type_name)
		{
			MsgType msg_type = MsgTypeIllegal;
			static std::map<std::string, MsgType> msg_type_map;

			if (msg_type_map.empty())
			{
				msg_type_map["cmd"]    = MsgTypeCmd;
				msg_type_map["ack"]    = MsgTypeAck;
				msg_type_map["nack"]   = MsgTypeNack;
				msg_type_map["notify"] = MsgTypeNotify;
			}

			if (msg_type_map.count(msg_type_name))
			{
				msg_type = msg_type_map[msg_type_name];
			}
			return msg_type;

		}

		MsgVal valid_msg_val(std::string msg_val_name)
		{
			MsgVal msg_val = MsgValIllegal;
			static std::map<std::string, MsgVal> msg_val_map;

			if (msg_val_map.empty())
			{
				msg_val_map["reset"]        = MsgValCmdReset;
				msg_val_map["status"]       = MsgValCmdStatus;
				msg_val_map["frame_ready"]  = MsgValNotifyFrameReady;
				msg_val_map["frame_release"] = MsgValNotifyFrameRelease;
			}

			if (msg_val_map.count(msg_val_name))
			{
				msg_val = msg_val_map[msg_val_name];
			}
			return msg_val;
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

		bool has_params(void)
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

		std::vector<rapidjson::Value> msg_params_;

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

	template<typename T> T IpcMessage::get_param(std::string const& param_name)
	{
		// Locate the params block and throw exception if absent
		rapidjson::Value::ConstMemberIterator itr = doc_.FindMember("params");
		if (itr == doc_.MemberEnd())
		{
			throw IpcMessageException("Missing params block in message");
		}

		// Locate parameter withing block and throw execption if absent, otherwise return value
		const rapidjson::Value& params = doc_["params"];
		rapidjson::Value::ConstMemberIterator param_itr = params.FindMember(param_name.c_str());
		if (param_itr == params.MemberEnd())
		{
			std::stringstream ss;
			ss << "Missing parameter " << param_name;
			throw IpcMessageException(ss.str());
		}
		return get_value<T>(param_itr);
	}


} // namespace frameReceiver



#endif /* IPCMESSAGE_H_ */

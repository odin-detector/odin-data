/*!
 * ParamContainer.h - abstract parameter container class with JSON encoding/decoding
 *
 * Created on: Oct 7, 2021
 *     Author: Tim Nicholls, STFC Detector Systems Software Group
 */

#ifndef PARAMCONTAINER_H_
#define PARAMCONTAINER_H_

#include <map>
#include <vector>
#include <functional>

using namespace std::placeholders;

namespace OdinData
{

//! ParamContainerException - custom exception class for ParamContainer implementing "what" for
//! error string
class ParamContainerException : public std::exception
{
public:

  //! Create ParamContainerException with no message
  ParamContainerException(void) throw() :
      what_("")
  { };

  //! Create ParamContainerException with informational message
  ParamContainerException(const std::string what) throw() :
      what_(what)
  {};

  //! Return the content of the informational message
  virtual const char* what(void) const throw()
  {
    return what_.c_str();
  };

  //! Destructor
  ~ParamContainerException(void) throw() {};

private:

  // Member variables
  const std::string what_;  //!< Informational message about the exception

};

} // namespace OdinData

// Override RapidJSON assertion mechanism before including appropriate headers
#ifdef RAPIDJSON_ASSERT
#undef RAPIDJSON_ASSERT
#endif
#define RAPIDJSON_ASSERT(x) if (!(x)) throw OdinData::ParamContainerException("rapidjson assertion thrown");
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"

namespace OdinData
{

//! ParamContainer - parameter container with JSON encoding/decoding
class ParamContainer
{

    //! Parameter setter function type definition
    typedef std::function<void(rapidjson::Value&)> SetterFunc;
    //! Parameter setter function map type definition
    typedef std::map<std::string, SetterFunc> SetterFuncMap;
    //! Parameter getter function type definition
    typedef std::function<void(rapidjson::Value&)> GetterFunc;
    //! Parameter getter function map type definition
    typedef std::map<std::string, GetterFunc> GetterFuncMap;

    public:

        //! Use the RapidJSON Document and Value types throughout
        typedef rapidjson::Document Document;
        typedef rapidjson::Value Value;

        //! Default constructor
        ParamContainer() {};

        //! Copy constructor
        ParamContainer(const ParamContainer& container);

        //! Encode the parameter container to a JSON-formatted string
        std::string encode(void);

        //! Encode the parameter container into an existing document, using the specified path
        //! as a prefix for all parameter paths
        void encode(ParamContainer::Document& doc_obj, std::string prefix_path = std::string()) const;

        //! Update the values of parameters from the specified JSON-formatted string
        void update(std::string json);

        //! Update the values of the parameters from the specified JSON-formatted character array
        void update(const char* json);

        //! Update the values of the parameters from the specified parameter container
        void update(const ParamContainer& container);

        //! Update the values of the parameters from the specified value object
        void update(const ParamContainer::Value& value_obj);

        //! Update the values of the parameters from the specified JSON document
        void update(ParamContainer::Document& doc_obj);

    protected:

        //! Bind a parameter to a path in the container.
        //!
        //! This template method binds the specified parameter to the specified path in the
        //! container, allowing the value of the parameter to be read or updated from an appropriate
        //! JSON message.
        //!
        //! \param param - reference to the parameter to bind
        //! \param path - JSON pointer-like path to bind the parameter to

        template<typename T>
        void bind_param(T& param, const std::string& path)
        {
            // Bind the parameter into the setter function map
            setter_map_[path] = std::bind(
                &ParamContainer::param_set<T>, this, std::ref(param), _1
            );

            // Bind the parameter into the getter function map
            getter_map_[path] = std::bind(
                &ParamContainer::param_get<T>, this, std::ref(param), _1
            );

        }

        //! Bind a vector parameter to a path in the container.
        //!
        //! This template method binds the specified vector parameter of a given type to the
        //! specified path in the parameter container, allowing the value of the parameter to be
        //! read or updated from an appropriate JSON message.
        //!
        //! \param param - reference to the vector parameter to bind
        //! \param path - JSON pointer-like path to bind the parameter to

        template<typename T>
        void bind_vector_param(std::vector<T>& param, const std::string& path)
        {
            // Bind the vector parameter into the setter function map
            setter_map_[path] = std::bind(
                &ParamContainer::vector_param_set<T>, this, std::ref(param), _1
            );

            // Bind the vector parameter into the getter function map
            getter_map_[path] = std::bind(
                &ParamContainer::vector_param_get<T>, this, std::ref(param), _1
            );
        }

        //! Set the value of a parameter in the container.
        //!
        //! This template method sets the value of of a parameter in the container to the value
        //! given the JSON object passed as an argument. This method is bound into the setter
        //! function map of the container and invoked by the update method. The type-specific
        //! set_value method is used to update the parameter value from the JSON object.
        //!
        //! \param param - reference to the parameter to update
        //! \param value_obj - reference to a RapidJSON value object containing the value to set

        template<typename T>
        void param_set(T& param, rapidjson::Value& value_obj)
        {
            set_value<T>(param, value_obj);
        }

        //! Get the value of a parameter in the container.
        //!
        //! This template method gets the value of a parameter in the container, setting that value
        //! in the JSON object passed as an argument. This method is bound into the getter
        //! function map of the container and invoked by the encode method. The type-specific
        //! get_value method is used to udpate the JSON object with the parameter value.
        //!
        //! \param param - reference to the parameter to retrieve
        //! \param value_obj - reference to a RapidJSON value object to be set with the value

        template<typename T> const
        void param_get(T& param, rapidjson::Value& value_obj)
        {
            get_value<T>(param, value_obj);
        }

        //! Set the value of a vector parameter in the container.
        //!
        //! This template method sets the value of of a vector parameter in the container to the
        //! values given the JSON object passed as an argument. This method is bound into the setter
        //! function map of the container and invoked by the update method. The type-specific
        //! set_value method is used to update the parameter values from the JSON object.
        //!
        //! \param param - reference to the vector parameter to update
        //! \param value_obj - reference to a RapidJSON value object containing the value to set

       template<typename T>
        void vector_param_set(std::vector<T>& param, rapidjson::Value& value_obj)
        {
            // Clear the existing values in the parameter vector
            param.clear();

            // Iterate through the values of the parameter in the value object and add to the
            // parameter vector
            for (rapidjson::Value::ValueIterator itr = value_obj.Begin();
                itr != value_obj.End(); ++itr)
            {
                T val;
                set_value<T>(val, *itr);
                param.push_back(val);
            }
        }

        //! Get the value of a vector parameter in the container.
        //!
        //! This template method gets the value of a vector parameter in the container, setting
        //! those values in the JSON object passed as an argument. This method is bound into the
        //! getter function map of the container and invoked by the encode method. The type-specific
        //! get_value method is used to udpate the JSON object with the parameter values.
        //!
        //! \param param - reference to the vector parameter to retrieve
        //! \param value_obj - reference to a RapidJSON value object to be set with the values

        template<typename T>
        void vector_param_get(std::vector<T>& param, rapidjson::Value& value_obj)
        {
            // Create a new JSON array object to hold the parameter values using RapidJSON swap
            // semantics
            rapidjson::Document::AllocatorType& allocator = doc_.GetAllocator();
            rapidjson::Value vec_obj(rapidjson::kArrayType);
            value_obj.Swap(vec_obj);

            // Iterate through the values in the parameter vector and add to the value object
            for (typename std::vector<T>::iterator itr = param.begin(); itr != param.end(); ++itr)
            {
                rapidjson::Value val;
                get_value<T>(*itr, val);
                value_obj.PushBack(val, allocator);
            }
        }

    private:

        //! Bind parameters - virtual method which must be defined in derived classes
        virtual void bind_params(void) = 0;

        //! Set the value of a parameter.
        //!
        //! This private template method is used by the param_set method to set the value of a
        //! parameter from the specified JSON object. Explicit specialisations of this method
        //! are provided, mapping each of the RapidJSON types onto the appropriate parameter types,
        //! which is returned as a value.
        //!
        //! \param value_obj - reference to a RapidJSON value object containing the value
        //! \return the value of the appropriate type

        template<typename T> void set_value(T& param, rapidjson::Value& value_obj) const;

        //! Get the value of a parameter.
        //!
        //! This private template method is used by the param_get method to get the value of a
        //! parameter and place it in the specified JSON object. Explicit specialisations of this
        //! method are provided, mapping each of the parameter types on the appropriate RapidJSON
        //! types.
        //!
        //! \param param - reference to the parameter to get the value of
        //! \param value_obj - RapidJSON value object to update with the value of the parameter

        template<typename T> void get_value(T& param, rapidjson::Value& value_obj) const;

        //! Construct a valid JSON pointer path.
        //!
        //! This private utility method is used to construct a valid JSON pointer path, ensuring
        //! that the leading / prefix is present.
        //!
        //! \param path - reference to path string
        //! \return valid pointer path string

        inline const std::string pointer_path(const std::string& path) const
        {
            // Set the pointer to the specified path
            std::string pointer_path(path);

            // If the pointer is not prefixed with the required slash, insert it
            if (*pointer_path.begin() != '/')
            {
                pointer_path.insert(0, "/");
            }
            return pointer_path;
        }

        SetterFuncMap setter_map_;      //!< Paramter setter function map
        GetterFuncMap getter_map_;      //!< Parameter getter function map
        ParamContainer::Document doc_;  //!< JSON document object used for encoding
};

} // namespace OdinData

#endif // PARAMCONTAINER_H_

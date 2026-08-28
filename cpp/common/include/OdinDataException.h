/*
 * OdinDataException.h
 *
 *  Created on: Feb 10, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef ODINDATAEXCEPTION_H_
#define ODINDATAEXCEPTION_H_

#include <exception>
#include <string>

namespace OdinData {
//! OdinDataException - custom exception class implementing "what" for error string
class OdinDataException : public std::exception {
public:
    //! Create OdinDataException with no message
    OdinDataException(void) noexcept :
        what_("") { };

    //! Creates OdinDataException with informational message
    OdinDataException(std::string&& what) noexcept :
        what_(std::move(what)) { };

    //! Returns the content of the informational message
    virtual const char* what(void) const noexcept
    {
        return what_.c_str();
    };

    //! Destructor
    ~OdinDataException(void) noexcept { };

private:
    // Member variables
    const std::string what_; //!< Informational message about the exception
};

} // namespace OdinData
#endif /* ODINDATAEXCEPTION_H_ */

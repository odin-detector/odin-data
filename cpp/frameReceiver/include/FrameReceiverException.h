/*
 * FrameReceiverException.h - Frame receiver exception class
 *
 *  Created on: Oct 10, 2017
 *      Author: Tim Nicholls, STFC Application Engineering Gruop
 */

#ifndef FRAMERECEIVER_INCLUDE_FRAMERECEIVEREXCEPTION_H_
#define FRAMERECEIVER_INCLUDE_FRAMERECEIVEREXCEPTION_H_

#include <exception>
#include <string>

namespace FrameReceiver {

//! FrameReceiverException - custom exception class implementing "what" for error string
class FrameReceiverException : public std::exception {
public:
    //! Create FrameReceiverException with no message
    FrameReceiverException(void) noexcept :
        what_("") { };

    //! Creates FrameReceiverExcetpion with informational message
    FrameReceiverException(std::string&& what) noexcept :
        what_(what) { };

    //! Returns the content of the informational message
    const char* what(void) const noexcept override
    {
        return what_.c_str();
    };

    //! Destructor
    ~FrameReceiverException(void) noexcept override { };

private:
    // Member variables
    const std::string what_; //!< Informational message about the exception

}; // FrameReceiverException
}

#endif /* FRAMERECEIVER_INCLUDE_FRAMERECEIVEREXCEPTION_H_ */

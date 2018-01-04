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

namespace FrameReceiver
{

  //! FrameReceiverException - custom exception class implementing "what" for error string
  class FrameReceiverException : public std::exception
  {
  public:

    //! Create FrameReceiverException with no message
    FrameReceiverException(void) throw() :
        what_("")
    { };

    //! Creates FrameReceiverExcetpion with informational message
    FrameReceiverException(const std::string what) throw() :
        what_(what)
    {};

    //! Returns the content of the informational message
    virtual const char* what(void) const throw()
    {
      return what_.c_str();
    };

    //! Destructor
    ~FrameReceiverException(void) throw() {};

  private:

    // Member variables
    const std::string what_;  //!< Informational message about the exception

  }; // FrameReceiverException
}

#endif /* FRAMERECEIVER_INCLUDE_FRAMERECEIVEREXCEPTION_H_ */

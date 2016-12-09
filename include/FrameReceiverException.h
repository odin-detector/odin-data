/*
 * FrameReceiverException.h
 *
 *  Created on: Feb 10, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#ifndef FRAMERECEIVEREXCEPTION_H_
#define FRAMERECEIVEREXCEPTION_H_

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

        //! Creates FrameReceiverException with informational message
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

} // namespace FrameReceiver



#endif /* FRAMERECEIVEREXCEPTION_H_ */

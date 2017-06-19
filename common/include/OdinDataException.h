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

namespace OdinData
{
//! OdinDataException - custom exception class implementing "what" for error string
class OdinDataException : public std::exception
{
public:

  //! Create OdinDataException with no message
  OdinDataException(void) throw() :
      what_("")
  { };

  //! Creates OdinDataException with informational message
  OdinDataException(const std::string what) throw() :
      what_(what)
  {};

  //! Returns the content of the informational message
  virtual const char* what(void) const throw()
  {
      return what_.c_str();
  };

  //! Destructor
  ~OdinDataException(void) throw() {};

private:

  // Member variables
  const std::string what_;  //!< Informational message about the exception

};

} // namespace OdinData
#endif /* ODINDATAEXCEPTION_H_ */

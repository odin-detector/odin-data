#ifndef ODINDATA_LOGGING_H
#define ODINDATA_LOGGING_H

#include <pwd.h>
#include <cstring>
#include <unistd.h>

#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/xml/domconfigurator.h>
#include <log4cxx/mdc.h>

#include "logging.h"

using namespace log4cxx;
using namespace log4cxx::helpers;

#ifndef HOST_NAME_MAX
# if defined(_POSIX_HOST_NAME_MAX)
#  define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
# elif defined(MAXHOSTNAMELEN)
#  define HOST_NAME_MAX MAXHOSTNAMELEN
# endif
#endif /* HOST_NAME_MAX */

namespace OdinData {

std::string app_path = "";

/** Configure logging constants
 *
 * Call this only once per thread context.
 *
 * @param app_path Path to executable (use argv[0])
 */
void configure_logging_mdc(const char *app_path) {
  char hostname[HOST_NAME_MAX];
  ::gethostname(hostname, HOST_NAME_MAX);
  hostname[HOST_NAME_MAX - 1] = '\0';
  MDC::put("host", hostname);

  std::stringstream ss;
  ss << ::getpid();
  MDC::put("pid", ss.str());

  const char *app_name = strrchr(app_path, static_cast<int>('/'));
  if (app_name != NULL) {
    MDC::put("app", app_name + 1);
  } else {
    MDC::put("app", app_path);
  }

  char thread_name[256];
  thread_name[0] = '\0';
  thread_name[256 - 1] = '\0';
  ::pthread_getname_np(::pthread_self(), thread_name, 256);
  MDC::put("thread", thread_name);

  uid_t uid = ::geteuid();
  struct passwd *pw = ::getpwuid(uid);
  if (pw) {
    MDC::put("user", pw->pw_name);
  }
}

}
#endif //ODINDATA_LOGGING_H

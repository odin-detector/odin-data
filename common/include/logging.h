#ifndef ODINDATA_LOGGING_H
#define ODINDATA_LOGGING_H

#include <string>

namespace OdinData
{

extern std::string app_path;

/** Configure logging constants
 *
 * Call this only once per thread context.
 *
 * @param app_path Path to executable (use argv[0])
 */
void configure_logging_mdc(const char* app_path);

}
#endif //ODINDATA_LOGGING_H

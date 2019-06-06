#include "PropertyTreeUtility.h"

#include <boost/regex.hpp>
#include <utility>

#include <boost/foreach.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string/replace.hpp>

namespace FrameSimulatorTest {

    /** Replace environment variables ${EXAMPLE} in a string; not recursive
     * @param original - string to replace environment variables in
     */
    void PropertyTreeUtility::expandEnvVars(std::string& original) {

      boost::smatch str_matches;
      boost::regex regex("\\$\\{([^}]+)\\}");

      if(boost::regex_search(original, str_matches, regex)) {
        std::string environment_var = getenv(str_matches[1].str().c_str());
        boost::replace_all(original, str_matches[0].str(), environment_var);
      }

    }

    /** Convert ini file derived ptree contents from section to std::vector<std::string>
     *
     * @param ptree - boost::property_tree derived from ini file
     * @param section - [section] in ini file to extract args from
     * @param args - extracted std::vector<std::string> of positional args
     */
    void PropertyTreeUtility::ini_to_command_args(boost::property_tree::ptree &ptree, const std::string &section,
                                                  std::vector<std::string> &args) {

      BOOST_FOREACH(boost::property_tree::ptree::value_type &vc, ptree.get_child(section)) {
              args.push_back("--" + vc.first);
              PropertyTreeUtility::expandEnvVars(vc.second.data());
              args.push_back(vc.second.data());
            }

    }


}
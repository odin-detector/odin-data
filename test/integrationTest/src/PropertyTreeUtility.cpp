#include "PropertyTreeUtility.h"

#include <boost/regex.hpp>
#include <utility>
#include <ios>

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
        std::string environment_var_str = str_matches[1].str();
        if(!getenv(environment_var_str.c_str()))
          throw std::runtime_error("environment variable not defined " + environment_var_str);
        std::string environment_var = getenv(environment_var_str.c_str());
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
              if (vc.first == "json_file") {
                std::string json_file = vc.second.data();
                std::ifstream file(json_file.c_str());
                std::string json;
                std::string new_json;
                while(std::getline(file, json)) {
                  PropertyTreeUtility::expandEnvVars(json);
                  new_json += json;
                }
                std::ofstream updated_file;
                updated_file.open(json_file.c_str(), std::ios::trunc);
                updated_file << new_json;
                updated_file.close();
              }

            }
    }


}
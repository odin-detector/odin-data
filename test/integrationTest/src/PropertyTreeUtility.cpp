#include "PropertyTreeUtility.h"

#include <boost/foreach.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace FrameSimulatorTest {

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
              args.push_back(vc.second.data());
            }

    }


}
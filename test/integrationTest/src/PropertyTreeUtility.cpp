#include "PropertyTreeUtility.h"

#include <boost/foreach.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace FrameSimulatorTest {

    void PropertyTreeUtility::ini_to_command_args(boost::property_tree::ptree &ptree, const std::string &section,
                                             std::vector<std::string>& args) {

      BOOST_FOREACH(boost::property_tree::ptree::value_type &vc, ptree.get_child(section)) {
              args.push_back("--" + vc.first);
              args.push_back(vc.second.data());
            }

    }

    void PropertyTreeUtility::inisection_to_json(boost::property_tree::ptree &ptree, const std::string &section) {

      boost::property_tree::json_parser::write_json("test.json", ptree.get_child(section));

    }


}
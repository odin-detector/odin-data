#ifndef ODINDATA_PROPERTYTREEUTILITY_H
#define ODINDATA_PROPERTYTREEUTILITY_H

#include <boost/property_tree/ptree.hpp>
#include <vector>

namespace FrameSimulatorTest {

    class PropertyTreeUtility {

    public:

        static void ini_to_command_args(boost::property_tree::ptree &ptree, const std::string &section, std::vector<std::string>& args);
        static void inisection_to_json(boost::property_tree::ptree &ptree, const std::string &section);

    };

}

#endif //ODINDATA_PROPERTYTREEUTILITY_H

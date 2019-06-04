#ifndef ODINDATA_PROPERTYTREEUTILITY_H
#define ODINDATA_PROPERTYTREEUTILITY_H

#include <boost/property_tree/ptree.hpp>
#include <vector>

namespace FrameSimulatorTest {

    /**
     * Helper class for manipulating boost::property_tree::ptree
     *
     */
    class PropertyTreeUtility {

    public:

        /** Convert ini file derived ptree contents from section to std::vector<std::string> */
        static void ini_to_command_args(boost::property_tree::ptree &ptree, const std::string &section, std::vector<std::string>& args);

    };

}

#endif //ODINDATA_PROPERTYTREEUTILITY_H

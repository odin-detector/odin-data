#ifndef FRAMESIMULATOR_FRAMESIMULATORPLUGIN_H
#define FRAMESIMULATOR_FRAMESIMULATORPLUGIN_H

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include <vector>
#include <string>
#include <boost/program_options.hpp>
#include <optional>

#include "FrameSimulatorOptions.h"

#include "IVersionedObject.h"

namespace po = boost::program_options;

namespace FrameSimulator {

    /** Abstract plugin class
     *
     *  All frame simulator plugins must inherit from this class.
     *  Once an instance of a plugin is constructed the plugin options are populated
     *  from the command line program options with a call to 'setup'. Once setup successfully
     *  the simulation is executed with a call to 'simulate'.
     *
     *  'populate_options' is called by the main executable (FrameSimulatorApp.cpp) to determine
     *  the plugin options required
     */
    class FrameSimulatorPlugin : public OdinData::IVersionedObject {

    public:

        FrameSimulatorPlugin();

        virtual bool setup(const po::variables_map& vm);
        virtual void simulate() = 0;

        virtual void populate_options(po::options_description& config);

    protected:

        //Number of frames to replay; if simulator plugin has insufficient frames defined, available frames are cycled
        std::optional<int> replay_numframes_;
        // Time (in s) between frames during replay
        std::optional<float> frame_gap_secs_;

    private:

        /** Pointer to logger **/
        LoggerPtr logger_;

    };

}

#endif

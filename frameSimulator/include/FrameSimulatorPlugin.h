#ifndef FRAMESIMULATOR_FRAMESIMULATORPLUGIN_H
#define FRAMESIMULATOR_FRAMESIMULATORPLUGIN_H

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include <boost/program_options.hpp>
#include <boost/optional.hpp>

#include "FrameSimulatorOptions.h"

#include "IVersionedObject.h"

namespace po = boost::program_options;

namespace FrameSimulator {

    class FrameSimulatorPlugin : public OdinData::IVersionedObject {

    public:

        FrameSimulatorPlugin();

        virtual bool setup(const po::variables_map& vm);
        virtual void simulate() = 0;

        virtual void populate_options(po::options_description& config);

    protected:

        boost::optional<int> replay_frames;
        boost::optional<int> replay_interval;

    private:

        /** Pointer to logger **/
        LoggerPtr logger_;

    };

}

#endif
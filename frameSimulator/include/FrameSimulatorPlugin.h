#ifndef FRAMESIMULATOR_FRAMESIMULATORPLUGIN_H
#define FRAMESIMULATOR_FRAMESIMULATORPLUGIN_H

#include <boost/program_options.hpp>

#include "FrameSimulatorOptions.h"

#include "IVersionedObject.h"

namespace po = boost::program_options;

namespace FrameSimulator {

    class FrameSimulatorPlugin : public OdinData::IVersionedObject {

    public:

        FrameSimulatorPlugin();

        virtual bool setup(const po::variables_map& vm) = 0;
        virtual void simulate() = 0;

        virtual void populate_options(po::options_description& config);

    };

}

#endif
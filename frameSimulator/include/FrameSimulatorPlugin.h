#ifndef FRAMESIMULATOR_FRAMESIMULATORPLUGIN_H
#define FRAMESIMULATOR_FRAMESIMULATORPLUGIN_H

#include "FrameSimulatorOptions.h"

namespace FrameSimulator {

    class FrameSimulatorPlugin {

    public:

        FrameSimulatorPlugin();

        virtual bool setup(const FrameSimulatorOptions& opts) = 0;
        virtual void simulate() = 0;

    protected:

        FrameSimulatorOptions opts;

    };

}

#endif
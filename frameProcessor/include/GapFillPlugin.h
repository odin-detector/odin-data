/*
 * GapFillPlugin.h
 *
 *  Created on: 5 Feb 2019
 *      Author: Alan Greer
 */

#ifndef ODINDATA_GAPFILLPLUGIN_H
#define ODINDATA_GAPFILLPLUGIN_H

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;


#include "FrameProcessorPlugin.h"
#include "ClassLoader.h"

namespace FrameProcessor
{

    class GapFillPlugin : public FrameProcessorPlugin
    {

    public:

        GapFillPlugin();
        virtual ~GapFillPlugin();
        void process_frame(boost::shared_ptr<Frame> frame);
        bool configuration_valid(boost::shared_ptr<Frame> frame);
        boost::shared_ptr<Frame> insert_gaps(boost::shared_ptr<Frame> frame);
        void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
        int get_version_major();
        int get_version_minor();
        int get_version_patch();
        std::string get_version_short();
        std::string get_version_long();

        /*Config Names*/
        /** The required grid for the image [y, x]*/
        static const std::string CONFIG_GRID_SIZE;
        /** The chip size of the image in pixels [y, x]*/
        static const std::string CONFIG_CHIP_SIZE;
        /** The gaps to insert in the x grid direction (must be grid[x] + 1 in dimension*/
        static const std::string CONFIG_GRID_X_GAPS;
        /** The gaps to insert in the y grid direction (must be grid[y] + 1 in dimension*/
        static const std::string CONFIG_GRID_Y_GAPS;

    private:

        void requestConfiguration(OdinData::IpcMessage& reply);

        /** Pointer to logger */
        LoggerPtr logger_;

        std::vector<int> grid_;
        std::vector<int> chip_;
        std::vector<int> gaps_x_;
        std::vector<int> gaps_y_;

    };

}/* namespace FrameProcessor */

#endif //ODINDATA_GAPFILLPLUGIN_H

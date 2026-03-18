#pragma once

#include <boost/bind/bind.hpp>
#include <boost/test/unit_test.hpp>
#ifdef BOOST_HAS_PLACEHOLDERS
using namespace boost::placeholders;
#endif

#include <log4cxx/basicconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/logger.h>
#include <log4cxx/simplelayout.h>
#include <log4cxx/xml/domconfigurator.h>
using namespace log4cxx;
using namespace log4cxx::xml;

#include "DataBlockFrame.h"
#include "DebugLevelLogger.h"
#include "FileWriterPlugin.h"
#include "HDF5File.h"
#include "IpcChannel.h"

class GlobalConfig {
public:
    GlobalConfig() :
        metaRxChannel_(ZMQ_PULL)
    {
        // std::cout << "GlobalConfig constructor" << std::endl;
        //  Create a default simple console appender for log4cxx.
        consoleAppender = new ConsoleAppender(LayoutPtr(new SimpleLayout()));
        BasicConfigurator::configure(AppenderPtr(consoleAppender));
        Logger::getRootLogger()->setLevel(Level::getWarn());
        set_debug_level(3);
        metaRxChannel_.bind("inproc://meta_rx");
    }
    ~GlobalConfig() {
        // std::cout << "GlobalConfig constructor" << std::endl;
        // delete consoleAppender;
    };

private:
    ConsoleAppender* consoleAppender;
    OdinData::IpcChannel metaRxChannel_;
};

void dummy_callback(const std::string& msg)
{
}

class FileWriterPluginTestFixture {
public:
    FileWriterPluginTestFixture()
    {
        unsigned short img[12] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        dimensions_t img_dims(2);
        img_dims[0] = 3;
        img_dims[1] = 4;
        dimensions_t chunk_dims(3);
        chunk_dims[0] = 1;
        chunk_dims[1] = 3;
        chunk_dims[2] = 4;

        dset_def.name = "data";
        dset_def.num_frames = 10;
        dset_def.data_type = FrameProcessor::raw_16bit;
        dset_def.frame_dimensions = dimensions_t(2);
        dset_def.frame_dimensions[0] = 3;
        dset_def.frame_dimensions[1] = 4;
        dset_def.chunks = chunk_dims;
        dset_def.create_low_high_indexes = false;

        FrameProcessor::FrameMetaData frame_meta(
            7, "data", FrameProcessor::raw_16bit, "test", img_dims, FrameProcessor::no_compression
        );
        frame = boost::shared_ptr<FrameProcessor::DataBlockFrame>(
            new FrameProcessor::DataBlockFrame(frame_meta, static_cast<void*>(img), 24)
        );

        for (unsigned short i = 0; i < 10; i++) {
            FrameProcessor::FrameMetaData loop_frame_meta(
                i, "data", FrameProcessor::raw_16bit, "test", img_dims, FrameProcessor::no_compression
            );
            boost::shared_ptr<FrameProcessor::DataBlockFrame> tmp_frame(
                new FrameProcessor::DataBlockFrame(loop_frame_meta, static_cast<void*>(img), 24)
            );
            img[0] = i;
            frames.push_back(tmp_frame);
        }

        hdf5_error_definition.create_duration = 0;
        hdf5_error_definition.write_duration = 0;
        hdf5_error_definition.flush_duration = 0;
        hdf5_error_definition.close_duration = 0;
        hdf5_error_definition.callback = boost::bind(&dummy_callback, _1);
    }
    ~FileWriterPluginTestFixture()
    {
    }
    boost::shared_ptr<FrameProcessor::DataBlockFrame> frame;
    std::vector<boost::shared_ptr<FrameProcessor::DataBlockFrame>> frames;
    FrameProcessor::HDF5ErrorDefinition_t hdf5_error_definition;
    FrameProcessor::FileWriterPlugin fw;
    FrameProcessor::DatasetDefinition dset_def;
    FrameProcessor::HDF5CallDurations_t durations;
};

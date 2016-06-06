/*
 * testSocket.cpp
 *
 *  Created on: 25 May 2016
 *      Author: gnx91527
 */

#include <signal.h>
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
using namespace std;


#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
namespace po = boost::program_options;

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
using namespace rapidjson;

#include "zmq/zmq.hpp"

#include <JSONSubscriber.h>
#include "FileWriterController.h"
#include "SharedMemoryParser.h"
#include "SharedMemoryController.h"


int main(int argc, char** argv)
{
    int rc = 0;

    // Create a default basic logger configuration, which can be overridden by command-line option later
    BasicConfigurator::configure();

    LoggerPtr logger(Logger::getLogger("socketApp"));
//    po::variables_map vm;
//    parse_arguments(argc, argv, vm, logger);

    boost::shared_ptr<filewriter::FileWriterController> fwc;
    fwc = boost::shared_ptr<filewriter::FileWriterController>(new filewriter::FileWriterController());

    filewriter::JSONSubscriber sh("tcp://127.0.0.1:5003");
    sh.registerCallback(fwc);
    sh.subscribe();

    //fwc->loadPlugin("DummyPlugin", "/home/gnx91527/work/percival/framereceiver/build/lib/libDummyPlugin.so");
//    // Create shared memory parser
//    boost::shared_ptr<filewriter::SharedMemoryParser> smp;
//    smp = boost::shared_ptr<filewriter::SharedMemoryParser>(new filewriter::SharedMemoryParser("FrameReceiverBuffer"));

//    // create the zmq publisher
//    boost::shared_ptr<filewriter::JSONPublisher> frame_release_publisher;
//    frame_release_publisher = boost::shared_ptr<filewriter::JSONPublisher>(new filewriter::JSONPublisher("tcp://127.0.0.1:5002"));
//    frame_release_publisher->connect();

//    boost::shared_ptr<filewriter::SharedMemoryController> smc;
//    smc = boost::shared_ptr<filewriter::SharedMemoryController>(new filewriter::SharedMemoryController());
//    smc->setSharedMemoryParser(smp);
//    smc->setFrameReleasePublisher(frame_release_publisher);

//    filewriter::JSONSubscriber sh2("tcp://127.0.0.1:5001");
//    sh2.registerCallback(smc);
//    sh2.subscribe();



    sleep(60);
    return 0;
}

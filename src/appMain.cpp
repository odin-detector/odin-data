/*
 * appMain.cpp
 *
 *  Created on: Nov 5, 2014
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

//#include "TestThroughputMulti.h"
#include <signal.h>
#include <iostream>
#include <zmq.h>
#include "IpcMessage.h"

// Interrupt signal handler
//void intHandler(int sig)
//{
//	TestThroughputMulti::stop();
//}

// Main application entry point

int main(int argc, char** argv)
{
	int rc = 0;

    FrameReceiver::IpcMessage fromStr("{\"project\":\"rapidjson\",\"stars\":10}");

    std::cout << "fromStr key stars has value " << fromStr.get_attribute<int>("stars") << std::endl;
    std::cout << "fromStr key  project has value " << fromStr.get_attribute<std::string>("project") << std::endl;

    try {
    	std::cout << "What happens with missing value: " << fromStr.get_attribute<int>("missing") << std::endl;
    }
    catch (std::exception& e)
    {
    	std::cout << "Successfully caught exception on missing value" << std::endl;
    }

    std::cout << "What happens with missing value and default:" << fromStr.get_attribute<int>("missing", 3456) << std::endl;

    FrameReceiver::IpcMessage hasParams("{\"project\":\"hasParams\", \"params\": { \"param1\":12345, \"param2\" : \"second\" }}");

    std::cout << "hasParams key project has value " << hasParams.get_attribute<std::string>("project") << std::endl;
    hasParams.dumpTypes();


    // 2. Modify it by DOM.
//    Value& s = d["stars"];
//    s.SetInt(s.GetInt() + 1);
//
//    // 3. Stringify the DOM
//    StringBuffer buffer;
//    Writer<StringBuffer> writer(buffer);
//    d.Accept(writer);
//
//    // Output {"project":"rapidjson","stars":11}
//    std::cout << buffer.GetString() << std::endl;

	// Trap Ctrl-C and pass to TestThroughputMulti
//	signal(SIGINT, intHandler);

	// Create a default basic logger configuration, which can be overridden by command-line option later
	//BasicConfigurator::configure();

	// Create a TestThrougputMulti instance
	//TestThroughputMulti ttm;

	// Parse command line arguments and set up node configuration
	//rc = ttm.parseArguments(argc, argv);

	// Run the instance
	//ttm.run();


	return rc;

}

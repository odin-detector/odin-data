/**
framenotifier_app.cpp

  Created on: 13 Oct 2015
      Author: up45
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

#include "framenotifier_data.h"
#include "FileWriter.h"

void parse_arguments(int argc, char** argv, po::variables_map& vm, LoggerPtr& logger)
{
    try
    {
        std::string config_file;

        // Declare a group of options that will allowed only on the command line
        po::options_description generic("Generic options");
        generic.add_options()
                ("help,h",
                    "Print this help message")
                ("config,c",    po::value<string>(&config_file),
                    "Specify program configuration file")
                ;

        // Declare a group of options that will be allowed both on the command line
        // and in the configuration file
        po::options_description config("Configuration options");
        config.add_options()
                ("logconfig,l",  po::value<string>(),
                    "Set the log4cxx logging configuration file")
                ("ready,r",       po::value<std::string>()->default_value("tcp://127.0.0.1:5001"),
                    "Ready ZMQ endpoint from frameReceiver")
                ("release",       po::value<std::string>()->default_value("tcp://127.0.0.1:5002"),
                        "Release frame ZMQ endpoint from frameReceiver")
                ("frames,f",     po::value<unsigned int>()->default_value(1),
                    "Set the number of frames to be notified about before terminating")
                ("sharedbuf",    po::value<std::string>()->default_value("FrameReceiverBuffer"),
                    "Set the name of the shared memory frame buffer")
                ("output,o",     po::value<std::string>(),
                    "Name of HDF5 file to write frames to (default: no file writing)")
                ;

        // Group the variables for parsing at the command line and/or from the configuration file
        po::options_description cmdline_options;
        cmdline_options.add(generic).add(config);
        po::options_description config_file_options;
        config_file_options.add(config);

        // Parse the command line options
        po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
        po::notify(vm);

        // If the command-line help option was given, print help and exit
        if (vm.count("help"))
        {
            std::cout << "usage: frameReceiver [options]" << std::endl << std::endl;
            std::cout << cmdline_options << std::endl;
            exit(1);
        }

        // If the command line config option was given, parse the specified configuration
        // file for additional options. Note that boost::program_options gives precedence
        // to the first instance occurring. In this case, that implies that command-line
        // options have precedence over equivalent configuration file entries.
        if (vm.count("config"))
        {
            std::ifstream config_ifs(config_file.c_str());
            if (config_ifs)
            {
                LOG4CXX_DEBUG(logger, "Parsing configuration file " << config_file);
                po::store(po::parse_config_file(config_ifs, config_file_options, true), vm);
                po::notify(vm);
            }
            else
            {
                LOG4CXX_ERROR(logger, "Unable to open configuration file " << config_file << " for parsing");
                exit(1);
            }
        }

        if (vm.count("logconfig"))
        {
            PropertyConfigurator::configure(vm["logconfig"].as<string>());
            LOG4CXX_DEBUG(logger, "log4cxx config file is set to " << vm["logconfig"].as<string>());
        }

        if (vm.count("ready"))
        {
            LOG4CXX_DEBUG(logger, "Setting frame ready notification ZMQ address to " << vm["ready"].as<string>());
        }

        if (vm.count("release"))
        {
            LOG4CXX_DEBUG(logger, "Setting frame release notification ZMQ address to " << vm["release"].as<string>());
        }

        if (vm.count("frames"))
        {
            LOG4CXX_DEBUG(logger, "Setting number of frames to receive to " << vm["frames"].as<unsigned int>());
        }

        if (vm.count("output"))
        {
            LOG4CXX_DEBUG(logger, "Writing frames to file: " << vm["output"].as<string>());
        }

    }
    catch (po::unknown_option &e)
    {
        LOG4CXX_WARN(logger, "CLI parsing error: " << e.what() << ". Will carry on...");
    }
    catch (Exception &e)
    {
        LOG4CXX_FATAL(logger, "Got Log4CXX exception: " << e.what());
        throw;
    }
    catch (exception &e)
    {
        LOG4CXX_ERROR(logger, "Got exception:" << e.what());
        throw;
    }
    catch (...)
    {
        LOG4CXX_FATAL(logger, "Exception of unknown type!");
        throw;
    }
}

int main(int argc, char** argv)
{
    int rc = 0;

    // Create a default basic logger configuration, which can be overridden by command-line option later
    BasicConfigurator::configure();

    LoggerPtr logger(Logger::getLogger("FrameNotifier"));
    po::variables_map vm;
    parse_arguments(argc, argv, vm, logger);

    SharedMemParser smp(vm["sharedbuf"].as<string>());

    // Assuming this is a P2M, our image dimensions are:
    size_t bytes_per_pixel = 2;
    dimensions_t p2m_dims(2); p2m_dims[0] = 1484; p2m_dims[1] = 1408;

    zmq::context_t zmq_context; // Global ZMQ context

    // The "release" ZMQ PUB socket, used to release frames back to the frameReceiver
    zmq::socket_t zsocket_release(zmq_context, ZMQ_PUB);
    zsocket_release.connect(vm["release"].as<string>().c_str());

    // The "ready" ZMQ SUB socket, used to get notifications that new frames are ready to be processed
    zmq::socket_t zsocket_ready(zmq_context, ZMQ_SUB);
    zsocket_ready.connect(vm["ready"].as<string>().c_str());
    zsocket_ready.setsockopt(ZMQ_SUBSCRIBE, "", strlen(""));
    zmq::pollitem_t poll_item;
    poll_item.socket = zsocket_ready;
    poll_item.events = ZMQ_POLLIN;
    poll_item.fd = 0;
    poll_item.revents = 0;

    // The file writer object
    FileWriter hdfwr;
    bool write_file = false;
    if (vm.count("output")) {
        write_file = true;
        hdfwr.createFile(vm["output"].as<string>());
        FileWriter::DatasetDefinition dset_def;
        dset_def.name = "data";
        dset_def.frame_dimensions = p2m_dims;
        dset_def.pixel = FileWriter::pixel_raw_16bit;
        dset_def.num_frames = vm["frames"].as<unsigned int>();
        hdfwr.createDataset(dset_def);

        dset_def.name = "reset";
        hdfwr.createDataset(dset_def);
    }

    // Debug info
    LOG4CXX_DEBUG(logger, "data_type_size   = " << data_type_size);
    LOG4CXX_DEBUG(logger, "total_frame_size = " << total_frame_size);

    // The polling loop. Polls on all elements in poll_item
    // Stop the loop by setting keep_running=false
    // Loop automatically ends if notification_count > user option "frames"
    unsigned long notification_count = 0;
    bool keep_running = true;
    LOG4CXX_DEBUG(logger, "Entering ZMQ polling loop (" << vm["ready"].as<string>().c_str() << ")");
    while (keep_running)
    {
        // Always reset the last raw events before polling
        poll_item.revents = 0;

        // Do the poll with a 10ms timeout (i.e. 10Hz poll)
        zmq::poll(&poll_item, 1, 10);

        // Check for poll error and exit the loop is one is found
        if (poll_item.revents & ZMQ_POLLERR)
        {
            LOG4CXX_ERROR(logger, "Got ZMQ error in polling. Quitting polling loop.");
            keep_running = false;
        }
        // Do work if a message is received
        else if (poll_item.revents & ZMQ_POLLIN)
        {
            LOG4CXX_DEBUG(logger, "Reading data from ZMQ socket");
            notification_count++;
            zmq::message_t msg;
            zsocket_ready.recv(&msg); // Read the message from the ready socket

            string msg_str(reinterpret_cast<char*>(msg.data()), msg.size()-1);
            LOG4CXX_DEBUG(logger, "Parsing JSON msg string: " << msg_str);
            Document msg_doc;
            msg_doc.Parse(msg_str.c_str()); // Parse the JSON string into a Document DOM object

            // Setup a string buffer and a writer so we can stringify the Document DOM object again
            StringBuffer buffer;
            PrettyWriter<StringBuffer> writer(buffer);
            msg_doc.Accept(writer);
            LOG4CXX_DEBUG(logger, "Parsed json: " << buffer.GetString());

            // Copy the data out into a Frame object
            unsigned int buffer_id = msg_doc["params"]["buffer_id"].GetInt64();
            LOG4CXX_DEBUG(logger, "Creating Reset Frame object. buffer=" << buffer_id
                                << " buffer addr: " << smp.get_buffer_address(buffer_id));
            LOG4CXX_DEBUG(logger, "  Header addr: " << smp.get_frame_header_address(buffer_id)
                                << "  Data addr: " << smp.get_reset_data_address(buffer_id));
            LOG4CXX_DEBUG(logger, FrameHeaderToString(static_cast<const FrameHeader*>(smp.get_frame_header_address(buffer_id))));
            Frame reset_frame(bytes_per_pixel, p2m_dims);
            reset_frame.set_dataset_name("reset");
            smp.get_reset_frame(reset_frame, buffer_id);

            LOG4CXX_DEBUG(logger, "Creating Data Frame object. buffer=" << buffer_id);
            LOG4CXX_DEBUG(logger,"  Data addr: " << smp.get_frame_data_address(buffer_id));
            Frame frame(bytes_per_pixel, p2m_dims);
            frame.set_dataset_name("data");
            smp.get_frame(frame, buffer_id);

            // Clear the json string buffer and reset the writer so we can re-use them
            // after modifying the Document DOM object
            buffer.Clear();
            writer.Reset(buffer);

            // We want to return the exact same JSON message back to the frameReceiver
            // so we just modify the relevant bits: msg_val: from frame_ready to frame_release
            // and update the timestamp
            msg_doc["msg_val"].SetString("frame_release");

            // Create and update the timestamp in the JSON message
            boost::posix_time::ptime msg_timestamp = boost::posix_time::microsec_clock::local_time();
            string msg_timestamp_str = boost::posix_time::to_iso_extended_string(msg_timestamp);
            msg_doc["timestamp"].SetString(StringRef(msg_timestamp_str.c_str()));

            // Encode the JSON Document DOM object using the writer
            msg_doc.Accept(writer);
            LOG4CXX_DEBUG(logger, "Changing msg_val: " << msg_doc["msg_val"].GetString());
            string release_msg(buffer.GetString()); // Get a copy of the JSON string
            LOG4CXX_DEBUG(logger, "New json: " << release_msg);

            LOG4CXX_DEBUG(logger, "Sending release response");
            // Send the new JSON object (string) back to the frameReceiver on the "release" ZMQ socket
            size_t nbytes = zsocket_release.send(release_msg.c_str(), release_msg.size() + 1);
            LOG4CXX_DEBUG(logger, "Sent " << nbytes << " bytes");

            // Write the frame to disk
            if (write_file) hdfwr.writeFrame(frame);

        } else
        {
            // No new data
        }

        // Quit the loop if we have received the desired number of frames
        if (notification_count >= vm["frames"].as<unsigned int>()) keep_running=false;
    }
    if (write_file) hdfwr.closeFile();
    return rc;
}



/*!
 * FrameReceiverApp.cpp
 *
 *  Created on: Jan 28, 2015
 *      Author: Tim Nicholls, STFC Application Engineering Group
 */

#include "FrameReceiverApp.h"
#include "FrameReceiverConfig.h"
#include "SharedBufferManager.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <iterator>
#include <cstdlib>
using namespace std;

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
namespace po = boost::program_options;

using namespace FrameReceiver;

bool FrameReceiverApp::terminate_frame_receiver_ = false;

IMPLEMENT_DEBUG_LEVEL;

static bool has_suffix(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() &&
            str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

//! Constructor for FrameReceiverApp class.
//!
//! This constructor initialises the FrameRecevierApp instance

FrameReceiverApp::FrameReceiverApp(void) :
    rx_channel_(ZMQ_PAIR),
    ctrl_channel_(ZMQ_REP),
    frame_ready_channel_(ZMQ_PUB),
    frame_release_channel_(ZMQ_SUB),
    frames_received_(0),
    frames_released_(0)
{

	// Retrieve a logger instance
	logger_ = Logger::getLogger("FR.APP");

}

//! Destructor for FrameRecveiverApp class
FrameReceiverApp::~FrameReceiverApp()
{

    // Delete the RX thread object by resetting the scoped pointer, allowing the IPC channel
    // to be closed cleanly
    rx_thread_.reset();

}

//! Parse command-line arguments and configuration file options.
//!
//! This method parses command-line arguments and configuration file options
//! to configure the application for operation. Most options can either be
//! given at the command line or stored in an INI-formatted configuration file.
//! The configuration options are stored in the FrameReceiverConfig helper object for
//! retrieval throughout the application.
//!
//! \param argc - standard command-line argument count
//! \param argv - array of char command-line options
//! \return return code, 0 if OK, 1 if option parsing failed

int FrameReceiverApp::parse_arguments(int argc, char** argv)
{

	int rc = 0;
	try
	{
		std::string config_file;

		// Declare a group of options that will allowed only on the command line
		po::options_description generic("Generic options");
		generic.add_options()
				("help,h",
					"Print this help message")
				("version,v",
					"Print program version string")
				("config,c",    po::value<string>(&config_file),
					"Specify program configuration file")
				;

		// Declare a group of options that will be allowed both on the command line
		// and in the configuration file
		po::options_description config("Configuration options");
		config.add_options()
				("debug,d",      po::value<unsigned int>()->default_value(debug_level),
				    "Set the debug level")
				("node,n",       po::value<unsigned int>()->default_value(FrameReceiver::Defaults::default_node),
					"Set the frame receiver node ID")
				("logconfig,l",  po::value<string>(),
					"Set the log4cxx logging configuration file")
				("maxmem,m",     po::value<std::size_t>()->default_value(FrameReceiver::Defaults::default_max_buffer_mem),
					"Set the maximum amount of shared memory to allocate for frame buffers")
				("sensortype,s", po::value<std::string>()->default_value("unknown"),
					"Set the sensor type to receive frame data from")
				("port,p",       po::value<std::string>()->default_value(FrameReceiver::Defaults::default_rx_port_list),
                    "Set the port to receive frame data on")
                ("ipaddress,i",  po::value<std::string>()->default_value(FrameReceiver::Defaults::default_rx_address),
                    "Set the IP address of the interface to receive frame data on")
                ("sharedbuf",    po::value<std::string>()->default_value(FrameReceiver::Defaults::default_shared_buffer_name),
                    "Set the name of the shared memory frame buffer")
                ("frametimeout", po::value<unsigned int>()->default_value(FrameReceiver::Defaults::default_frame_timeout_ms),
                    "Set the incomplete frame timeout in ms")
                ("frames,f",     po::value<unsigned int>()->default_value(FrameReceiver::Defaults::default_frame_count),
                    "Set the number of frames to receive before terminating")
                ("packetlog",    po::value<bool>()->default_value(FrameReceiver::Defaults::default_enable_packet_logging),
                    "Enable logging of packet diagnostics to file")
				("rxbuffer",     po::value<unsigned int>()->default_value(FrameReceiver::Defaults::default_rx_recv_buffer_size),
					"Set UDP receive buffer size")
				;

		// Group the variables for parsing at the command line and/or from the configuration file
		po::options_description cmdline_options;
		cmdline_options.add(generic).add(config);
		po::options_description config_file_options;
		config_file_options.add(config);

		// Parse the command line options
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
		po::notify(vm);

		// If the command-line help option was given, print help and exit
		if (vm.count("help"))
		{
		    std::cout << "usage: frameReceiver [options]" << std::endl << std::endl;
			std::cout << cmdline_options << std::endl;
			return 1;
		}

		// If the command line version option was given, print version and exit
		if (vm.count("version"))
		{
			std::cout << "Will print version here" << std::endl;
			return 1;
		}

		if (vm.count("debug"))
		{
			set_debug_level(vm["debug"].as<unsigned int>());
			LOG4CXX_DEBUG_LEVEL(1, logger_, "Debug level set to  " << debug_level);
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
				LOG4CXX_DEBUG_LEVEL(1, logger_, "Parsing configuration file " << config_file);
				po::store(po::parse_config_file(config_ifs, config_file_options, true), vm);
				po::notify(vm);
			}
			else
			{
				LOG4CXX_ERROR(logger_, "Unable to open configuration file " << config_file << " for parsing");
				exit(1);
			}
		}

		if (vm.count("logconfig"))
		{
            std::string logconf_fname = vm["logconfig"].as<string>();
            if (has_suffix(logconf_fname, ".xml")) {
                log4cxx::xml::DOMConfigurator::configure(logconf_fname);
            } else {
                PropertyConfigurator::configure(logconf_fname);
            }
            LOG4CXX_DEBUG_LEVEL(1, logger_, "log4cxx config file is set to " << logconf_fname);
		}

		if (vm.count("maxmem"))
		{
		    config_.max_buffer_mem_ = vm["maxmem"].as<std::size_t>();
		    LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting frame buffer maximum memory size to " << config_.max_buffer_mem_);
		}

		if (vm.count("sensortype"))
		{
		    std::string sensor_name = vm["sensortype"].as<std::string>();
		    config_.sensor_type_ = config_.map_sensor_name_to_type(sensor_name);
		    LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting sensor type to " << sensor_name << " (" << config_.sensor_type_ << ")");
		}

		if (vm.count("port"))
		{
		    config_.rx_ports_.clear();
            config_.tokenize_port_list(config_.rx_ports_, vm["port"].as<std::string>());

		    std::stringstream ss;
            for (std::vector<uint16_t>::iterator itr = config_.rx_ports_.begin(); itr !=config_.rx_ports_.end(); itr++)
            {
                ss << *itr << " ";
            }
		    LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting RX port(s) to " << ss.str());
		}

		if (vm.count("ipaddress"))
		{
		    config_.rx_address_ = vm["ipaddress"].as<std::string>();
		    LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting RX interface address to " << config_.rx_address_);
		}

		if (vm.count("sharedbuf"))
		{
		    config_.shared_buffer_name_ = vm["sharedbuf"].as<std::string>();
		    LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting shared frame buffer name to " << config_.shared_buffer_name_);
		}

		if (vm.count("frametimeout"))
		{
		    config_.frame_timeout_ms_ = vm["frametimeout"].as<unsigned int>();
		    LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting incomplete frame timeout to " << config_.frame_timeout_ms_);
		}

		if (vm.count("frames"))
		{
		    config_.frame_count_ = vm["frames"].as<unsigned int>();
		    LOG4CXX_DEBUG_LEVEL(1, logger_, "Setting number of frames to receive to " << config_.frame_count_);
		}

		if (vm.count("packetlog"))
		{
		    config_.enable_packet_logging_ = vm["packetlog"].as<bool>();
		    LOG4CXX_DEBUG_LEVEL(1, logger_, "Packet diagnostic logging is " <<
		            (config_.enable_packet_logging_ ? "enabled" : "disabled"));
		}

		if (vm.count("rxbuffer"))
		{
			config_.rx_recv_buffer_size_ = vm["rxbuffer"].as<unsigned int>();
			LOG4CXX_DEBUG_LEVEL(1, logger_, "RX receive buffer size is " << config_.rx_recv_buffer_size_);
		}

	}
	catch (Exception &e)
	{
		LOG4CXX_ERROR(logger_, "Got Log4CXX exception: " << e.what());
		rc = 1;
	}
	catch (exception &e)
	{
		LOG4CXX_ERROR(logger_, "Got exception:" << e.what());
		rc = 1;
	}
	catch (...)
	{
		LOG4CXX_ERROR(logger_, "Exception of unknown type!");
		rc = 1;
	}

	return rc;

}

void FrameReceiverApp::run(void)
{

    terminate_frame_receiver_ = false;
    LOG4CXX_INFO(logger_,  "Running frame receiver");

    try {

        // Initialise IPC channels
        initialise_ipc_channels();

        // Add timers to the reactor
        //int rxPingTimer = reactor_.add_timer(1000, 0, boost::bind(&FrameReceiverApp::rxPingTimerHandler, this));
        //int timer2 = reactor_.add_timer(1500, 0, boost::bind(&FrameReceiverApp::timerHandler2, this));

        // Create the appropriate frame decoder
        initialise_frame_decoder();

        // Initialise the frame buffer buffer manager
        initialise_buffer_manager();

        // Create the RX thread object
        rx_thread_.reset(new FrameReceiverRxThread( config_, logger_, buffer_manager_, frame_decoder_));

        // Pre-charge all frame buffers onto the RX thread queue ready for use
        precharge_buffers();

        LOG4CXX_DEBUG_LEVEL(1, logger_, "Main thread entering reactor loop");

        // Run the reactor event loop
        reactor_.run();

        // Destroy the RX thread
        rx_thread_.reset();

        // Clean up IPC channels
        cleanup_ipc_channels();

        // Remove timers and channels from the reactor
        //reactor_.remove_timer(rxPingTimer);
        //reactor_.remove_timer(timer2);

    }
    catch (FrameReceiverException& e)
    {
        LOG4CXX_ERROR(logger_, "Frame receiver run failed: " << e.what());
    }
    catch (...)
    {
        LOG4CXX_ERROR(logger_, "Unexpected exception during frame receiver run");
    }
}

void FrameReceiverApp::stop(void)
{
    terminate_frame_receiver_ = true;
}

void FrameReceiverApp::initialise_ipc_channels(void)
{
    // Bind the control channel
    ctrl_channel_.bind(config_.ctrl_channel_endpoint_);

    // Bind the RX thread channel
    rx_channel_.bind(config_.rx_channel_endpoint_);

    // Bind the frame ready and release channels
    frame_ready_channel_.bind(config_.frame_ready_endpoint_);
    frame_release_channel_.bind(config_.frame_release_endpoint_);

    // Set default subscription on frame release channel
    frame_release_channel_.subscribe("");

    // Add IPC channels to the reactor
    reactor_.register_channel(ctrl_channel_, boost::bind(&FrameReceiverApp::handle_ctrl_channel, this));
    reactor_.register_channel(rx_channel_, boost::bind(&FrameReceiverApp::handle_rx_channel, this));
	reactor_.register_channel(frame_release_channel_, boost::bind(&FrameReceiverApp::handle_frame_release_channel, this));

}

void FrameReceiverApp::cleanup_ipc_channels(void)
{
    // Remove IPC channels from the reactor
    reactor_.remove_channel(ctrl_channel_);
    reactor_.remove_channel(rx_channel_);
    reactor_.remove_channel(frame_release_channel_);

    // Close all channels
    ctrl_channel_.close();
    rx_channel_.close();
    frame_ready_channel_.close();
    frame_release_channel_.close();

}

void FrameReceiverApp::initialise_frame_decoder(void)
{
    switch (config_.sensor_type_)
    {
    case Defaults::SensorTypePercivalEmulator:
        frame_decoder_.reset(new PercivalEmulatorFrameDecoder(logger_, config_.enable_packet_logging_, config_.frame_timeout_ms_));
        LOG4CXX_INFO(logger_, "Created PERCIVAL emulator frame decoder instance");
        break;

    case Defaults::SensorTypePercival2M:
    case Defaults::SensorTypePercival13M:
        throw FrameReceiverException("Cannot initialize frame decoder - sensor type not yet implemented");
        break;

    case Defaults::SensorTypeExcalibur:
    	frame_decoder_.reset(new ExcaliburFrameDecoder(logger_, config_.enable_packet_logging_, config_.frame_timeout_ms_));
    	LOG4CXX_INFO(logger_, "Created EXCALIBUR frame decoder instance");
    	break;

    case Defaults::SensorTypeIllegal:
        throw FrameReceiverException("Cannot initialize frame decoder - illegal sensor type specified");
        break;

    default:
        throw FrameReceiverException("Cannot initialize frame decoder - sensor type not recognised");
        break;
    }
}


void FrameReceiverApp::initialise_buffer_manager(void)
{
    // Create a shared buffer manager
    buffer_manager_.reset(new SharedBufferManager(config_.shared_buffer_name_, config_.max_buffer_mem_,
            frame_decoder_->get_frame_buffer_size(), false));
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Initialised frame buffer manager of total size " << config_.max_buffer_mem_
            << " with " << buffer_manager_->get_num_buffers() << " buffers");

    // Register buffer manager with the frame decoder
    frame_decoder_->register_buffer_manager(buffer_manager_);

}

void FrameReceiverApp::precharge_buffers(void)
{
    // Push the IDs of all of the empty buffers onto the RX thread channel
    // TODO if the number of buffers is so big that the RX thread channel would reach HWM (in either direction)
    // before the reactor has time to start, we could consider putting this pre-charge into a timer handler
    // that runs as soon as the reactor starts, but need to think about how this might block. Need non-blocking
    // send on channels??
    for (int buf = 0; buf < buffer_manager_->get_num_buffers(); buf++)
    {
        IpcMessage buf_msg(IpcMessage::MsgTypeNotify, IpcMessage::MsgValNotifyFrameRelease);
        buf_msg.set_param("buffer_id", buf);
        rx_channel_.send(buf_msg.encode());
    }
}

void FrameReceiverApp::handle_ctrl_channel(void)
{
    // Receive a request message from the control channel
    std::string ctrl_req_encoded = ctrl_channel_.recv();

    // Construct a default reply
    IpcMessage ctrl_reply;

    // Parse and handle the message
    try {

        IpcMessage ctrl_req(ctrl_req_encoded.c_str());

        switch (ctrl_req.get_msg_type())
        {
        case IpcMessage::MsgTypeCmd:

        	LOG4CXX_DEBUG_LEVEL(3, logger_, "Got control channel command request");
            ctrl_reply.set_msg_type(IpcMessage::MsgTypeAck);
            ctrl_reply.set_msg_val(ctrl_req.get_msg_val());
            break;

        default:
        	LOG4CXX_ERROR(logger_, "Got unexpected command on control channel with type " << ctrl_req.get_msg_type());
            ctrl_reply.set_msg_type(IpcMessage::MsgTypeNack);
            ctrl_reply.set_msg_val(ctrl_req.get_msg_val());
        	break;
        }
    }
    catch (IpcMessageException& e)
    {
        LOG4CXX_ERROR(logger_, "Error decoding control channel request: " << e.what());
    }
    ctrl_channel_.send(ctrl_reply.encode());

}

void FrameReceiverApp::handle_rx_channel(void)
{
    std::string rx_reply_encoded = rx_channel_.recv();
    try {
        IpcMessage rx_reply(rx_reply_encoded.c_str());
        //LOG4CXX_DEBUG_LEVEL(1, logger_, "Got reply from RX thread : " << rx_reply_encoded);

        if ((rx_reply.get_msg_type() == IpcMessage::MsgTypeNotify) &&
            (rx_reply.get_msg_val() == IpcMessage::MsgValNotifyFrameReady))
        {
        	LOG4CXX_DEBUG_LEVEL(2, logger_, "Got frame ready notification from RX thread for frame " << rx_reply.get_param<int>("frame", -1)
                    << " in buffer " << rx_reply.get_param<int>("buffer_id", -1));
            frame_ready_channel_.send(rx_reply_encoded);

            frames_received_++;
        }
        else
        {
            LOG4CXX_ERROR(logger_, "Got unexpected message from RX thread: " << rx_reply_encoded);
        }
    }
    catch (IpcMessageException& e)
    {
        LOG4CXX_ERROR(logger_, "Error decoding RX thread channel reply: " << e.what());
    }
}

void FrameReceiverApp::handle_frame_release_channel(void)
{
    std::string frame_release_encoded = frame_release_channel_.recv();
    try {
        IpcMessage frame_release(frame_release_encoded.c_str());
        //LOG4CXX_DEBUG(logger_, "Got message on frame release channel : " << frame_release_encoded);

        if ((frame_release.get_msg_type() == IpcMessage::MsgTypeNotify) &&
            (frame_release.get_msg_val() == IpcMessage::MsgValNotifyFrameRelease))
        {
        	LOG4CXX_DEBUG_LEVEL(2, logger_, "Got frame release notification from processor from frame " << frame_release.get_param<int>("frame", -1)
                    << " in buffer " << frame_release.get_param<int>("buffer_id", -1));
            rx_channel_.send(frame_release_encoded);

            frames_released_++;

            if (config_.frame_count_ && (frames_released_ >= config_.frame_count_))
            {
                LOG4CXX_INFO(logger_, "Specified number of frames (" << config_.frame_count_ << ") received and released, terminating");
                stop();
                reactor_.stop();
            }
        }
        else
        {
            LOG4CXX_ERROR(logger_, "Got unexpected message on frame release channel: " << frame_release_encoded);
        }
    }
    catch (IpcMessageException& e)
    {
        LOG4CXX_ERROR(logger_, "Error decoding message on frame release channel: " << e.what());
    }
}

void FrameReceiverApp::rx_ping_timer_handler(void)
{

    IpcMessage rxPing(IpcMessage::MsgTypeCmd, IpcMessage::MsgValCmdStatus);
    rx_channel_.send(rxPing.encode());

}

void FrameReceiverApp::timer_handler2(void)
{
    static unsigned int dummy_last_frame = 0;

    LOG4CXX_DEBUG_LEVEL(1, logger_, "Sending frame ready message for frame " << dummy_last_frame);
    IpcMessage frame_ready(IpcMessage::MsgTypeNotify, IpcMessage::MsgValNotifyFrameReady);
    frame_ready.set_param("frame", dummy_last_frame++);
    frame_ready_channel_.send(frame_ready.encode());
}


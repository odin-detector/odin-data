/*
 * KafkaProducerPlugin.cpp
 *
 *  Created on: 25 Mar 2019
 *      Author: Emilio Perez
 */
#include <cstring>
#include <cstdlib>
#include <boost/algorithm/string.hpp>
#include <climits>
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "KafkaProducerPlugin.h"
#include "version.h"

namespace FrameProcessor {

  /**
   * Callback function used to report status of a delivered message.
   *
   * This function is only used internally.
   *
   * \param[in] kafka_producer- Pointer to a Kafka producer handler.
   * \param[in] kafka_message - Pointer to the message structure of the message being reported
   * \param[in] opaque - Opaque pointer (not used)
   */
  static void kafka_message_callback(rd_kafka_t *kafka_producer,
                                     const rd_kafka_message_t *kafka_message,
                                     void *opaque)
  {
    KafkaProducerPlugin *kafka_producer_plugin = static_cast<KafkaProducerPlugin *>(kafka_message->_private);
    if (kafka_message->err) {
      kafka_producer_plugin->on_message_error(
        rd_kafka_err2str(kafka_message->err));
    } else {
      // count message as acknowledged
      kafka_producer_plugin->on_message_ack();
    }
  }

  const std::string KafkaProducerPlugin::CONFIG_SERVERS = "servers";
  const std::string KafkaProducerPlugin::CONFIG_TOPIC = "topic";
  const std::string KafkaProducerPlugin::CONFIG_PARTITION = "partition";
  const std::string KafkaProducerPlugin::CONFIG_DATASET = "dataset";
  const std::string KafkaProducerPlugin::CONFIG_INCLUDE_PARAMETERS = "include_parameters";

  /**
   * The constructor sets up logging used within the class.
   */
  KafkaProducerPlugin::KafkaProducerPlugin()
    : dataset_name_(KAFKA_DEFAULT_DATASET), topic_name_(KAFKA_DEFAULT_TOPIC),
      kafka_producer_(NULL), kafka_topic_(NULL),
      partition_(RD_KAFKA_PARTITION_UA),
      include_parameters_(true)
  {
    // Setup logging for the class
    logger_ = Logger::getLogger("FP.KafkaProducer");
    logger_->setLevel(Level::getAll());
    LOG4CXX_TRACE(logger_, "KafkaProducer constructor.");
    this->reset_statistics();
  }

  /**
   * The destructor cleans up Kafka handlers.
   */
  KafkaProducerPlugin::~KafkaProducerPlugin()
  {
    LOG4CXX_TRACE(logger_, "KafkaProducer destructor.");
    destroy_kafka();
  }

  /**
   * Set configuration options for this Plugin.
   *
   * This sets up the Kafka Producer Plugin according to the configuration IpcMessage
   * objects that are received.
   *
   * \param[in] config - IpcMessage containing configuration data.
   * \param[out] reply - Response IpcMessage.
   */
  void KafkaProducerPlugin::configure(OdinData::IpcMessage &config,
                                      OdinData::IpcMessage &reply)
  {
    boost::lock_guard<boost::recursive_mutex> lock(mutex_);
    if (config.has_param(CONFIG_SERVERS)) {
      destroy_kafka();
      configure_kafka_servers(config.get_param<std::string>(CONFIG_SERVERS));
      configure_kafka_topic(this->topic_name_);
    }

    if (config.has_param(CONFIG_TOPIC)) {
      configure_kafka_topic(config.get_param<std::string>(CONFIG_TOPIC));
    }

    if (config.has_param(CONFIG_PARTITION)) {
      configure_partition(config.get_param<uint32_t>(CONFIG_PARTITION));
    }

    if (config.has_param(CONFIG_DATASET)) {
      configure_dataset(config.get_param<std::string>(CONFIG_DATASET));
    }

    if (config.has_param(CONFIG_INCLUDE_PARAMETERS)) {
      this->include_parameters_ = config.get_param<bool>(CONFIG_INCLUDE_PARAMETERS);
    }
  }

  /**
   * Get the configuration values for this Plugin.
   *
   * \param[out] reply - Response IpcMessage.
   */
  void KafkaProducerPlugin::requestConfiguration(OdinData::IpcMessage &reply)
  {
    reply.set_param(get_name() + "/" + KafkaProducerPlugin::CONFIG_SERVERS,
                    this->servers_);
    reply.set_param(get_name() + "/" + KafkaProducerPlugin::CONFIG_TOPIC,
                    this->topic_name_);
    reply.set_param(get_name() + "/" + KafkaProducerPlugin::CONFIG_PARTITION,
                    this->partition_);
    reply.set_param(get_name() + "/" + KafkaProducerPlugin::CONFIG_DATASET,
                    this->dataset_name_);
    reply.set_param(get_name() + "/" + KafkaProducerPlugin::CONFIG_INCLUDE_PARAMETERS,
                    this->include_parameters_);
  }

  /**
   * Collate status information for the plugin. The status is added to the status IpcMessage object.
   *
   * \param[out] status - Reference to an IpcMessage value to store the status.
   */
  void KafkaProducerPlugin::status(OdinData::IpcMessage &status)
  {
    // Make sure statistics are updated
    poll_delivery_message_report_queue();
    /* Number of sent frames */
    status.set_param(get_name() + "/" + "sent", frames_sent_);
    /* Number of lost frames */
    status.set_param(get_name() + "/" + "lost", frames_lost_);
    /* Number of acknowledged frames */
    status.set_param(get_name() + "/" + "ack", frames_ack_);
  }

  /**
   * Clear frame statistics
   */
  bool KafkaProducerPlugin::reset_statistics()
  {
    this->frames_sent_ = 0;
    this->frames_lost_ = 0;
    this->frames_ack_ = 0;
    return true;
  }

  /**
   * Destroy Kafka handlers for the connection and the topic
   */
  void KafkaProducerPlugin::destroy_kafka()
  {
    if (kafka_topic_ != NULL) {
      rd_kafka_flush(kafka_producer_, KAFKA_LINGER_MS);
      rd_kafka_topic_destroy(kafka_topic_);
      kafka_topic_ = NULL;
    }
    if (kafka_producer_ != NULL) {
      rd_kafka_destroy(kafka_producer_);
      kafka_producer_ = NULL;
    }
  }

  /**
   * Poll the delivery message report queue, calling the
   * callback function if appropriate.
   */
  void KafkaProducerPlugin::poll_delivery_message_report_queue()
  {
    if (kafka_producer_ != NULL) {
      boost::lock_guard<boost::recursive_mutex> lock(mutex_);
      rd_kafka_poll(kafka_producer_, 0);
    }
  }

  /**
   * Configure Kafka connection handler for the server/s specified.
   *
   * \param[in] servers - string representing Kafka brokers using format: IP:PORT[,IP2:PORT2,...]
   */
  void KafkaProducerPlugin::configure_kafka_servers(std::string servers)
  {
    rd_kafka_t *kafka_producer;
    rd_kafka_conf_t *kafka_config;
    char errBuf[KAFKA_ERROR_BUFFER_LEN];
    kafka_config = rd_kafka_conf_new();
    int status;

    status = rd_kafka_conf_set(kafka_config,
                               "message.max.bytes",
                               KAFKA_MESSAGE_MAX_BYTES,
                               errBuf,
                               sizeof(errBuf));

    if (status != RD_KAFKA_CONF_OK) {
      LOG4CXX_ERROR(logger_, "Kafka configuration error while setting max message size: "
        << errBuf);
      return;
    }

    status = rd_kafka_conf_set(kafka_config,
                               "queue.buffering.max.kbytes",
                               "2097151",
                               errBuf,
                               sizeof(errBuf));

    if (status != RD_KAFKA_CONF_OK) {
      LOG4CXX_ERROR(logger_, "Kafka configuration error while setting max buffering size: "
        << errBuf);
      return;
    }

    status = rd_kafka_conf_set(kafka_config,
                               "linger.ms",
                               "5",
                               errBuf,
                               sizeof(errBuf));

    if (status != RD_KAFKA_CONF_OK) {
      LOG4CXX_ERROR(logger_, "Kafka configuration error while setting linger ms: "
        << errBuf);
      return;
    }

    status = rd_kafka_conf_set(kafka_config,
                               "bootstrap.servers",
                               servers.c_str(),
                               errBuf,
                               sizeof(errBuf));

    if (status != RD_KAFKA_CONF_OK) {
      LOG4CXX_ERROR(logger_, "Kafka configuration error while setting botstrap servers"
        << errBuf);
      return;
    }

    // Configure callback to count ACKed messages
    rd_kafka_conf_set_dr_msg_cb(kafka_config, kafka_message_callback);

    // kafkaProducer will free kafka_config when destroyed
    kafka_producer = rd_kafka_new(RD_KAFKA_PRODUCER, kafka_config, errBuf,
                                  sizeof(errBuf));
    if (!kafka_producer) {
      LOG4CXX_ERROR(logger_, "Kafka handler error: " << errBuf);
      return;
    }

    this->kafka_producer_ = kafka_producer;

    this->servers_ = servers;
    LOG4CXX_TRACE(logger_, "Configured kafka servers: " << servers);

  }

  /**
   * Configure Kafka topic handler for the topic specified
   *
   * \param[in] topic_name - string representing the topic name.
   */
  void KafkaProducerPlugin::configure_kafka_topic(std::string topic_name)
  {

    if (this->kafka_producer_ == NULL) {
      LOG4CXX_WARN(logger_, "Broker is not configured");
      this->kafka_topic_ = NULL;
      return;
    }

    if (this->kafka_topic_ != NULL) {
      rd_kafka_topic_destroy(kafka_topic_);
    }

    this->kafka_topic_ = rd_kafka_topic_new(kafka_producer_,
                                            topic_name.c_str(),
                                            NULL);

    if (!this->kafka_topic_) {
      LOG4CXX_ERROR(logger_, "Kafka topic error");
    }

    this->topic_name_ = topic_name;
    LOG4CXX_TRACE(logger_, "Configured kafka topic: " << topic_name);
  }

  /**
   * Configure the dataset that will be published
   */
  void KafkaProducerPlugin::configure_dataset(std::string dataset)
  {
    this->dataset_name_ = dataset;
    LOG4CXX_TRACE(logger_, "Configured dataset " << this->dataset_name_);
  }

  /**
   * Set Kafka partition to send messages to
   *
   * If it is not configured, it defaults to automatic partitioning (using
   * the topic's partitioner function)
   *
   * \param[in] partition - partition number.
   */
  void KafkaProducerPlugin::configure_partition(int32_t partition)
  {
    this->partition_ = partition;
  }

  /**
   * Create a message with the following structure:
   *
   * [ json header length (2 bytes) ] + [ json header ] + [ frame data ]
   *
   * \param[in] frame - Pointer to a Frame object.
   * \param[out] nbytes - Reference to the message size in bytes.
   */
  void *KafkaProducerPlugin::create_message(boost::shared_ptr<Frame> frame,
                                            size_t &nbytes)
  {
    // creates header information
    rapidjson::StringBuffer string_buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(string_buffer);
    writer.StartObject();
    writer.String(MSG_HEADER_FRAME_SIZE_KEY);
    writer.Uint64(frame->get_data_size());
    writer.String(MSG_HEADER_DATA_TYPE_KEY);
    writer.Int(frame->get_meta_data().get_data_type());
    writer.String(MSG_HEADER_FRAME_NUMBER_KEY);
    writer.Uint64(frame->get_frame_number());
    writer.String(MSG_HEADER_ACQUISITION_ID_KEY);
    writer.String(frame->get_meta_data().get_acquisition_ID().c_str());
    writer.String(MSG_HEADER_COMPRESSION_KEY);
    writer.Uint(frame->get_meta_data().get_compression_type());
    writer.String(MSG_HEADER_FRAME_OFFSET_KEY);
    writer.Uint64(frame->get_meta_data().get_frame_offset());
    writer.String(MSG_HEADER_FRAME_DIMENSIONS_KEY);
    writer.StartArray();
    dimensions_t dims = frame->get_meta_data().get_dimensions();
    for (dimensions_t::iterator it = dims.begin(); it != dims.end(); it++) {
      writer.Uint64(*it);
    }
    writer.EndArray();
    if (this->include_parameters_) {
      const std::map<std::string, boost::any> &parameters = frame->get_meta_data().get_parameters();
      writer.String(MSG_HEADER_FRAME_PARAMETERS_KEY);
      writer.StartObject();
      for (std::map<std::string, boost::any>::const_iterator it = parameters.begin(); it != parameters.end(); it++) {
        writer.String(it->first.c_str());
        const std::type_info &ti = it->second.type();
        if (it->second.type() == typeid(unsigned long)) {
          writer.Uint64(frame->get_meta_data().get_parameter<unsigned long>(it->first));
        }
        else if (it->second.type() == typeid(float)) {
          writer.Double(frame->get_meta_data().get_parameter<float>(it->first));
        } else {
          writer.Null();
        }
      }
      writer.EndObject();
    }
    writer.EndObject();

    if (string_buffer.GetSize() > USHRT_MAX) {
      LOG4CXX_ERROR(logger_, "Header size is too big, it should be less than "
        << USHRT_MAX);
      nbytes = 0;
      return NULL;
    }

    size_t message_size = sizeof(uint16_t) + string_buffer.GetSize() + 1
      + frame->get_data_size();
    char *msg = (char *) malloc(message_size);
    uint16_t header_size = static_cast<uint16_t>(string_buffer.GetSize() + 1);

    *(reinterpret_cast<uint16_t *>(msg)) = header_size;
    // copy header data, this includes an ending null byte
    memcpy(msg + sizeof(uint16_t), string_buffer.GetString(),
           header_size);
    // copy frame data
    memcpy(msg + sizeof(uint16_t) + header_size, frame->get_data_ptr(),
           frame->get_data_size());
    nbytes = message_size;
    return msg;
  }

  /**
   * Create and enqueue a frame message to kafka server/s
   *
   * \param[in] frame - Pointer to a Frame object.
   */
  void KafkaProducerPlugin::enqueue_frame(boost::shared_ptr<Frame> frame)
  {
    LOG4CXX_TRACE(logger_, "Sending frame to message queue ...");
    if (!this->kafka_topic_) {
      LOG4CXX_WARN(logger_, "Topic not configured");
      return;
    }
    // This lock avoids configuring/destroying/enqueuing at the same time
    boost::lock_guard<boost::recursive_mutex> lock(mutex_);
    char *buf;
    size_t len;
    // This buffer is freed by kafka (when there are no errors)
    buf = (char *) create_message(frame, len);
    if (!buf) {
      return;
    }

    retry:
		// enqueue message
		int status = rd_kafka_produce(
		  this->kafka_topic_,
		  partition_,
		  /* free buffer when enqueued */
		  RD_KAFKA_MSG_F_FREE,
		  /* Data */
		  buf, len,
		  /* No key */
		  NULL, 0,
		  /* Opaque pointer */
		  this);

		if (status) {

		    if (rd_kafka_last_error() ==
		      RD_KAFKA_RESP_ERR__QUEUE_FULL) {
		        /* If the internal queue is full, wait for
		        * messages to be delivered and then retry.
		        * The internal queue represents both
		        * messages to be sent and messages that have
		        * been sent or failed, awaiting their
		        * delivery report callback to be called.
		        *
		        * The internal queue is limited by the
		        * configuration property
		        * queue.buffering.max.messages */
		        LOG4CXX_DEBUG(logger_, "Blocking whilst producer queue full");
		        rd_kafka_poll(this->kafka_producer_, 10/*block for max 10ms*/);
		        goto retry;
			} else {
		        // Dropping frame because some error other than queue full. The other possible errors are:
				//ERR_MSG_SIZE_TOO_LARGE - message is larger than configured max size: messages.max.bytes
				//ERR__UNKNOWN_PARTITION - requested partition is unknown in the Kafka cluster.
				//ERR__UNKNOWN_TOPIC
		        LOG4CXX_ERROR(logger_, "Error while producing: "
		        << rd_kafka_err2str(rd_kafka_last_error()));
		        free(buf);
		        frames_lost_++;
			}
		} else {
		  frames_sent_++;
		  LOG4CXX_DEBUG(logger_, "Frame joined producer queue: " << frame->get_frame_number());
		  LOG4CXX_DEBUG(logger_, "Total frames sent to queue: " << frames_sent_);
		}

		rd_kafka_poll(this->kafka_producer_, 0);
  }

  /**
   * It updates stats when a message is acknowledged
   */
  void KafkaProducerPlugin::on_message_ack()
  {
    this->frames_ack_++;
  }

  /**
   * It logs an error when a message delivery has failed
   */
  void KafkaProducerPlugin::on_message_error(const char *error)
  {
    LOG4CXX_ERROR(logger_, "Error while delivering message: " << error);
  }

  /**
   * If dataset configured matches, it sends the frame to kafka server/s
   *
   * \param[in] frame - Pointer to a Frame object.
   */
  void KafkaProducerPlugin::process_frame(boost::shared_ptr<Frame> frame)
  {
    LOG4CXX_TRACE(logger_, "Received a new frame...");
    if (frame->get_meta_data().get_dataset_name() == this->dataset_name_) {
      this->enqueue_frame(frame);
    }
    this->push(frame);
  }

  int KafkaProducerPlugin::get_version_major()
  {
    return ODIN_DATA_VERSION_MAJOR;
  }

  int KafkaProducerPlugin::get_version_minor()
  {
    return ODIN_DATA_VERSION_MINOR;
  }

  int KafkaProducerPlugin::get_version_patch()
  {
    return ODIN_DATA_VERSION_PATCH;
  }

  std::string KafkaProducerPlugin::get_version_short()
  {
    return ODIN_DATA_VERSION_STR_SHORT;
  }

  std::string KafkaProducerPlugin::get_version_long()
  {
    return ODIN_DATA_VERSION_STR;
  }

} /* namespace FrameProcessor */

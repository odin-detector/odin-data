/*
 * KafkaProducerPlugin.h
 *
 *  Created on: 25 Mar 2019
 *      Author: Emilio Perez
 */
#ifndef KAFKAPRODUCERPLUGIN_H
#define KAFKAPRODUCERPLUGIN_H

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>

#include <librdkafka/rdkafka.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameProcessorPlugin.h"

#define KAFKA_ERROR_BUFFER_LEN 512
#define KAFKA_MAX_MSG_LEN 512
#define KAFKA_LINGER_MS 1000
#define KAFKA_DEFAULT_DATASET "data"
#define KAFKA_DEFAULT_TOPIC   "data"
#define KAFKA_POLLING_MS 5000
#define KAFKA_MESSAGE_MAX_BYTES "134217728"
#define MSG_HEADER_FRAME_SIZE_KEY "data_size"
#define MSG_HEADER_DATA_TYPE_KEY "data_type"
#define MSG_HEADER_FRAME_NUMBER_KEY "frame_number"
#define MSG_HEADER_FRAME_DIMENSIONS_KEY "dims"
#define MSG_HEADER_ACQUISITION_ID_KEY "acquisition_id"
#define MSG_HEADER_COMPRESSION_KEY "compression"
#define MSG_HEADER_FRAME_OFFSET_KEY "frame_offset"
#define MSG_HEADER_FRAME_PARAMETERS_KEY "parameters"

/**
 * This plugin class creates and send messages containing
 * frames to one or more Kafka servers
 *
 */
namespace FrameProcessor {

  class KafkaProducerPlugin : public FrameProcessorPlugin {
  public:

    KafkaProducerPlugin();

    ~KafkaProducerPlugin();

    /* FrameProcessor interface methods */
    void configure(OdinData::IpcMessage &config, OdinData::IpcMessage &reply);

    void requestConfiguration(OdinData::IpcMessage &reply);

    void status(OdinData::IpcMessage &status);

    void process_frame(boost::shared_ptr <Frame> frame);

    bool reset_statistics();

    /* KafkaProducerPlugin specific methods */
    void destroy_kafka();

    void poll_delivery_message_report_queue();

    void on_message_ack();

    void on_message_error(const char *error);

    void configure_kafka_servers(std::string servers);

    void configure_kafka_topic(std::string topic_name);

    void configure_partition(int32_t partition);

    void configure_dataset(std::string dataset);

    void *create_message(boost::shared_ptr<Frame> frame,
                         size_t &nbytes);

    void enqueue_frame(boost::shared_ptr<Frame> frame);

    int get_version_major();

    int get_version_minor();

    int get_version_patch();

    std::string get_version_short();

    std::string get_version_long();

  private:
    /* Pointer to logger */
    LoggerPtr logger_;
    std::string dataset_name_;
    std::string topic_name_;
    std::string servers_;
    int32_t partition_;
    /* Kafka related attributes */
    rd_kafka_t *kafka_producer_;
    rd_kafka_topic_t *kafka_topic_;
    /* Frames statistics */
    uint32_t frames_sent_;
    uint32_t frames_lost_;
    uint32_t frames_ack_;
    /* Timer used for polling every KAFKA_POLLING_MS period
     * this is necessary to serve the delivery report queue */
    int polling_timer_id_;
    /* For protecting critical region, specially when configuring, we don't
     * want to poll or produce while the handlers are been destroyed */
    boost::recursive_mutex mutex_;

    /* True if frame parameters need to be included in the message header */
    bool include_parameters_;

    /* plugin specific parameters */
    static const std::string CONFIG_SERVERS;
    static const std::string CONFIG_TOPIC;
    static const std::string CONFIG_PARTITION;
    /* the datasets that will be published */
    static const std::string CONFIG_DATASET;
    static const std::string CONFIG_INCLUDE_PARAMETERS;

  };

} /* namespace FrameProcessor */

#endif //KAFKAPRODUCERPLUGIN_H

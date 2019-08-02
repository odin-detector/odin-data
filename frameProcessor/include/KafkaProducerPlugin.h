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
#define KAFKA_DEFAULT_TOPIC "data"
#define KAFKA_DEFAULT_RETRIES "1000"
#define KAFKA_DEFAULT_BACKOFF "500"
#define KAFKA_DEFAULT_MAX_TIME "300000"
#define KAFKA_DEFAULT_MAX_Q_MSGS "200"
#define KAFKA_DEFAULT_MAX_Q_SIZE "2097151"
#define KAFKA_DEFAULT_BATCH_MSGS "1"
#define KAFKA_DEFAULT_ACKS "all"
#define KAFKA_DEFAULT_MAX_BYTES "134217728"
#define KAFKA_DEFAULT_MAX_Q_BUFFER_TIME "0"
#define MSG_HEADER_FRAME_SIZE_KEY "data_size"
#define MSG_HEADER_DATA_TYPE_KEY "data_type"
#define MSG_HEADER_FRAME_NUMBER_KEY "frame_number"
#define MSG_HEADER_FRAME_DIMENSIONS_KEY "dims"
#define MSG_HEADER_ACQUISITION_ID_KEY "acquisition_id"
#define MSG_HEADER_COMPRESSION_KEY "compression"
#define MSG_HEADER_FRAME_OFFSET_KEY "frame_offset"
#define MSG_HEADER_FRAME_PARAMETERS_KEY "parameters"

namespace FrameProcessor {

  /**
   * KafkaProducerPlugin integrates Odin with Kafka.
   *
   * It creates and send messages that contains frame data and metadata
   * to one or more Kafka servers.
   *
   * Plugin parameters:
   *  servers: Kafka broker list using format IP:PORT[,IP2:PORT2,...]
   *           Once this is set, the plugin starts delivering to the specified server/s.
   *  dataset: Dataset name of frame that will be delivered. Defaults to "data".
   *  topic: Topic name of the queue to send the message to. Defaults to "data".
   *  partition: Partition number. Defaults to RD_KAFKA_PARTITION_UA (automatic partitioning)
   *  max_retries: How many times to retry sending a failing Message. Defaults to 1000.
   *  retry_backoff: The backoff time in ms before retrying a protocol request. Defaults to 500 ms.
   *  retry_max_time: Local message timeout. This value is only enforced locally and limits the time a produced message waits for successful delivery. 
   *                  A time of 0 is infinite. This is the maximum time librdkafka may use to deliver a message (including retries). 
   *                  Delivery error occurs when either the retry count or the message timeout are exceeded. Defaults to 300000 ms.
   *  max_q_msgs: Maximum number of messages allowed on the producer queue. This queue is shared by all topics and partitions. Defaults to 200.
   *  max_q_size: Maximum total message size sum allowed on the producer queue. This queue is shared by all topics and partitions. 
   *              This property has higher priority than queue.buffering.max.messages. Defaults to 2097151 kB (maximum possible).
   *  batch_msgs: Maximum number of messages batched in one MessageSet. Defaults to 1.
   *  acks: This field indicates the number of acknowledgements the leader broker must receive from ISR brokers before responding to the request: 
   *        *0*=Broker does not send any response/ack to client, *-1* or *all*=Broker will block until message is committed by all in sync replicas
   *        (ISRs). If there are less than `min.insync.replicas` (broker configuration) in the ISR set the produce request will fail. Defaults to
   *        *all*.
   *  max_msg_bytes: Maximum Kafka protocol request message size. Defaults to 134217728 B.
   *  max_q_buffer_time: Delay in ms to wait for messages in the producer queue to accumulate before constructing message batches to transmit to brokers.
   *                     Defaults to 0 ms.
   *  include_parameters: Boolean indicating if frame parameters should be included in the message.
   *                      Defaults to true.
   *
   * Status variables:
   *  sent: Number of sent frames.
   *  lost: Number of lost frames.
   *  ack:  Number of acknowledged frames.
   */
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

    void set_property(rd_kafka_conf_t *, const char *, const char *, char *);

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
    /** Pointer to logger */
    LoggerPtr logger_;
    /** Name of the dataset that will be delivered */
    std::string dataset_name_;
    /** Topic name identifying the destination queue */
    std::string topic_name_;
    /** Kafka brokers to connect to */
    std::string servers_;
    /** Partition number */
    int32_t partition_;
    /** Maximum number of produce retries for kafka */
    std::string max_retries_;
    /** Time to wait between retries */
    std::string retry_backoff_;
    /** Max time to produce message (inc retries) */
    std::string retry_max_time_;
    /** Max producer queue msgs */
    std::string max_q_msgs_;
    /** Max producer queue size */
    std::string max_q_size_;
    /** Max producer batch msgs */
    std::string batch_msgs_;
    /** Kafka acknowledgments */
    std::string acks_;
    /** Kafka max message bytes */
    std::string max_msg_bytes_;
    /** Kafka max queue buffer time (ms) */
    std::string max_q_buffer_time_;
    /** Pointer to a Kafka producer handler */
    rd_kafka_t *kafka_producer_;
    /** Pointer to a Kafka topic handler */
    rd_kafka_topic_t *kafka_topic_;
    /** Number of sent frames */
    uint32_t frames_sent_;
    /** Number of lost frames */
    uint32_t frames_lost_;
    /** Number of acknowledged frames */
    uint32_t frames_ack_;
    /* Timer used for polling every KAFKA_POLLING_MS period
     * this is necessary to serve the delivery report queue */
    int polling_timer_id_;
    /* For protecting critical region, specially when configuring, we don't
     * want to poll or produce while the handlers are been destroyed */
    boost::recursive_mutex mutex_;

    /** True if frame parameters need to be included in the message header */
    bool include_parameters_;

    /** Configuration constant for servers parameter */
    static const std::string CONFIG_SERVERS;
    /** Configuration constant for topic parameter */
    static const std::string CONFIG_TOPIC;
    /** Configuration constant for partition parameter */
    static const std::string CONFIG_PARTITION;
    /** Configuration constant for dataset parameter */
    static const std::string CONFIG_DATASET;
    /** Configuration constant for include_parameters */
    static const std::string CONFIG_INCLUDE_PARAMETERS;
    /** Configuration constant for kafka max retries */
    static const std::string CONFIG_KAFKA_RETRIES;
    /** Configuration constant for kafka time between retries */
    static const std::string CONFIG_KAFKA_RETRY_BACKOFF;
    /** Configuration constant for kafka max time for producing message (inc retries). 0 is infinite */
    static const std::string CONFIG_KAFKA_MAX_RETRY_TIME;
    /** Configuration constant for kafka max producer queue number of messages */
    static const std::string CONFIG_KAFKA_MAX_Q_MSGS;
    /** Configuration constant for kafka max producer queue size (max is 2097151 kB) */
    static const std::string CONFIG_KAFKA_MAX_Q_SIZE;
    /** Configuration constant for kafka max producer batch number of messages*/
    static const std::string CONFIG_KAFKA_BATCH_MSGS;
    /** Configuration constant for kafka acknowledgments */
    static const std::string CONFIG_KAFKA_ACKS;
    /** Configuration constant for kafka max msg size */
    static const std::string CONFIG_KAFKA_MAX_MSQ_BYTES;
    /** Configuration constant for kafka max queue buffer time (ms)*/
    static const std::string CONFIG_KAFKA_MAX_Q_BUFFER_TIME;

  };

} /* namespace FrameProcessor */

#endif //KAFKAPRODUCERPLUGIN_H

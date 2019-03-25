//
// Created by hir12111 on 25/03/19.
//

#include "KafkaProducerPlugin.h"
#include "version.h"

namespace FrameProcessor {

/**
 * The constructor sets up logging used within the class.
 */
  KafkaProducer::KafkaProducer()
  {
    // Setup logging for the class
    logger_ = Logger::getLogger("FP.KafkaProducer");
    logger_->setLevel(Level::getAll());
    LOG4CXX_TRACE(logger_, "KafkaProducer constructor.");
  }

/**
 * Destructor.
 */
  KafkaProducer::~KafkaProducer()
  {
    LOG4CXX_TRACE(logger_, "KafkaProducer destructor.");
  }


/**
 * Send the frame to a Kafka Broker and push it
 *
 * \param[in] frame - Pointer to a Frame object.
 */
  void KafkaProducer::process_frame(boost::shared_ptr <Frame> frame)
  {
    LOG4CXX_TRACE(logger_, "Received a new frame...");
    this->push(frame);
  }

  int KafkaProducer::get_version_major()
  {
    return ODIN_DATA_VERSION_MAJOR;
  }

  int KafkaProducer::get_version_minor()
  {
    return ODIN_DATA_VERSION_MINOR;
  }

  int KafkaProducer::get_version_patch()
  {
    return ODIN_DATA_VERSION_PATCH;
  }

  std::string KafkaProducer::get_version_short()
  {
    return ODIN_DATA_VERSION_STR_SHORT;
  }

  std::string KafkaProducer::get_version_long()
  {
    return ODIN_DATA_VERSION_STR;
  }

} /* namespace FrameProcessor */

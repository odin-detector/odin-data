/*
 * CallDuration.h
 *
 *  Created on: 20th March 2020
 *      Author: Gary Yendell
 */

#ifndef FRAMEPROCESSOR_SRC_CALLDURATION_H_
#define FRAMEPROCESSOR_SRC_CALLDURATION_H_

namespace FrameProcessor {

/**
 * A simple store for call duration metrics.
 *
 * Durations in microseconds.
 */
class CallDuration
{
public:
  void update(unsigned int duration);
  void reset();

  /** Last call duration **/
  unsigned int last_;
  /** Maximum call duration **/
  unsigned int max_;
  /** Mean call duration (exponential average) **/
  unsigned int mean_;
};

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_SRC_CALLDURATION_H_ */

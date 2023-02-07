/*
 * CallDuration.cpp
 *
 *  Created on: 20th March 2020
 *      Author: Gary Yendell
 */

#include "CallDuration.h"

namespace FrameProcessor {

/**
 * Replace last, replace max if higher and recalculate mean
 *
 * \param[in] duration - Duration to update with
 * */
void CallDuration::update(unsigned int duration) {
  last_ = duration;
  if (duration > max_) {
    max_ = duration;
  }
  if (mean_ == 0) {
    mean_ = duration;
  } else {
    mean_ = (mean_ + (double) duration) / 2;
  }
}

/**
 * Reset all values to 0
 * */
void CallDuration::reset() {
  last_ = 0;
  max_ = 0;
  mean_ = 0;
}

} /* namespace FrameProcessor */

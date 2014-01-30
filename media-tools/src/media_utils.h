/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _MEDIA_UTILS_H
#define _MEDIA_UTILS_H

#include "module_common_types.h" //taken from webrtc.org code
#include <math.h>

namespace media_resource {

// Standalone SNR computation function. Below piece of the code
// is taken from webrtc.org code output_mixer_unittest.cc
bool
VerifyParams(const AudioFrame& ref_frame, const AudioFrame& test_frame) {
  if (ref_frame.num_channels_ == test_frame.num_channels_) {
    if (ref_frame.samples_per_channel_ == test_frame.samples_per_channel_) {
      if (ref_frame.sample_rate_hz_ == test_frame.sample_rate_hz_) {
        return true;
      }
    }
  }
  return false;
}

// Computes the best SNR based on the error between |ref_frame| and
// |test_frame|. It allows for up to a |max_delay| in samples between the
// signals to compensate for the resampling delay.
void
GetSNR(const AudioFrame& aRefFrame, const AudioFrame& aTestFrame,
       int aMaxDelay, int aMaxSamples, float* aSNR, int *aDelay) {

  // set these to invalid values.
  *aSNR = -1.0;
  *aDelay = -1;

  // Compute best_snr and best_delay across function calls.
  static float best_snr = 0;
  static int best_delay = 0;

  for (int delay = 0; delay <= aMaxDelay; delay++) {
    float mse = 0;
    float variance = 0;
    for (int i = 0; i < aMaxSamples - delay; i++) {
      int error = aRefFrame.data_[i] - aTestFrame.data_[i + delay];
      mse += error * error;
      variance += aRefFrame.data_[i] * aRefFrame.data_[i];
    }
    float snr = 100;  // We assign 100 dB to the zero-error case.
    if (mse > 0) {
      snr = 10 * log10(variance / mse);
    }

    if (snr > best_snr) {
      best_snr = snr;
      best_delay = delay;
    }
  }
  *aDelay = best_delay;
  *aSNR = best_snr;
}

} // namespace
#endif

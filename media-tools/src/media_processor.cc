/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <stdio.h>
#include <math.h>
#include "module_common_types.h" //taken from webrtc.org code
#include "media_provider.h"
#include "media_processor.h"

using namespace std;

namespace media_resource {

// Below pieces of code is taken from webrtc.org repository.
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

void
UpdateAudioFrameData(AudioFrame& aFrame, AudioProvider*& aProvider) {
  aFrame.sample_rate_hz_ = aProvider->SamplingRate();
  aFrame.samples_per_channel_ = aFrame.sample_rate_hz_ / 100;
  aFrame.num_channels_ = aProvider->NumChannels();
}

// Computes the best SNR based on the error between |ref_frame| and
// |test_frame|. It allows for up to a |max_delay| in samples between the
// signals to compensate for the resampling delay.
void
GetSNR(const AudioFrame& aRefFrame, const AudioFrame& aTestFrame,
       int aMaxDelay, int kMaxDataSizeSamples, float* aSNR, int *aDelay) {

  // set these to invalid values.
  *aSNR = -1.0;
  *aDelay = -1;

  // Compute best_snr and best_delay across function calls.
  static float best_snr = 0;
  static int best_delay = 0;

  for (int delay = 0; delay <= aMaxDelay; delay++) {
    float mse = 0;
    float variance = 0;
    for (int i = 0; i < kMaxDataSizeSamples - delay; i++) {
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

void AudioProcessor::ComputeSNR(const std::string& aRefFileName,
                                const std::string& aTestFileName,
	                        float* aSNR, int* aDelay) {
  *aSNR = *aDelay = -1;
  const int kInputKernelDelaySamples = 16;
  const int MAX_SAMPLES = 640; // Stereo, 16 khz, 20ms audio 

  AudioProvider* provider_ref = NULL;
  AudioProvider* provider_test = NULL;

  // Create media providers for ref and test files.
  AudioProvider::Create(provider_ref);
  AudioProvider::Create(provider_test);

  if (!provider_ref || !provider_test)
    return;

  // Initialize the providers with source media files
  if (provider_ref->SetFileAsSource(aRefFileName,
                                    media_resource::WAVE) == -1) {
    cout << "Couldnot Initialize Provider for the Reference File " << endl;
    return;
  }

 if (provider_test->SetFileAsSource(aTestFileName,
                                    media_resource::WAVE) == -1) {
    cout << "Couldnot Initialize Provider the Test File " << endl;
    return;
  }

  AudioFrame src_frame;
  AudioFrame test_frame;
  float tmpSnr = 0.0;
  int tmpDelay = 0;
  int count = 0;
  int read_bytes_ref = 0;
  int read_bytes_test = 0;

  // Update frame with the meta info
  UpdateAudioFrameData(src_frame, provider_ref);
  UpdateAudioFrameData(test_frame, provider_test);

  // make sure meta-info match since we don't support
  // resampling.
  if(!VerifyParams(src_frame, test_frame)) {
    cout << " Reference and Test Frames meta-data mismatch " << endl;
    return;
  }

  const int max_delay = static_cast<double>(provider_test->SamplingRate())
    / provider_ref->SamplingRate() * kInputKernelDelaySamples * provider_test->NumChannels() * 2;

  while (1) {
    ++count;
    read_bytes_ref = provider_ref->GetData(src_frame.data_, MAX_SAMPLES);
    read_bytes_test = provider_test->GetData(test_frame.data_, MAX_SAMPLES);

    if(read_bytes_ref <= 0 || read_bytes_test <= 0) {
      // we reached end of one of the files.
      // no point in computing SNR
      break;
    }
    GetSNR(src_frame, test_frame, max_delay, MAX_SAMPLES, &tmpSnr, &tmpDelay);
  }

  *aSNR = tmpSnr;
  *aDelay = tmpDelay;
  
  
  AudioProvider::Destroy(provider_ref);
  AudioProvider::Destroy(provider_test);
}

}; //namespace

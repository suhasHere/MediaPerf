/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <stdio.h>
#include "media_utils.h"
#include "media_provider.h"
#include "media_processor.h"

using namespace std;

namespace media_resource {

// Utility function.
void
UpdateAudioFrameData(AudioFrame& aFrame, AudioProvider*& aProvider) {
  aFrame.sample_rate_hz_ = aProvider->SamplingRate();
  aFrame.samples_per_channel_ = aFrame.sample_rate_hz_ / 100;
  aFrame.num_channels_ = aProvider->NumChannels();
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
  int read_bytes_deg = 0;

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
    read_bytes_deg = provider_test->GetData(test_frame.data_, MAX_SAMPLES);

    if(read_bytes_ref <= 0 || read_bytes_deg <= 0) {
      // we reached end of one of the files.
      // no point in computing SNR
      break;
    }
    // compute snr block by block
    GetSNR(src_frame, test_frame, max_delay, MAX_SAMPLES, &tmpSnr, &tmpDelay);
  }

  *aSNR = tmpSnr;
  *aDelay = tmpDelay;
  
  // let's clean up.
  AudioProvider::Destroy(provider_ref);
  AudioProvider::Destroy(provider_test);
}

}; //namespace

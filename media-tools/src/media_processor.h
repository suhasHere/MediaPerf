/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MEDIA_PROCESSOR_H
#define MEDIA_PROCESSOR_H

/**
 * Uiltity class to host media processing utilitities
 */

/**
 * Design Notes:
 *  The idea behind using webrtc.org headers is to eventually link
 *  statically built libcommon_audio* libraries to this project.
 *  This will enable us to easily use audio tools such as re-sampler
 *  and others once we plan to add more complicated audio and video
 *  processing here. This decission might be changed if needed as
 *  we unearth more details
 */

namespace media_resource {

// Class to provide utility functions on the Audio Data
// At present, we compute SNR alone.
class AudioProcessor {
public:
  /**
   * Computes SNR between reference and test audio files.
   * aSNR and aDelay are returned with the computed values
   * on success.
   */
  void ComputeSNR(const std::string& aRefFileName,
                  const std::string& aTestFileName,
    	          float* aSNR, int* aDelay);
};

} // namespace
#endif


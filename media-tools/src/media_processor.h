/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MEDIA_PROCESSOR_H
#define MEDIA_PROCESSOR_H

/**
 * Uiltity class to host media processing utilitities
 * Only SNR is supported.
 */

namespace media_resource {

// Class to provide utility functions on the Audio Data
// At present, we compute SNR alone
class AudioProcessor {
public:
  /**
   * Computes SNR between reference and test audio files.
   * aSNR and aDelay are returned
   */
  void ComputeSNR(const std::string& aRefFileName,
                  const std::string& aTestFileName,
    	          float* aSNR, int* aDelay);
};

} // namespace
#endif


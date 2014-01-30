/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**-
 * Parts of this code is originally from the webrtc.org code.
 * Original copyrught is:
 * 
 * Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 * Use of this source code is governed by a BSD-style license
 * that can be found in the LICENSE file in the root of the source
 * tree. An additional intellectual property rights grant can be found
 * in the file PATENTS.  All contributing project authors may
 * be found in the AUTHORS file in the root of the source tree.
 */

#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <assert.h>
#include <string.h>  // memcpy

#include <algorithm>

#include "typedefs.h"
#include "constructor_magic.h" // handy magic code

/* This class holds up to 60 ms of super-wideband (32 kHz) stereo audio. It
 * allows for adding and subtracting frames while keeping track of the resulting
 * states.
 *
 * Notes
 * - The total number of samples in |data_| is
 *   samples_per_channel_ * num_channels_
 *
 * - Stereo data is interleaved starting with the left channel.
 *
 * - The +operator assume that you would never add exactly opposite frames when
 *   deciding the resulting state. To do this use the -operator.
 */
class AudioFrame {
 public:
  // Stereo, 32 kHz, 60 ms (2 * 32 * 60)
  static const int kMaxDataSizeSamples = 3840;

  enum VADActivity {
    kVadActive = 0,
    kVadPassive = 1,
    kVadUnknown = 2
  };

  enum SpeechType {
    kNormalSpeech = 0,
    kPLC = 1,
    kCNG = 2,
    kPLCCNG = 3,
    kUndefined = 4
  };

  AudioFrame();
  virtual ~AudioFrame() {}

  // |interleaved_| is not changed by this method.
  void UpdateFrame(int id, uint32_t timestamp, const int16_t* data,
                   int samples_per_channel, int sample_rate_hz,
                   SpeechType speech_type, VADActivity vad_activity,
                   int num_channels = 1, uint32_t energy = -1);

  AudioFrame& Append(const AudioFrame& rhs);

  void CopyFrom(const AudioFrame& src);

  void Mute();

  AudioFrame& operator>>=(const int rhs);
  AudioFrame& operator+=(const AudioFrame& rhs);
  AudioFrame& operator-=(const AudioFrame& rhs);

  int id_;
  uint32_t timestamp_;
  int16_t data_[kMaxDataSizeSamples];
  int samples_per_channel_;
  int sample_rate_hz_;
  int num_channels_;
  SpeechType speech_type_;
  VADActivity vad_activity_;
  uint32_t energy_;
  bool interleaved_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioFrame);
};

inline AudioFrame::AudioFrame()
    : id_(-1),
      timestamp_(0),
      samples_per_channel_(0),
      sample_rate_hz_(0),
      num_channels_(1),
      speech_type_(kUndefined),
      vad_activity_(kVadUnknown),
      energy_(0xffffffff),
      interleaved_(true) {}

inline void AudioFrame::UpdateFrame(int id, uint32_t timestamp,
                                    const int16_t* data,
                                    int samples_per_channel, int sample_rate_hz,
                                    SpeechType speech_type,
                                    VADActivity vad_activity, int num_channels,
                                    uint32_t energy) {
  id_ = id;
  timestamp_ = timestamp;
  samples_per_channel_ = samples_per_channel;
  sample_rate_hz_ = sample_rate_hz;
  speech_type_ = speech_type;
  vad_activity_ = vad_activity;
  num_channels_ = num_channels;
  energy_ = energy;

  const int length = samples_per_channel * num_channels;
  assert(length <= kMaxDataSizeSamples && length >= 0);
  if (data != NULL) {
    memcpy(data_, data, sizeof(int16_t) * length);
  } else {
    memset(data_, 0, sizeof(int16_t) * length);
  }
}

inline void AudioFrame::CopyFrom(const AudioFrame& src) {
  if (this == &src) return;

  id_ = src.id_;
  timestamp_ = src.timestamp_;
  samples_per_channel_ = src.samples_per_channel_;
  sample_rate_hz_ = src.sample_rate_hz_;
  speech_type_ = src.speech_type_;
  vad_activity_ = src.vad_activity_;
  num_channels_ = src.num_channels_;
  energy_ = src.energy_;
  interleaved_ = src.interleaved_;

  const int length = samples_per_channel_ * num_channels_;
  assert(length <= kMaxDataSizeSamples && length >= 0);
  memcpy(data_, src.data_, sizeof(int16_t) * length);
}

inline void AudioFrame::Mute() {
  memset(data_, 0, samples_per_channel_ * num_channels_ * sizeof(int16_t));
}

inline AudioFrame& AudioFrame::operator>>=(const int rhs) {
  assert((num_channels_ > 0) && (num_channels_ < 3));
  if ((num_channels_ > 2) || (num_channels_ < 1)) return *this;

  for (int i = 0; i < samples_per_channel_ * num_channels_; i++) {
    data_[i] = static_cast<int16_t>(data_[i] >> rhs);
  }
  return *this;
}

inline AudioFrame& AudioFrame::Append(const AudioFrame& rhs) {
  // Sanity check
  assert((num_channels_ > 0) && (num_channels_ < 3));
  assert(interleaved_ == rhs.interleaved_);
  if ((num_channels_ > 2) || (num_channels_ < 1)) return *this;
  if (num_channels_ != rhs.num_channels_) return *this;

  if ((vad_activity_ == kVadActive) || rhs.vad_activity_ == kVadActive) {
    vad_activity_ = kVadActive;
  } else if (vad_activity_ == kVadUnknown || rhs.vad_activity_ == kVadUnknown) {
    vad_activity_ = kVadUnknown;
  }
  if (speech_type_ != rhs.speech_type_) {
    speech_type_ = kUndefined;
  }

  int offset = samples_per_channel_ * num_channels_;
  for (int i = 0; i < rhs.samples_per_channel_ * rhs.num_channels_; i++) {
    data_[offset + i] = rhs.data_[i];
  }
  samples_per_channel_ += rhs.samples_per_channel_;
  return *this;
}

inline AudioFrame& AudioFrame::operator+=(const AudioFrame& rhs) {
  // Sanity check
  assert((num_channels_ > 0) && (num_channels_ < 3));
  assert(interleaved_ == rhs.interleaved_);
  if ((num_channels_ > 2) || (num_channels_ < 1)) return *this;
  if (num_channels_ != rhs.num_channels_) return *this;

  bool noPrevData = false;
  if (samples_per_channel_ != rhs.samples_per_channel_) {
    if (samples_per_channel_ == 0) {
      // special case we have no data to start with
      samples_per_channel_ = rhs.samples_per_channel_;
      noPrevData = true;
    } else {
      return *this;
    }
  }

  if ((vad_activity_ == kVadActive) || rhs.vad_activity_ == kVadActive) {
    vad_activity_ = kVadActive;
  } else if (vad_activity_ == kVadUnknown || rhs.vad_activity_ == kVadUnknown) {
    vad_activity_ = kVadUnknown;
  }

  if (speech_type_ != rhs.speech_type_) speech_type_ = kUndefined;

  if (noPrevData) {
    memcpy(data_, rhs.data_,
           sizeof(int16_t) * rhs.samples_per_channel_ * num_channels_);
  } else {
    // IMPROVEMENT this can be done very fast in assembly
    for (int i = 0; i < samples_per_channel_ * num_channels_; i++) {
      int32_t wrapGuard =
          static_cast<int32_t>(data_[i]) + static_cast<int32_t>(rhs.data_[i]);
      if (wrapGuard < -32768) {
        data_[i] = -32768;
      } else if (wrapGuard > 32767) {
        data_[i] = 32767;
      } else {
        data_[i] = (int16_t)wrapGuard;
      }
    }
  }
  energy_ = 0xffffffff;
  return *this;
}

inline AudioFrame& AudioFrame::operator-=(const AudioFrame& rhs) {
  // Sanity check
  assert((num_channels_ > 0) && (num_channels_ < 3));
  assert(interleaved_ == rhs.interleaved_);
  if ((num_channels_ > 2) || (num_channels_ < 1)) return *this;

  if ((samples_per_channel_ != rhs.samples_per_channel_) ||
      (num_channels_ != rhs.num_channels_)) {
    return *this;
  }
  if ((vad_activity_ != kVadPassive) || rhs.vad_activity_ != kVadPassive) {
    vad_activity_ = kVadUnknown;
  }
  speech_type_ = kUndefined;

  for (int i = 0; i < samples_per_channel_ * num_channels_; i++) {
    int32_t wrapGuard =
        static_cast<int32_t>(data_[i]) - static_cast<int32_t>(rhs.data_[i]);
    if (wrapGuard < -32768) {
      data_[i] = -32768;
    } else if (wrapGuard > 32767) {
      data_[i] = 32767;
    } else {
      data_[i] = (int16_t)wrapGuard;
    }
  }
  energy_ = 0xffffffff;
  return *this;
}

#endif  // MODULE_COMMON_TYPES_H

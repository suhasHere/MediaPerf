/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MEDIA_PROVIDER_H_
#define MEDIA_PROVIDER_H_

#include <string>
#include <stddef.h>
#include "typedefs.h"
#include "file_wrapper.h"

/**
 * Classes that provide abstraction and functionality for dealing
 * with different underlying media sources. As of now, logic for
 * dealing with WAV formatted audio file sources is supported.
 * Also this file will be expanded to include video sources as 
 * more utilities are added.
 */

 /**
  * TODO: Abstract the provider interface for dealing with 
  * Audio and Video sources.
  */
namespace media_resource {

// We support only PCM formatted media files today.
enum AudioFormat
{
  PCM =1 ,
  UNKNOWN
};

// Supported file formats.
enum FileFormats {
  WAVE = 1,
  UNSUPPORTED = 2
};

// Structure for WAV Header
// Source: https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
struct WAVHeader
{
  char mChunkId[4];
  int mChunkSize;
  char mFormat[4];
  char mSubChunk1Id[4];
  int mSubChunk1Size;
  short int mAudioFormat;
  short int mAudioChannels;
  int mSamplingRate;
  int mByteRate;
  short int mBlockAlign;
  short int mBitsPerSample;
  char mSubChunk2ID[4];
  int mSubChunk2Size;
};


// Source of the Audio Data. Audio data can be retrieved from
// sources such as media files or any mechanism that supports
// media streaming.
// This class also holds meta information such as Sample-rates
// Channels from the underlying source of media.
class AudioProvider {
public:
  
  // Factory to create AudioProvider Instance
  static int32_t Create(AudioProvider*& decoder);
  static void Destroy(AudioProvider*& decoder);

  // Initialize the provider with the file as source
  int16_t SetFileAsSource(const std::string& file,
                          FileFormats format);

  // Read numButes of data into the buffer passed in
  // Memory management of the buffer is handled by the caller.
  int32_t GetData(int16_t* buffer, const int numBytes);

  // Media File meta information API
  int32_t SamplingRate() const {
    return mSamplingRate;
  }

  int16_t NumChannels() const {
    return mNumChannels;
  }

  int16_t NumSamples() const {
    return mNumSamples;
  }

private:
  AudioProvider();
  ~AudioProvider();

  // Utility functions
  int16_t VerifyWAVHeader();

  int16_t mBitsPerSample;
  int16_t mNumChannels;
  int32_t mNumSamples;
  int32_t mSamplingRate;
  enum AudioFormat mAudioFormat;
  bool mIsInitialized;

  FileReader* mReader; // Owns

  DISALLOW_COPY_AND_ASSIGN(AudioProvider);
};

} //namespace





#endif

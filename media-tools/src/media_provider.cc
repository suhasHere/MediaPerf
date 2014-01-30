//TODO: Add Mozilla Copyright

#include <iostream>
#include <cstring>
#include "media_provider.h"

using namespace std;

namespace media_resource {

///////////////////////////////////////////////////////////
/////  AudioProvider Implementation /////////////////////////

int32_t
AudioProvider::Create(AudioProvider*& aProvider) {
  aProvider = new AudioProvider();
  if ( aProvider == NULL) {
    return -1;
  }
  return 0;
}

AudioProvider::AudioProvider():mBitsPerSample(0),
                               mNumChannels(0),
                               mNumSamples(0),
                               mIsInitialized(false),
                               mSamplingRate(0),
                               mAudioFormat(UNKNOWN),
                               mReader(NULL) { }

void
AudioProvider::Destroy(AudioProvider*& aProvider) {
  if (aProvider) {
    delete aProvider;
    aProvider = NULL;
  }
}


AudioProvider::~AudioProvider() {
  if(mReader) {
    // this should close the file automatically
    FileReader::Destroy(mReader);
  }
}

// Set Provider's media source as File with appropriate type
// Only WAV formatted audio file is supported as of today.
int16_t
AudioProvider::SetFileAsSource(const std::string& aFile,
                               FileFormats format) {

  if(!aFile.size() || format == UNSUPPORTED) {
    return -1;
  }

  // we overwrite if already set
  if(mReader) {
    FileReader::Destroy(mReader);
  }

  // Create file reader
  FileReader::Create(mReader);
  if(!mReader) {
    return -1;
  }

  if(mReader->OpenFile(aFile) == -1) {
    FileReader::Destroy(mReader);
    return -1;
  }

  //Attempt to read the header file.
  switch (format) {
    case WAVE:
      // Is this a good source ?
      if (VerifyWAVHeader() == -1 ) {
        mIsInitialized = false;
        mReader->CloseFile();
      } else {
        mIsInitialized = true;
      }
      break;
    default:
      mIsInitialized = false;
      break;
  }

  return (mIsInitialized == true) ? 0 : -1;
}

// Verifies if the provider source is indeed in WAV Format
int16_t
AudioProvider::VerifyWAVHeader() {
  int32_t len;
  WAVHeader header;
  len = mReader->Read(&header, sizeof(header));
  if (len != sizeof(header)) {
    cerr << " Invalid WAV Header " << endl;
    return -1;
  }

  if (strncmp(header.mChunkId, "RIFF", 4) != 0 ) {
    cerr << " RIFF Match Failed " << header.mChunkId << endl;
    return -1;
  }

  if (strncmp(header.mFormat, "WAVE", 4) != 0 ) {
    cerr << " WAVE Match Failed " << header.mFormat << endl;
    return -1;
  }

  // looks like WAV format. let's store meta info for
  // local use.
  mBitsPerSample = header.mBitsPerSample;
  mNumChannels = header.mAudioChannels;
  mSamplingRate = header.mSamplingRate;
  mAudioFormat = (header.mAudioFormat == 1 ) ? PCM : UNKNOWN;
  mNumSamples =
    (header.mSubChunk2Size * 8) / (header.mBitsPerSample * header.mAudioChannels);
  return 0;
}

int32_t
AudioProvider::GetData(int16_t* aBuffer, const int aNumBytes) {
  if (aNumBytes <= 0 || !mReader) {
    return -1;
  }

  //read numBytes from the underlying source
  return mReader->Read(aBuffer, aNumBytes);
}

} //namespace

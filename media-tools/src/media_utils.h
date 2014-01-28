#ifndef __MEDIA_UTILS_H
#define _MEDIA_UTILS_H

namespace media_resource {

enum FileFormats {
  WAVE = 1,
  UNSUPPORTED = 2
};

    // Structure representing WAV Header
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
}





#endif

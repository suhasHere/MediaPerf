# Utilities to process media - audio/video for Media Perf and Quality Tests
  note: This is still a work-in-progress and more will be added in the
  future as needed

##  Important Concepts
  - File Wrapper   - Abstraction over file operations
  - Media Provider - Reads media from the underlying sources such as files
                     and provides abstract API to consumers of the media
                     data 
  - Media Processor - Utities that process the media data. Only SNR reporting
                      is provided as of today.

# Run the following command to build and generate the binary named  MediaUtils
## Pre-requisites
    GYP generator is required

## Building
    gyp media_utils.gyp --depth=. -f make --generator-output=./build
    ==> This will generate OSX or Linux specific makefiles into build/
    make -C ./build
    ==> This will create binary named MediaUtils under build/out/Release

## Testing
    ./build/out/Release/MediaUtils -c snr -r <input.wav> -t <test.wav>
    where:
    -c --> command
    -r --> reference audio file
    -t --> test audio file 



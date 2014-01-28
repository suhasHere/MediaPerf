#include <iostream>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include "media_processor.h"


using namespace std;

typedef media_resource::AudioProcessor AudioProcessor;

int main(int argc, char** argv) {
  int opt = 0;
  string command;
  string  ref_file;
  string test_file;
  string usage = "MediaTool -c <command> -r <ref_media_file> -t <test_media_file> ";

  if (argc < 6) {
    cout << usage << endl;
    return -1;
  }

  opt = getopt( argc, argv, "c:r:t:vh");
  while (opt != -1) {
    switch (opt) {
      case 'c':
        command = optarg;
        break;
     case 'r':
        ref_file = optarg;
        break;
     case 't':
        test_file = optarg;
        break;
     case 'h':   /* fall-through is intentional */
     case '?':
      cout << usage << endl;
      break;
    }
    opt = getopt( argc, argv, "c:r:t:vh");
  }

  if(command == "snr") {
    AudioProcessor audio_proc;
    float snr = -1.0;
    int delay = -1.0;
    cout << " Reference File : " << ref_file << ", " 
         << " Test File :      " << test_file << endl;
    audio_proc.ComputeSNR(ref_file, test_file, &snr, &delay);
    printf("SNR_DELAY=%3.4f,%d", snr, delay);
  }
  return 0;
}

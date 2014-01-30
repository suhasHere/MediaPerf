/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "media_processor.h"

using namespace std;

//Application Program to verify media utilities.

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
    audio_proc.ComputeSNR(ref_file, test_file, &snr, &delay);
    printf("SNR_DELAY=%3.4f,%d", snr, delay);
  }
  return 0;
}

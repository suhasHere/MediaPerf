/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <stdio.h>
#include "file_wrapper.h"

using namespace std;

namespace media_resource {

void
FileReader::Create(FileReader*& aReader) {
  aReader = new FileReader();
}

void
FileReader::Destroy(FileReader*& aReader) {
  if (aReader) {
    delete aReader;
  }
}

FileReader::FileReader():mFileHandle(NULL) {}


FileReader::~FileReader() {
  if (mFileHandle) {
    CloseFile();
  }
}

int
FileReader::OpenFile(const std::string& aFileNamePath) {
  if (!aFileNamePath.size())
    return -1;

  mFileHandle = fopen(aFileNamePath.c_str(), "rb");
  if (!mFileHandle) {
    cerr << " File open failed " << endl;
    return -1;
  }
  return 0;
}

void
FileReader::CloseFile() {
  if (mFileHandle) {
    fclose(mFileHandle);
    mFileHandle = NULL;
  }
}

int
FileReader::Read(void* aBuffer, int aLength) {
  if ( !aBuffer || aLength < 0 )
    return -1;

  if (!mFileHandle)
    return -1;

  int nBytes = static_cast<int>(fread(aBuffer, 1 , aLength, mFileHandle));
  if (nBytes != aLength) {
    // We either reached end of the file or it is a file-read error
    CloseFile();
  }
  return nBytes;
}

} //namespace

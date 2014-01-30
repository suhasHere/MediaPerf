/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef FILE_READER_
#define FILE_READER_

#include <string>
#include <stddef.h>
#include "constructor_magic.h"

/**
 * Purpose of this code is to wrap underlying file-system APIs for
 * dealing with file handling. Only File-Reading abstraction has
 * been provided as of today. More functionlity can be added as
 * needed in the future.
 * TODO: Make this interface as a stream abstraction so the consumers
 * can use it in IOStream context.
 */

namespace media_resource {

class FileReader  {
public:
  // Factory methods for object management
  static void Create(FileReader*& aReader);
  static void Destroy(FileReader*& aReader);

  // aFileName should be absolute path.
  int OpenFile(const std::string& aFileNamePath);
  void CloseFile();

  // Read aLength bytes in to aBuffer
  // Memory for aBuffer is the responsibility of the caller.
  int Read(void* aBuffer, int aLength);

private:
   FileReader();
   ~FileReader();
   std::string mFileName;
   FILE* mFileHandle;

   DISALLOW_COPY_AND_ASSIGN(FileReader);
};

} //namespace

#endif

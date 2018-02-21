//===--- CMemoryReader.h - MemoryReader wrapper for C impls -----*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
//  This file declares an implementation of MemoryReader that wraps the
//  C interface provided by SwiftRemoteMirror.
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_REMOTE_CMEMORYREADER_H
#define SWIFT_REMOTE_CMEMORYREADER_H

#include "swift/SwiftRemoteMirror/MemoryReaderInterface.h"
#include "swift/Remote/MemoryReader.h"

namespace swift {
namespace remote {

/// An implementation of MemoryReader which wraps the C interface offered
/// by SwiftRemoteMirror.
class CMemoryReader final : public MemoryReader {
  MemoryReaderImpl Impl;

public:
  CMemoryReader(MemoryReaderImpl Impl) : Impl(Impl) {
    assert(this->Impl.getPointerSize && "No getPointerSize implementation");
    assert(this->Impl.getStringLength && "No stringLength implementation");
    assert(this->Impl.readBytes && "No readBytes implementation");
    assert(this->Impl.getPointerSize(this->Impl.reader_context) != 0 &&
           "Invalid target pointer size");
  }

  uint8_t getPointerSize() override {
    return Impl.getPointerSize(Impl.reader_context);
  }

  uint8_t getSizeSize() override {
    return Impl.getSizeSize(Impl.reader_context);
  }

  RemoteAddress getSymbolAddress(const std::string &name) override {
    auto addressData = Impl.getSymbolAddress(Impl.reader_context,
                                             name.c_str(), name.size());
    return RemoteAddress(addressData);
  }

  uint64_t getStringLength(RemoteAddress address) {
    return Impl.getStringLength(Impl.reader_context,
                                address.getAddressData());
  }

  bool readString(RemoteAddress address, std::string &dest) override {
    auto length = getStringLength(address);
    if (!length)
      return false;

    const void *ptr;
    std::function<void()> freeFunc;
    std::tie(ptr, freeFunc) = readBytes(address, length);
    if (ptr != nullptr) {
      dest = std::string(reinterpret_cast<const char *>(ptr), length);
      freeFunc();
      return true;
    } else {
      return false;
    }
  }

  std::tuple<const void *, std::function<void()>>
    readBytes(RemoteAddress address, uint64_t size) override {
      FreeBytesFunction freeFunc;
      void *freeContext;
      auto ptr = Impl.readBytes(Impl.reader_context, address.getAddressData(), size,
                                &freeFunc, &freeContext);
      auto freeLambda = [=]{ freeFunc(ptr, freeContext); };
      
      return std::make_tuple(ptr, freeLambda);
  }
};

}
}

#endif

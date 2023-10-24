#ifndef _RESOURCE_H_
#define _RESOURCE_H_
#include <stdio.h>
#ifdef __linux__
#include <dirent.h>
#include <errno.h>
#else
#include <windows.h>
#endif

#include "common/exception.h"
#include "tool/scoped.h"

namespace scope {
#ifdef __linux__
struct DirTraits {
  using Type = DIR*;
  static constexpr Type Default = NULL;
  static void Clean(Type& resource) {
    if (::closedir(resource)) {
      throw except::Exception("Close Dir", errno);
    }
  }
};
using ScopedDir = ScopedResource<DirTraits>;
#else
struct HandleTraits {
  using Type = HANDLE;
  static constexpr Type Default = NULL;
  static void Clean(Type& resource) {
    if (!::CloseHandle(resource)) {
      throw except::Exception("Close Handle", GetLastError());
    }
  }
};
using ScopedHandle = ScopedResource<HandleTraits>;

struct ModuleTraits {
  using Type = HMODULE;
  static constexpr Type Default = NULL;
  static void Clean(Type& resource) {
    if (!::FreeLibrary(resource)) {
      throw except::Exception("Free Library", GetLastError());
    }
  }
};
using ScopedModule = ScopedResource<ModuleTraits>;
#endif
struct FileTraits {
  using Type = FILE*;
  static constexpr Type Default = NULL;
  static void Clean(Type& resource) {
    if (::fclose(resource)) {
      throw except::Exception("Close File", errno);
    }
  }
};
using ScopedFile = ScopedResource<FileTraits>;

template <typename Base>
struct MemoryTraits {
  using Type = Base*;
  static constexpr Type Default = NULL;
  static void Clean(Type& resource) { delete[] resource; }
};
}  // namespace scope
#endif
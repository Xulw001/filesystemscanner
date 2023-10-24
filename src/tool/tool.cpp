#ifndef __linux__
#include "tool.h"

#include <Windows.h>
bool common::W2A(const std::wstring &src, std::string *dest) {
  if (src.empty()) {
    return true;
  }

  int size = -1;
  size = WideCharToMultiByte(CP_ACP, 0, src.c_str(), -1, NULL, 0, NULL, NULL);
  if (size <= 0) {
    return false;
  }

  dest->resize(size);
  size = WideCharToMultiByte(CP_ACP, 0, src.c_str(), -1, (char *)dest->c_str(),
                             size, NULL, NULL);
  if (size <= 0) {
    return false;
  }

  dest->resize(size - 1);
  return true;
}
#endif

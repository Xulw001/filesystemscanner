#ifndef _EXCEPTION_H
#define _EXCEPTION_H
#include "type.h"
namespace except {
using common::b32;
class Exception {
 public:
  Exception(const char* what, b32 system_code)
      : what_(what), system_code_(system_code) {
    ;
  }

  const char* what() const { return what_; }
  b32 system_code() const { return system_code_; }

 private:
  const char* what_;
  b32 system_code_;
};
}  // namespace except

#endif
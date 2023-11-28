#ifndef COMMON_TYPE_H_
#define COMMON_TYPE_H_
namespace common {
typedef char b8;
typedef short b16;
typedef int b32;
typedef long long b64;

typedef unsigned char ub8;
typedef unsigned short ub16;
typedef unsigned int ub32;
typedef unsigned long long ub64;
typedef unsigned long ulong;

#ifdef _WIN32
typedef wchar_t word;
#endif

enum FilePointer {
  SK_SET,
  SK_CUR,
  SK_END,
};

}  // namespace common
#endif
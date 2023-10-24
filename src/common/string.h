#ifndef COMMON_STRING_H_
#define COMMON_STRING_H_
#include <string>
namespace common {
#ifdef __linux__
typedef std::string os_string;
const os_string empty = "";
#else
typedef std::wstring os_string;
const os_string empty = L"";
#endif

}  // namespace common
#endif
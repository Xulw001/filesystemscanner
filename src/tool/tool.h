#ifndef _TASKMGR_TOOL_H
#define _TASKMGR_TOOL_H
#include <string>
namespace common {
bool W2A(const std::wstring &src, std::string *dest);
}  // namespace common
#endif
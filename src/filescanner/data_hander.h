#ifndef _DATA_HANDER_H_
#define _DATA_HANDER_H_
#include "common/string.h"
#include "common/type.h"

namespace filescanner {
using namespace common;
class IDataHanderInterface {
 public:
  // callback for scanner if need read filecontent
  virtual bool FileContentNeed() = 0;
  // callback for scanner when file content arrive,
  // when ret > 0, continue read;
  //      ret = 0, goto next file;
  // when ret < 0, goto error;
  virtual b32 OnFileContent(const os_string& filepath, const b8* contents,
                            const ulong& size) = 0;
  // callback for scanner when a new file arrive
  virtual void OnFilePath(const common::os_string& filepath,
                          const b64& file_size) = 0;
};
}  // namespace filescanner
#endif
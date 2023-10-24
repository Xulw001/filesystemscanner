#ifndef VOLUME_HELPER_H_
#define VOLUME_HELPER_H_
#include <vector>

#include "common/string.h"
#include "common/type.h"
#include "common/unorder_map.h"

using common::os_string;
using std::vector;
namespace filescanner {

typedef struct {
  os_string fs_device;  // device name
  os_string fs_type;    // file system type
  os_string fs_path;    // mount path
} Volume_Info;

typedef struct {
  os_string fs_path;  // mount path
  ub32 fs_type;       // file system type
  ub32 fs_size;
} Volume_Info2;

typedef common::unordered_map<os_string, Volume_Info2> VolumeMap;

class VolumeHelper {
 public:
  static bool GetVolumeInfo(VolumeMap *volume_info_map);
  static bool GetActualPathByPath(const os_string &path, os_string *act_path);

 private:
  static bool GetVolumeInfo(vector<Volume_Info> *volume_info_vec);
  static bool PraseVolumeInfo(const vector<Volume_Info> &volume_info_vec,
                              VolumeMap *volume_info_map);
  static b64 GetVolumeSpace(const os_string &volume_path);
};
}  // namespace filescanner
#endif

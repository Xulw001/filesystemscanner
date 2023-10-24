#ifndef DISK_SCANNER_H_
#define DISK_SCANNER_H_
#include "common/string.h"
#include "common/unorder_map.h"
#include "data_hander.h"
#include "volume_helper.h"

using common::os_string;
namespace filescanner {
class CDiskScanner {
 public:
  explicit CDiskScanner(IDataHanderInterface *p_data_hander)
      : p_data_hander_(p_data_hander) {
    file_count_ = 0;
    disk_occupied_space_ = 0;
    disk_scanned_space_ = 0;
    scan_process_ = 0;
    scan_canceled_ = false;
    current_path_ = "";
  }

  bool Scan();
  bool Scan(const os_string &path);

  void UpdateScannedSpace(b64 ll_scanned_space) {
    disk_scanned_space_ += ll_scanned_space;
  }

  void UpdateScannedProcess(b64 ll_file_count);

  b32 GetFileSystemType(const os_string &volume_path) {
    return map_volume_info_[volume_path].fs_type;
  }

  const os_string &GetFileSystemMountedPath(const os_string &volume_path) {
    return map_volume_info_[volume_path].fs_path;
  }

  bool CheckFileSystemMounted(const os_string &volume_path) {
    return map_volume_info_.find(volume_path) != map_volume_info_.end();
  }

  bool scan_canceled() const { return scan_canceled_; }
  void set_scan_canceled() { scan_canceled_ = true; }

  b16 process() const { return scan_process_; }

  const std::string &current_path() const { return current_path_; }
  void set_current_path(const std::string &current_path) {
    current_path_ = current_path;
  }

 private:
  IDataHanderInterface *p_data_hander_;

  b64 file_count_;           // the count has been deal with data hander
  b64 disk_occupied_space_;  // the disk size which has be used
  b64 disk_scanned_space_;   // the disk size which has be scanned

  b16 scan_process_;
  bool scan_canceled_;

  std::string current_path_;  // recently file has been dealed

  VolumeMap map_volume_info_;
};
}  // namespace filescanner
#endif
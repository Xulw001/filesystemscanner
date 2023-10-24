#ifndef SCANNER_FACTORY_H_
#define SCANNER_FACTORY_H_
#include "data_hander.h"
#include "disk_scanner.h"
#include "folder_selector.h"
#include "volume_scanner.h"

namespace filescanner {

typedef enum {
  NTFS,
  EXT4,
  XFS,
  OTHER = 9999,
} fs_type;

class ScannerFactory {
 public:
  ScannerFactory();
  ~ScannerFactory();
  static IVolumeScannerInterface *CreateScanner(
      const os_string &path, CDiskScanner *p_disk_scanner,
      IDataHanderInterface *p_data_handler, CFolderSelector *p_folder_selector);
  fs_type GetFileSystemType(const os_string &volume_name);

 private:
  common::unordered_map<os_string, fs_type> fs_name_map;
};
}  // namespace filescanner
#endif
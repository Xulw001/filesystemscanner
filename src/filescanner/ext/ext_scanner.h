#ifdef __linux__
#ifndef _EXT_SCANNER_H
#define _EXT_SCANNER_H
#include "common/map.h"
#include "disk_scanner.h"
#include "ext_record.h"
#include "folder_selector.h"
#include "volume_scanner.h"

namespace filescanner {
namespace ext {
class CExt4Scanner : public IVolumeScannerInterface {
 public:
  CExt4Scanner(const os_string &path, CDiskScanner *p_disk_scanner,
                  IDataHanderInterface *p_data_handler,
                  CFolderSelector *p_folder_selector)
      : volume_name_(path),
        p_disk_scanner_(p_disk_scanner),
        p_folder_selector_(p_folder_selector),
        p_data_handler_(p_data_handler),
        ext_volume_(),
        file_record_(&ext_volume_) {
    ;
  }
  bool Scan();

 private:
  bool TraverseSubEntries(CFileContent *fp, const os_string &dir_path);
  bool PraseFiles(CFileContent *fp, const os_string &dir_path);

 private:
  os_string volume_name_;
  CDiskScanner *p_disk_scanner_;
  CFolderSelector *p_folder_selector_;
  IDataHanderInterface *p_data_handler_;

  CExt4Volume ext_volume_;
  CFileRecord file_record_;

  common::map<b64, os_string> dir_list_;
  common::map<b64, os_string> file_list_;
};
}  // namespace ext
}  // namespace filescanner
#endif
#endif
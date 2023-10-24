#ifdef _WIN32
#ifndef NTFS_SCANNER_H_
#define NTFS_SCANNER_H_
#include "disk_scanner.h"
#include "folder_selector.h"
#include "volume_scanner.h"
#include "ntfs_record.h"
#include "common/map.h"

namespace filescanner {
namespace ntfs {
using common::os_string;

struct FileInfo {
  os_string path;
  b64 size;
};

class CIndexEntry;

class CNtfsScanner : public IVolumeScannerInterface {
 public:
  CNtfsScanner(const os_string &path, CDiskScanner *p_disk_scanner,
               IDataHanderInterface *p_data_handler,
               CFolderSelector *p_folder_selector)
      : volume_name_(path),
        p_disk_scanner_(p_disk_scanner),
        p_folder_selector_(p_folder_selector),
        p_data_handler_(p_data_handler),
        ntfs_volume_(),
        file_record_(&ntfs_volume_) {
    ;
  }

  bool Scan();

 private:
  void TraverseSubEntries(const os_string &dir_path);
  void IndexEntryCallback(const CIndexEntry &ie, const os_string &dir_path);
  void TraverseSubNode(const b64 &vcn, const os_string &dir_path);
  void ParseFiles();

 private:
  os_string volume_name_;
  CDiskScanner *p_disk_scanner_;
  CFolderSelector *p_folder_selector_;

  IDataHanderInterface *p_data_handler_;

  CNtfsVolume ntfs_volume_;
  CFileRecord file_record_;

  common::map<b64, os_string> folder_map_;
  common::map<b64, FileInfo> file_map_;
  //   list<wstring> m_lstRawFiles;
  //   map<ULONGLONG, FileBaseInfo> m_mapNotResidentFiles;
};
}  // namespace ntfs
}  // namespace filescanner
#endif
#endif

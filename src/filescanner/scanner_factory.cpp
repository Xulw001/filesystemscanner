#include "scanner_factory.h"

#include "default/default_scanner.h"
#include "ntfs/ntfs_scanner.h"

namespace filescanner {
ScannerFactory::ScannerFactory() {
#ifdef _WIN32
  fs_name_map.emplace(std::make_pair(L"NTFS", NTFS));
#else
  fs_name_map.emplace(std::make_pair("ext4", EXT4));
  fs_name_map.emplace(std::make_pair("xfs", XFS));
#endif
}

ScannerFactory::~ScannerFactory() { fs_name_map.clear(); }

IVolumeScannerInterface *ScannerFactory::CreateScanner(
    const os_string &path, CDiskScanner *p_disk_scanner,
    IDataHanderInterface *p_data_handler, CFolderSelector *p_folder_selector) {
  fs_type filetype = (fs_type)p_disk_scanner->GetFileSystemType(path);
  switch (filetype) {
    case NTFS:
      return new ntfs::CNtfsScanner(path, p_disk_scanner, p_data_handler,
                                    p_folder_selector);
    case EXT4:
    case XFS:
    case OTHER:
      return new default ::CDefaultScanner(path, p_disk_scanner, p_data_handler,
                                           p_folder_selector);
  }
  return NULL;
}

fs_type ScannerFactory::GetFileSystemType(const os_string &volume_name) {
  common::unordered_map<os_string, fs_type>::iterator find =
      fs_name_map.find(volume_name);
  if (find == fs_name_map.end()) {
    fs_name_map.emplace(std::make_pair(volume_name, OTHER));
    return OTHER;
  } else {
    return find->second;
  }
}
}  // namespace filescanner
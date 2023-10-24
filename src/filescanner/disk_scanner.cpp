#include "disk_scanner.h"

#include <functional>
#include <thread>
#ifdef __linux__
#include <malloc.h>
#else
#include <Windows.h>
#endif

#include "folder_selector.h"
#include "scanner_factory.h"

namespace filescanner {

bool CDiskScanner::Scan() {
  VolumeHelper volume_helper;
  if (!volume_helper.GetVolumeInfo(&map_volume_info_)) {
    return false;
  }

  VolumeMap::iterator iter;
  ScannerFactory scanner_factory;
  for (iter = map_volume_info_.begin(); iter != map_volume_info_.end();
       iter++) {
    IVolumeScannerInterface *p_vol_scanner = ScannerFactory::CreateScanner(
        iter->first.c_str(), this, p_data_hander_, NULL);

    if (p_vol_scanner) {
      p_vol_scanner->Scan();
      delete p_vol_scanner;
    }

    if (scan_canceled_) break;
  }

  scan_process_ = 100;

#ifdef __linux__
  malloc_trim(0);
#else
  // first-time to release unmapped memory
  SetProcessWorkingSetSize(GetCurrentProcess(), 100000, 200000);
#endif

  return true;
}
bool CDiskScanner::Scan(const os_string &path) {
  VolumeHelper volume_helper;
  if (!volume_helper.GetVolumeInfo(&map_volume_info_)) {
    return false;
  }

  VolumeMap::iterator iter;
  for (iter = map_volume_info_.begin(); iter != map_volume_info_.end();
       iter++) {
    disk_occupied_space_ += iter->second.fs_size;
  }

  ScannerFactory scanner_factory;
  CFolderSelector folder_selector(path);
  for (iter = map_volume_info_.begin(); iter != map_volume_info_.end();
       iter++) {
    IVolumeScannerInterface *volume_scanner = ScannerFactory::CreateScanner(
        iter->first.c_str(), this, p_data_hander_, &folder_selector);
    if (volume_scanner) {
      volume_scanner->Scan();
      delete volume_scanner;
    }

    if (scan_canceled_) break;
  }

  scan_process_ = 100;

#ifdef __linux__
  malloc_trim(0);
#else
  // first-time to release unmapped memory
  SetProcessWorkingSetSize(GetCurrentProcess(), 100000, 200000);
#endif

  return true;
}

void CDiskScanner::UpdateScannedProcess(b64 ll_file_count) {
  file_count_ += ll_file_count;

  ub8 percent = (ub8)(disk_scanned_space_ * 95 / disk_occupied_space_);
  percent = percent > 95 ? percent : 95;

  static time_t tick_time = time(NULL);
  if (percent <= scan_process_) {
    time_t diff_time = time(NULL) - tick_time;
    if (diff_time > 3) {
      tick_time = time(NULL);
      scan_process_++;
    }
  } else {
    tick_time = time(NULL);
    scan_process_ = percent;
  }
}
}  // namespace filescanner
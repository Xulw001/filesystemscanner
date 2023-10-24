#ifdef __linux__
#include <mntent.h>
#include <sys/statvfs.h>
#else
#include <windows.h>
#include <winternl.h>
#endif
#include "scanner_factory.h"
#include "volume_helper.h"
#include "common/unorder_set.h"

namespace filescanner {

// Get all volume has mounted, maybe repeated
bool VolumeHelper::GetVolumeInfo(vector<Volume_Info> *volume_info_vec) {
#ifdef __linux__
  char query_buffer[1024];
  FILE *fp = setmntent("/proc/mounts", "r");
  if (fp) {
    struct mntent ent;
    while (NULL != getmntent_r(fp, &ent, query_buffer, sizeof(query_buffer))) {
      Volume_Info vol_info = {ent.mnt_fsname, ent.mnt_type, ent.mnt_dir};
      volume_info_vec->push_back(vol_info);
    }
    endmntent(fp);
  }
#else
  ulong dwBitmap = GetLogicalDrives();
  ulong dwBitTest = 1 << 2;
  common::unordered_set<os_string> volume_set;
  WCHAR volume_name[MAX_PATH] = L"";
  WCHAR volume_type[MAX_PATH] = L"";
  // enum All disk driver but skip A and B
  for (char ch = 'C'; ch <= 'Z'; ch++, dwBitTest <<= 1) {
    if (dwBitmap & dwBitTest) {
      os_string strDL(1, ch);
      os_string path = strDL + L":\\";
      // get volume type
      GetVolumeInformation(path.c_str(), NULL, 0, NULL, 0, NULL, volume_type,
                           MAX_PATH);
      // get volume name from disk driver
      GetVolumeNameForVolumeMountPoint(path.c_str(), volume_name, MAX_PATH);

      Volume_Info vol_info;
      vol_info.fs_device = volume_name;
      vol_info.fs_path = path;
      vol_info.fs_type = volume_type;
      volume_info_vec->push_back(vol_info);
      volume_set.emplace(path);
    }
  }

  HANDLE hFind;
  WCHAR volume_path[MAX_PATH] = L"";
  common::unordered_set<os_string>::iterator it;
  // enum all volume with driver name to find volume which don't named
  for (it = volume_set.begin(); it != volume_set.end(); it++) {
    hFind = FindFirstVolumeMountPoint(it->c_str(), volume_path, MAX_PATH);
    if (hFind != INVALID_HANDLE_VALUE) {
      do {
        os_string path = *it + volume_path;
        GetVolumeNameForVolumeMountPoint(path.c_str(), volume_name, MAX_PATH);
        GetVolumeInformation(path.c_str(), NULL, 0, NULL, 0, NULL, volume_type,
                             MAX_PATH);

        Volume_Info vol_info;
        vol_info.fs_device = volume_name;
        vol_info.fs_path = path;
        vol_info.fs_type = volume_type;
        volume_info_vec->push_back(vol_info);
      } while (FindNextVolumeMountPoint(hFind, volume_path, MAX_PATH));
      FindVolumeMountPointClose(hFind);
    }
  }
#endif
  return !volume_info_vec->empty();
}

bool VolumeHelper::GetVolumeInfo(VolumeMap *volume_info_map) {
  vector<Volume_Info> vec_volume_info;
  if (!GetVolumeInfo(&vec_volume_info)) {
    return false;
  }

  if (!PraseVolumeInfo(vec_volume_info, volume_info_map)) {
    return false;
  }

  return true;
}

b64 VolumeHelper::GetVolumeSpace(const os_string &volume_path) {
#ifdef __linux__
  struct statvfs64 vfs_info;
  if (statvfs64(volume_path.c_str(), &vfs_info) < 0) {
    return 0;
  }

  return (vfs_info.f_files - vfs_info.f_ffree);
#else
  ULARGE_INTEGER li_total_space = {0};
  ULARGE_INTEGER li_free_space = {0};
  if (!GetDiskFreeSpaceEx(volume_path.c_str(), NULL, &li_total_space,
                          &li_free_space)) {
    return 0;
  }

  return (li_total_space.QuadPart - li_free_space.QuadPart);
#endif
}

bool VolumeHelper::PraseVolumeInfo(const vector<Volume_Info> &volume_info_vec,
                                   VolumeMap *volume_info_map) {
  os_string volume_path;
  os_string mounted_path;
  ScannerFactory sc;
  for (vector<Volume_Info>::const_iterator it = volume_info_vec.begin();
       it != volume_info_vec.end(); it++) {
    // get type by name
    fs_type type = sc.GetFileSystemType(it->fs_type);
    // when volume not support, set volume scanner to default
    if (type == OTHER) {
      volume_path = it->fs_path;
      mounted_path = common::empty;
    } else {
      volume_path = it->fs_device;
      mounted_path = it->fs_path;
    }
#ifdef _WIN32
    // remove end separator
    volume_path = volume_path.substr(0, volume_path.length() - 1);
#endif
    if (volume_info_map->find(volume_path) == volume_info_map->end()) {
      Volume_Info2 vol_Info;
      vol_Info.fs_path = mounted_path;
      vol_Info.fs_type = type;
      vol_Info.fs_size = GetVolumeSpace(it->fs_path);
      volume_info_map->emplace(volume_path, vol_Info);
    }
  }
  return !volume_info_map->empty();
}

//  if path contains soft link, return the path in link path
//  if path contains mount point, return the path in volume
bool VolumeHelper::GetActualPathByPath(const os_string &path,
                                       os_string *act_path) {
#ifdef __linux__
  char *finalfilepath = realpath(path.c_str(), NULL);
  if (finalfilepath) {
    *act_path = finalfilepath;
    free(finalfilepath);
    return true;
  }
#else
  bool find = false;
  HANDLE hFile =
      CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                 NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
  if (hFile != INVALID_HANDLE_VALUE) {
    WCHAR finalfilepath[MAX_PATH] = {0};
    if (GetFinalPathNameByHandle(hFile, finalfilepath, MAX_PATH,
                                 FILE_NAME_NORMALIZED)) {
      *act_path = finalfilepath + 4;
      find = true;
    }
    CloseHandle(hFile);
    return find;
  }
#endif
  return false;
}
}  // namespace filescanner
#include "default_scanner.h"

#include "tool/resource.h"

#ifdef __linux__
#include <dirent.h>
#include <stdio.h>
#else
#include <windows.h>
#endif

namespace filescanner {
namespace other {
CDefaultScanner::CDefaultScanner(const os_string &path,
                                 CDiskScanner *p_disk_scanner,
                                 IDataHanderInterface *p_data_handler,
                                 CFolderSelector *p_folder_selector)
    : volume_name_(path),
      p_disk_scanner_(p_disk_scanner),
      p_folder_selector_(p_folder_selector),
      p_data_handler_(p_data_handler) {
#ifndef __linux__
  pNtQueryDirectoryFile_ = NULL;
#endif
}

bool CDefaultScanner::Scan() {
#ifndef __linux__
  bool result = false;
  scope::ScopedModule ntdll(LoadLibrary(L"ntdll.dll"));
  if (ntdll) {
    pNtQueryDirectoryFile_ =
        (PNTQUERYDIRECTORYFILE)GetProcAddress(ntdll, "NtQueryDirectoryFile");
    if (pNtQueryDirectoryFile_) {
      result = FasterTraverseDir(volume_name_);
    } else {
      result = TraverseDir(volume_name_);
    }
    return result;
  }
#endif
  return TraverseDir(volume_name_);
}

#ifndef __linux__
bool CDefaultScanner::TraverseDir(const os_string &path) {
  vector<os_string> vec_dir;
  HANDLE hFind;
  WIN32_FIND_DATA find_data;
  vec_dir.emplace_back(path);

  os_string directory;
  while (!vec_dir.empty()) {
    directory = vec_dir.back() + L"\\*";
    vec_dir.pop_back();

    if (p_disk_scanner_->scan_canceled()) break;

    hFind = FindFirstFile(directory.c_str(), &find_data);
    if (hFind != INVALID_HANDLE_VALUE) {
      do {
        if (find_data.cFileName[0] == L'$' ||
            wcscmp(find_data.cFileName, L".") == 0 ||
            wcscmp(find_data.cFileName, L"..") == 0)
          continue;

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED ||
            find_data.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED)
          continue;

        os_string filepath =
            directory.substr(0, directory.length() - 1) + find_data.cFileName;

        if (p_folder_selector_) {
          if (!p_folder_selector_->NeedScan(filepath)) {
            continue;
          }
        }
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
          if (find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            continue;
          } else {
            vec_dir.push_back(filepath);
          }
        } else {
          b64 filesize = find_data.nFileSizeLow | (b64)find_data.nFileSizeHigh
                                                      << 32;
          p_data_handler_->OnFilePath(filepath, filesize);
          if (filesize == 0) {
            continue;
          } else {
            if (p_data_handler_->FileContentNeed()) {
              scope::ScopedHandle fd(
                  CreateFile(filepath.c_str(), GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL));
              if (fd != INVALID_HANDLE_VALUE) {
                scope::ScopedPtr<b8> content(new b8[kBlockSize]);

                ulong dwLen = 0;
                while (ReadFile(fd, content, kBlockSize, &dwLen, NULL)) {
                  b32 res =
                      p_data_handler_->OnFileContent(filepath, content, dwLen);
                  if (res <= 0) {
                    break;
                  }
                }
              }
            }

            if (p_folder_selector_) {
              p_folder_selector_->set_not_empty();
            }
          }

          filesize = (filesize + 4095) / 4096 * 4096;
          p_disk_scanner_->UpdateScannedSpace(filesize);
          p_disk_scanner_->UpdateScannedProcess(1);
        }
      } while (FindNextFile(hFind, &find_data));
    }
    FindClose(hFind);
  }

  return true;
}

bool CDefaultScanner::FasterTraverseDir(const os_string &path) {
  vector<os_string> vec_dir;
  vec_dir.emplace_back(path);

  os_string directory;
  scope::ScopedPtr<b8> query_buffer;
  ulong query_buffer_size;
  query_buffer_size =
      sizeof(FILE_DIRECTORY_INFORMATION) + MAX_PATH * sizeof(WCHAR);
  query_buffer_size *= 16;

  query_buffer = new b8[query_buffer_size];
  if (!query_buffer) {
    return false;
  }

  while (!vec_dir.empty()) {
    directory = vec_dir.back() + L"\\";
    vec_dir.pop_back();

    if (p_disk_scanner_->scan_canceled()) break;

    scope::ScopedHandle hFind(
        CreateFile(directory.c_str(), SYNCHRONIZE | FILE_LIST_DIRECTORY,
                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                   OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL));
    if (hFind != INVALID_HANDLE_VALUE) {
      IO_STATUS_BLOCK IoStatusBlock;
      do {
        NTSTATUS nt_status = pNtQueryDirectoryFile_(
            hFind, NULL, NULL, NULL, &IoStatusBlock, query_buffer,
            query_buffer_size, FileDirectoryInformation, FALSE, NULL, FALSE);

        if (nt_status == 0) {
          PFILE_DIRECTORY_INFORMATION file_info =
              (PFILE_DIRECTORY_INFORMATION)query_buffer.get();
          for (; (b8 *)file_info < query_buffer.get() + query_buffer_size;
               file_info = (PFILE_DIRECTORY_INFORMATION)(
                   ((b8 *)file_info) + file_info->NextEntryOffset)) {
            os_string filepath(path);
            b64 file_size = file_info->EndOfFile.QuadPart;
            if (file_info->FileName[0] == L'$' ||
                (wcsncmp(file_info->FileName, L".", 1) == 0 &&
                 file_info->FileNameLength == 2) ||
                (wcsncmp(file_info->FileName, L"..", 2) == 0 &&
                 file_info->FileNameLength == 4))
              goto NEXT;

            if (file_info->FileAttributes & FILE_ATTRIBUTE_COMPRESSED ||
                file_info->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED)
              goto NEXT;

            filepath.append(file_info->FileName,
                            file_info->FileNameLength / sizeof(WCHAR));
            if (p_folder_selector_) {
              if (!p_folder_selector_->NeedScan(filepath)) {
                goto NEXT;
              }
            }
            if (file_info->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
              if (file_info->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
                goto NEXT;
              }
              vec_dir.push_back(filepath);
            } else {
              p_data_handler_->OnFilePath(filepath, file_size);
              if (file_size == 0) {
                goto NEXT;
              } else {
                if (p_data_handler_->FileContentNeed()) {
                  scope::ScopedHandle fd(
                      CreateFile(filepath.c_str(), GENERIC_READ,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL));
                  if (fd != INVALID_HANDLE_VALUE) {
                    scope::ScopedPtr<b8> content(new b8[kBlockSize]);

                    ulong dwLen = 0;
                    while (ReadFile(fd, content, kBlockSize, &dwLen, NULL)) {
                      b32 res = p_data_handler_->OnFileContent(filepath,
                                                               content, dwLen);
                      if (res <= 0) {
                        break;
                      }
                    }
                  }
                }

                if (p_folder_selector_) {
                  p_folder_selector_->set_not_empty();
                }
              }
            }
          NEXT:
            file_size = (file_size + 4095) / 4096 * 4096;
            p_disk_scanner_->UpdateScannedSpace(file_size);
            p_disk_scanner_->UpdateScannedProcess(1);
            if (!file_info->NextEntryOffset) {
              break;
            }
          }
        } else {
#define STATUS_NO_SUCH_FILE 0xC000000FL
#define STATUS_NO_MORE_FILES 0x80000006L
          if (nt_status == STATUS_NO_MORE_FILES ||
              nt_status == STATUS_NO_SUCH_FILE) {
            break;
          }
        }
      } while (true);
    }
  }

  return true;
}
#else
bool CDefaultScanner::TraverseDir(const os_string &path) {
  vector<os_string> vec_dir;
  vec_dir.push_back(path);

  struct dirent *p_file_info = NULL;
  scope::ScopedDir p_dir;

  os_string directory;
  while (!vec_dir.empty()) {
    directory = vec_dir.back();
    vec_dir.pop_back();

    if (p_disk_scanner_->scan_canceled()) break;

    p_dir = opendir(directory.c_str());
    if (p_dir) {
      while (NULL != (p_file_info = readdir(p_dir))) {
        os_string filepath = directory + p_file_info->d_name;
        if (strcmp(p_file_info->d_name, ".") == 0 ||
            strcmp(p_file_info->d_name, "..") == 0)
          goto NEXT;

        if (p_file_info->d_type == DT_FIFO || p_file_info->d_type == DT_SOCK ||
            p_file_info->d_type == DT_LNK || p_file_info->d_type == DT_CHR ||
            p_file_info->d_type == DT_BLK)
          goto NEXT;

        if (p_folder_selector_) {
          if (!p_folder_selector_->NeedScan(filepath)) {
            goto NEXT;
          }
        }

        if (p_file_info->d_type == DT_DIR) {
          if (p_disk_scanner_->CheckFileSystemMounted(filepath)) {
            goto NEXT;
          }
          vec_dir.push_back(filepath);
        } else {
          scope::ScopedFile fp(fopen(filepath.c_str(), "rb"));
          if (fp) {
            fseek(fp, 0, SEEK_END);
            p_data_handler_->OnFilePath(filepath, ftell(fp));
            if (p_data_handler_->FileContentNeed()) {
              scope::ScopedPtr<b8> content(new b8[kBlockSize]);

              ulong dwLen = 0;
              fseek(fp, 0, SEEK_SET);
              while (fread(content, kBlockSize, sizeof(b8), fp)) {
                b32 res =
                    p_data_handler_->OnFileContent(filepath, content, dwLen);
                if (res <= 0) {
                  break;
                }
              }
            }

            if (p_folder_selector_) {
              p_folder_selector_->set_not_empty();
            }
          }
        }
      NEXT:
        p_disk_scanner_->UpdateScannedSpace(1);
        p_disk_scanner_->UpdateScannedProcess(1);
      }
    }
  }

  return true;
}
#endif
}  // namespace other
}  // namespace filescanner
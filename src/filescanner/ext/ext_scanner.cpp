#ifdef __linux__
#include "ext_scanner.h"

#include <stdio.h>
#include <unistd.h>

#include "ext_content.h"

namespace filescanner {
namespace ext {

bool CExt4Scanner::TraverseSubEntries(CFileContent *fp,
                                      const std::string &parant_path) {
  if (fp->IsDirectory()) {
    if (fp->PraseBlock()) {
      CIndexList dir_list;
      if (((CDirectory *)fp)->ReadData(&dir_list)) {
        for (CIndexList::iterator it = dir_list.begin(); it != dir_list.end();
             it++) {
          if (it->second.filename == "." || it->second.filename == "..")
            continue;

          os_string filepath = parant_path + "/" + it->second.filename;
          if (p_folder_selector_) {
            if (!p_folder_selector_->NeedScan(filepath)) {
              continue;
            }
          }

          switch (it->second.file_type) {
            case Regular:
              file_list_[it->first] = filepath;
              break;
            case Directory:
            case (b8)Other:
              dir_list_[it->first] = filepath;
              break;
            default:
              break;
          }
        }
        return true;
      }
    }
    return false;
  }
  return true;
}

bool CExt4Scanner::PraseFiles(CFileContent *fp,
                              const std::string &parant_path) {
  if (fp->IsRegularFile()) {
    CFile *fp_new = (CFile *)fp;
    p_data_handler_->OnFilePath(parant_path, fp_new->file_size());
    if (p_data_handler_->FileContentNeed()) {
      if (fp_new->PraseBlock()) {
        scope::ScopedPtr<b8> content(new b8[kBlockSize]);
        b32 dwLen = 0;
        while (fp_new->Read(content, kBlockSize, &dwLen)) {
          b32 res = p_data_handler_->OnFileContent(parant_path, content, dwLen);
          if (res <= 0) {
            break;
          }
        }
      }
    }

    p_disk_scanner_->UpdateScannedSpace((fp_new->file_size() + 4095) / 4096 *
                                        4096);
    p_disk_scanner_->UpdateScannedProcess(1);

    return true;
  }

  for (auto itr = file_list_.begin(); itr != file_list_.end(); itr++) {
    if (p_disk_scanner_->scan_canceled()) break;

    scope::ScopedPtr<CFileContent> fp_new(file_record_.ReadContent(itr->first));
    if (!fp_new) {
      continue;
    }
    fp_new->set_file_mask(MASK_IFREG);

    p_data_handler_->OnFilePath(itr->second, fp_new->file_size());
    if (p_data_handler_->FileContentNeed()) {
      if (fp_new->PraseBlock()) {
        scope::ScopedPtr<b8> content(new b8[kBlockSize]);
        b32 dwLen = 0;
        while (((CFile*)fp_new.get())->Read(content, kBlockSize, &dwLen)) {
          b32 res = p_data_handler_->OnFileContent(itr->second, content, dwLen);
          if (res <= 0) {
            break;
          }
        }
      }
    }

    if (p_folder_selector_) {
      p_folder_selector_->set_not_empty();
    }

    p_disk_scanner_->UpdateScannedSpace((fp_new->file_size() + 4095) / 4096 *
                                        4096);
    p_disk_scanner_->UpdateScannedProcess(1);
  }

  file_list_.clear();
  return true;
}

bool CExt4Scanner::Scan() {
  bool result = ext_volume_.OpenVolume(volume_name_);
  if (!result) {
    return false;
  }

  common::os_string root_path =
      p_disk_scanner_->GetFileSystemMountedPath(volume_name_);
  root_path = (root_path.size() == 1) ? "" : root_path;

  dir_list_[INODE_ROOT] = root_path;

  while (dir_list_.size() > 0) {
    if (p_disk_scanner_->scan_canceled()) break;

    common::map<b64, os_string>::iterator itr = dir_list_.begin();
    if (p_disk_scanner_->CheckFileSystemMounted(itr->second)) {
      dir_list_.erase(itr);
      continue;
    }

    scope::ScopedPtr<CFileContent> fp(file_record_.ReadContent(itr->first));
    if (!fp) {
      dir_list_.erase(itr);
      continue;
    }

    fp->set_file_mask(MASK_IFDIR | MASK_IFREG);

    TraverseSubEntries(fp, itr->second);

    PraseFiles(fp, itr->second);

    dir_list_.erase(itr);
  }

  return true;
}

}  // namespace ext
}  // namespace filescanner
#endif

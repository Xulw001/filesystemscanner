#ifdef _WIN32
#include "ntfs_scanner.h"

#include "ntfs_attribute.h"

namespace filescanner {
namespace ntfs {
bool CNtfsScanner::Scan() {
  bool result = ntfs_volume_.OpenVolume(volume_name_);
  if (!result) {
    return false;
  }

  common::os_string root_path =
      p_disk_scanner_->GetFileSystemMountedPath(volume_name_);
  if (root_path.back() == '\\') {
    root_path.erase(root_path.end() - 1);
  }
  folder_map_[MFT_IDX_ROOT] = root_path;

  while (folder_map_.size() > 0) {
    if (p_disk_scanner_->scan_canceled()) break;

    common::map<b64, os_string>::iterator& itr = folder_map_.begin();
    if (!file_record_.ReadFileRecord(itr->first)) {
      folder_map_.erase(itr);
      continue;
    }

    file_record_.ReadAttrs();

    TraverseSubEntries(itr->second);

    ParseFiles();

    folder_map_.erase(itr);
  }

  return true;
}

void CNtfsScanner::TraverseSubEntries(const os_string& dir_path) {
  CAttrListIter it = file_record_.AttrBegin(INDEX_ROOT);
  if (it == file_record_.AttrEnd(INDEX_ROOT)) {
    return;
  }

  CIndexRootAttr* index = (CIndexRootAttr*)*it;
  if (!index->LoadIndexEntry()) {
    return;
  }

  CIndexList::iterator it_entry = index->begin();
  while (it_entry != index->end()) {
    if (it_entry->IsSubNodePtr()) {
      TraverseSubNode(it_entry->GetSubNodeVCN(), dir_path);
    }

    IndexEntryCallback(*it_entry, dir_path);

    it_entry++;
  }
}

void CNtfsScanner::TraverseSubNode(const b64& vcn, const os_string& dir_path) {
  CAttrListIter it = file_record_.AttrBegin(INDEX_ALLOCATION);
  if (it == file_record_.AttrEnd(INDEX_ALLOCATION)) {
    return;
  }

  CIndexAllocAttr* index = (CIndexAllocAttr*)*it;

  CIndexBlock ib;
  if (index->LoadIndexEntry(vcn, &ib)) {
    CIndexList::iterator it_entry = ib.begin();
    while (it_entry != ib.end()) {
      if (it_entry->IsSubNodePtr()) {
        TraverseSubNode(it_entry->GetSubNodeVCN(), dir_path);
      }

      IndexEntryCallback(*it_entry, dir_path);

      it_entry++;
    }
  }
}

void CNtfsScanner::IndexEntryCallback(const CIndexEntry& ie,
                                      const os_string& dir_path) {
  if (!ie.IsWin32Name()) {
    return;
  }

  WCHAR file_name[MAX_PATH] = {0};
  ie.GetFileName(file_name, MAX_PATH);
  if ((file_name[0] == L'$') || (0 == wcscmp(file_name, L".")) ||
      (0 == wcscmp(file_name, L".."))) {
    return;
  }

  if (0 == wcscmp(file_name, L"System Volume Information")) {
    return;
  }

  os_string wstrFile = dir_path + L"\\" + file_name;
  if (p_folder_selector_) {
    if (!p_folder_selector_->NeedScan(wstrFile)) {
      return;
    }
  }

  if (ie.IsDirectory()) {
    if (ie.IsReparse()) {
      OutputDebugString(wstrFile.c_str());
      return;
    }
    folder_map_[ie.GetMFTReference()] = wstrFile;
  } else {
    // skip the empty files
    b64 file_size = ie.GetFileSize();
    p_data_handler_->OnFilePath(wstrFile, file_size);
    if (file_size > 0) {
      if (p_data_handler_->FileContentNeed()) {
        FileInfo fbi = {wstrFile, file_size};
        file_map_[ie.GetMFTReference()] = fbi;
      }

      if (p_folder_selector_) {
        p_folder_selector_->set_not_empty();
      }
    }
  }
}

void CNtfsScanner::ParseFiles() {
  for (auto& itr = file_map_.begin(); itr != file_map_.end(); itr++) {
    if (p_disk_scanner_->scan_canceled()) break;

    CFileRecord frFile(&ntfs_volume_);
    FileInfo& fbi = itr->second;

    frFile.ReadFileRecord(itr->first);
    frFile.ReadAttrs();

    CBaseAttr* pData = frFile.FindData();
    if (!pData) {
      OutputDebugString(fbi.path.c_str());
      continue;
    }

    scope::ScopedPtr<b8> content(new b8[kBlockSize]);
    b32 dwLen = 0;
    while (pData->Read(content, kBlockSize, &dwLen)) {
      b32 res = p_data_handler_->OnFileContent(fbi.path, content, dwLen);
      if (res <= 0) {
        break;
      }
    }

    common::b64 file_size = (fbi.size + 4095) / 4096 * 4096;
    p_disk_scanner_->UpdateScannedSpace(file_size);
    p_disk_scanner_->UpdateScannedProcess(1);
  }

  file_map_.clear();
}
}  // namespace ntfs
}  // namespace filescanner
#endif
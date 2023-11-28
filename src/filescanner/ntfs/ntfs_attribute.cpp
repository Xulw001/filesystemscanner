#ifdef _WIN32
#include "ntfs_attribute.h"
namespace filescanner {
namespace ntfs {
// Read Content From Resident Attr
bool CResidentAttr::Read(void *buff, b32 size, b32 *length) {
  if (size == 0) return true;

  // file eof
  if (cur_offset_ >= attr_size_) return false;

  // cal content size to read
  if (size + cur_offset_ > attr_size_)
    *length = attr_size_ - cur_offset_;  // Beyond scope
  else
    *length = size;

  memcpy(buff, (b8 *)attr_body_ + cur_offset_, *length);
  cur_offset_ += *length;

  return true;
}

// Move File Pointer In Resident Attr
b64 CResidentAttr::Seek(b64 offset, FilePointer fp) {
  switch (fp) {
    case SK_SET:
      cur_offset_ = offset;
      break;
    case SK_CUR:
      cur_offset_ += offset;
      break;
    case SK_END:
      cur_offset_ = (offset >= 0) ? attr_size_ : attr_size_ + offset;
      break;
  }
  return cur_offset_;
}

// Prase DataRun List
bool CNonResidentAttr::PraseDataRun(b8 **data_run_p, b64 *num_cluster,
                                    b64 *lcn_offset) {
  b8 mark = **data_run_p;
  // data run len
  b8 length = mark & 0x0f;
  // data run offset
  b8 offset = mark >> 4;

  memcpy(num_cluster, (*data_run_p) + sizeof(mark), length);
  if (*num_cluster < 0) {
    return false;
  }

  *lcn_offset = 0;
  // when sparse, offset must be 0;
  if (offset) {
    // check last byte wheather minus,
    if ((b8)(*data_run_p)[length + offset] < 0) {
      *lcn_offset = -1;
    }
    memcpy(lcn_offset, (*data_run_p) + sizeof(mark) + length, offset);
  }

  *data_run_p += (sizeof(mark) + length + offset);
  return true;
}

bool CNonResidentAttr::LoadDataRun() {
  b8 *data_run_p =
      (b8 *)non_resident_header_ + non_resident_header_->data_run_offset;

  b64 lcn = 0;
  b64 lcn_offset = 0;
  b64 num_cluster = 0;
  b64 vcn = 0;
  while (*data_run_p) {
    if (!PraseDataRun(&data_run_p, &num_cluster, &lcn_offset)) {
      return false;
    }
    // cal lcn of data run
    lcn += lcn_offset;

    DataRunEntry entry;
    entry.lcn = lcn_offset ? lcn : -1;
    entry.num_clusters = num_cluster;
    entry.vcn_begin = vcn;
    entry.vcn_end = vcn + num_cluster - 1;
    // update current vcn
    vcn = entry.vcn_end + 1;

    if (entry.vcn_end <=
        non_resident_header_->vcn_end - non_resident_header_->vcn_begin) {
      data_run_.emplace_back(entry);
    } else {
      // Remove entries
      data_run_.clear();
      data_run_.swap(DataRunList());

      return false;
    }
  }
  return true;
}

b32 CNonResidentAttr::ReadCluster(b64 vcn, void *buff, b32 size) {
  if (vcn > non_resident_header_->vcn_end - non_resident_header_->vcn_begin) {
    return -1;
  }

  if (data_run_.empty()) {
    return -1;
  }

  for (DataRunList::const_iterator it = data_run_.begin();
       it != data_run_.end(); it++) {
    if (vcn >= it->vcn_begin && vcn <= it->vcn_end) {
      LARGE_INTEGER fr_addr;
      fr_addr.QuadPart = (it->lcn + vcn - it->vcn_begin) * cluster_size_;
      if (HFILE_ERROR == SetFilePointer(volume_, fr_addr.LowPart,
                                        &fr_addr.HighPart, FILE_BEGIN)) {
        return -1;
      }

      // cal size of this data run lefted
      b32 data_size = (it->vcn_end - vcn + 1) * cluster_size_;
      if (data_size > size) {
        data_size = size;
      }

      b32 length;
      if (ReadFile(volume_, buff, data_size, (LPDWORD)&length, NULL)) {
        return length;
      }
      break;
    }
  }

  return -1;
}

bool CNonResidentAttr::Read(void *buff, b32 size, b32 *length) {
  do {
    *length = 0;
    if (size == 0) {
      break;
    }

    if (!data_load_) {
      if (!LoadDataRun()) {
        break;
      }
      data_load_ = true;
    }

    if (cur_offset_ >= non_resident_header_->byte_alloc) {
      break;  // EOF
    }

    if (cur_offset_ + size > non_resident_header_->byte_alloc) {
      size = non_resident_header_->byte_alloc - cur_offset_;  // Beyond scope
    }

    // if cur is unaligned, read the cluster to buffer
    b32 cluster_offset = cur_offset_ % cluster_size_;
    if (cluster_offset) {
      b64 vcn = cur_offset_ / cluster_size_;
      scope::ScopedPtr<b8> unaligned_(new b8[cluster_size_]);
      if (ReadCluster(vcn, unaligned_, cluster_size_) < 0) {
        return false;
      }

      b32 data_len = cluster_size_ - cluster_offset;
      *length = (data_len > size) ? size : data_len;
      memcpy(buff, unaligned_.get() + cluster_offset, *length);

      size -= (*length);
      cur_offset_ += (*length);
    }

    b8 *cur_buff = (b8 *)buff + (*length);
    do {
      b64 vcn = cur_offset_ / cluster_size_;
      b32 len = ReadCluster(vcn, cur_buff, size);
      if (len < 0) {
        return false;
      }

      cur_buff += len;
      size -= len;
      *length += len;
      // update file pointer
      cur_offset_ += len;
    } while (size);

    return true;
  } while (false);

  return false;
}

b64 CNonResidentAttr::Seek(b64 offset, FilePointer fp) {
  switch (fp) {
    case SK_SET:
      cur_offset_ = offset;
      break;
    case SK_CUR:
      cur_offset_ += offset;
      break;
    case SK_END:
      cur_offset_ = (offset >= 0) ? non_resident_header_->byte_alloc
                                  : non_resident_header_->byte_alloc + offset;
      break;
  }
  return cur_offset_;
}

void CFileName::GetFileTime(FILETIME *tm_write, FILETIME *tm_create,
                            FILETIME *tm_access, FILETIME *tm_change) const {
  UTC2Local(filename_ ? filename_->time_update : 0, tm_write);
  UTC2Local(filename_ ? filename_->time_create : 0, tm_create);
  UTC2Local(filename_ ? filename_->time_access : 0, tm_access);
  UTC2Local(filename_ ? filename_->time_mft_change : 0, tm_change);
}

void CFileName::UTC2Local(const b64 &ulong_time, FILETIME *tm) const {
  LARGE_INTEGER ul_time;
  FILETIME file_tm;
  ul_time.QuadPart = ulong_time;
  file_tm.dwHighDateTime = ul_time.HighPart;
  file_tm.dwLowDateTime = ul_time.LowPart;
  if (!FileTimeToLocalFileTime(&file_tm, tm)) {
    *tm = file_tm;
  }
}

bool CIndexRootAttr::LoadIndexEntry() {
  if (index_root_->attr_type == FILE_NAME) {
    IndexEntry *entry = (IndexEntry *)(((b8 *)&index_root_->entry_offset) +
                                       index_root_->entry_offset);
    b32 entry_size = 0;
    do {
      CIndexList::emplace_back(CIndexEntry(entry));
      if (entry->flags & ENTRY_LAST) {
        break;
      }

      entry_size += entry->size;
      entry = (IndexEntry *)((b8 *)entry + entry->size);
    } while (entry_size < index_root_->total_size);
    return true;
  }
  return false;
}

bool CIndexAllocAttr::LoadIndexEntry(b64 vcn, CIndexBlock *ib) {
  if (non_resident_header_->byte_alloc % idx_block_size_) {
    return false;
  }

  b64 block_count = non_resident_header_->byte_alloc / idx_block_size_;
  if (vcn >= block_count) {
    return false;
  }

  b64 offset = Seek(vcn * cluster_size_, SK_SET);
  if (offset < 0) {
    return false;
  }

  b32 len;
  ib->index_block_ = (IndexBlock *)new b8[idx_block_size_];
  if (Read(ib->index_block_, idx_block_size_, &len)) {
    if (ib->index_block_->magic != kIndexBlockMagic) {
      return false;
    }

    // Patch US
    b16 *usn_addr =
        (b16 *)((b8 *)ib->index_block_.get() + ib->index_block_->usn_offset);
    b16 usn = *usn_addr;
    b16 *usarray = usn_addr + 1;
    if (!PatchUS((b16 *)ib->index_block_.get(), idx_block_size_ / sector_size_,
                 usn, usarray)) {
      return false;
    }

    IndexEntry *ie = (IndexEntry *)(((b8 *)&ib->index_block_->entry_offset) +
                                    ib->index_block_->entry_offset);
    b32 entry_size = 0;
    do {
      ib->emplace_back(CIndexEntry(ie));
      if (ie->flags & ENTRY_LAST) {
        break;
      }

      entry_size += ie->size;
      ie = (IndexEntry *)((b8 *)ie + ie->size);
    } while (entry_size < ib->index_block_->total_size);

    return true;
  }
  return false;
}

bool CIndexAllocAttr::PatchUS(b16 *sector_pointer, b32 sector_num, b16 usn,
                              b16 *us_data) const {
  for (b32 i = 0; i < sector_num; i++) {
    // offset to the end word of sector
    sector_pointer += ((sector_size_ >> 1) - 1);
    if (*sector_pointer != usn) return false;  // USN check
    *sector_pointer = us_data[i];              // set data to update us
    sector_pointer++;
  }
  return true;
}
}  // namespace ntfs
}  // namespace filescanner
#endif
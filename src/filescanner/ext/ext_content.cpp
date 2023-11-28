#ifdef __linux__
#include "ext_content.h"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

namespace filescanner {
namespace ext {
bool CFileContent::PraseBlock() {
  if (file_mask_ & TypeMask(inode_info_->i_mode & 0xF000)) {
    if ((inode_info_->i_mode & 0xF000) == TYPE_IFLNK) {
      if (inode_info_->i_size <= sizeof(inode_info_->i_block))
        // symlink, do nothing
        const b8 *symlink_path = (const b8 *)inode_info_->i_block;
    }
    if ((inode_info_->i_flags & EXT4_INLINE_DATA_FL) &&
        inode_info_->i_blocks <= 1) {
      return PraseInlineData();
    } else {
      if (inode_info_->i_flags & EXT4_EXTENTS_FL)
        return PraseExtentTree();
      else
        return PraseBlockAddressing();
    }
    return true;
  }

  return false;
}

// The inline data feature was designed to handle the case
// that a file's data is so tiny that it readily fits inside the inode.
// If the file is smaller than 60 bytes,
// then the data are stored inline in inode.i_block.
// If the rest of the file would fit inside the extended attribute space,
// then it might be found as an extended attribute "system.data".
// max size = 256 - 156 + 60 = 160
bool CFileContent::PraseInlineData() {
  b32 offset = 0;
  if (inode_info_->i_size <= sizeof(inode_info_->i_block))
    offset = inode_info_->i_size;
  else
    offset = sizeof(inode_info_->i_block);

  memcpy(inline_data_, inode_info_->i_block, offset);
  if (inode_info_->i_size <= (ub32)offset) return true;

  app_ext4_attr_header *attr_header =
      (app_ext4_attr_header *)((b8 *)&inode_info_->i_extra_isize +
                               inode_info_->i_extra_isize);
  if (attr_header->h_magic != kExtAttrMagic) return false;

  // Extended attributes, when stored after the inode,
  // have a header ext4_xattr_ibody_header that is 4 bytes long
  app_ext4_attr_entry *attr_data =
      (app_ext4_attr_entry *)((b8 *)attr_header + sizeof(attr_header->h_magic));
  while (attr_data->e_name_index != kExtAttrDataIdx ||
         attr_data->e_name_len != sizeof(kExtAttrDataName)) {
    attr_data =
        (app_ext4_attr_entry *)((b8 *)attr_data + sizeof(app_ext4_attr_entry) +
                                (attr_data->e_name_len + 3) / 4 * 4);
  }

  // For an inode attribute e_value_offs is relative to the first entry
  if (*(b32 *)((b8 *)attr_data + sizeof(app_ext4_attr_entry)) ==
      kExtAttrDataName) {
    memcpy(inline_data_ + offset,
           (b8 *)attr_header + sizeof(attr_header->h_magic) +
               attr_data->e_value_offs,
           attr_data->e_value_size);
  }

  return true;
}

bool CFileContent::PraseExtentTree() { return PraseExtentTree((b64)0); }

bool CFileContent::PraseExtentTree(const b64 block_start) {
  bool ret = false;
  app_ext4_extent_header *extent_header = NULL;
  do {
    if (block_start) {
      if (lseek64(ext_volume_->fd_, block_start * ext_volume_->block_size_,
                  SEEK_SET) < 0)
        break;

      extent_header =
          (app_ext4_extent_header *)new b8[ext_volume_->block_size_];
      if (read(ext_volume_->fd_, extent_header, ext_volume_->block_size_) !=
          ext_volume_->block_size_)
        break;
    } else
      extent_header = (app_ext4_extent_header *)inode_info_->i_block;

    if (extent_header->eh_magic != kExtExtentMagic) break;
    if (extent_header->eh_depth == 0)
      ret = PraseExtent(extent_header);
    else
      ret = PraseExtentIndex(extent_header);
  } while (false);

  if (block_start && extent_header) delete[](b8 *) extent_header;

  return ret;
}

bool CFileContent::PraseExtent(app_ext4_extent_header *extent_header) {
  app_ext4_extent *extent_data =
      (app_ext4_extent *)((b8 *)extent_header + sizeof(app_ext4_extent_header));
  for (b32 i = 0; i < extent_header->eh_entries; i++) {
    b64 block_idx =
        extent_data->ee_start | ((b64)extent_data->ee_start_hi << 32);
    b32 actual_block = extent_data->ee_len > 32768 ? extent_data->ee_len - 32768
                                                   : extent_data->ee_len;
    for (b32 j = 0; j < actual_block; j++) {
      block_list_.push_back(block_idx++);
    }
    extent_data =
        (app_ext4_extent *)((b8 *)extent_data + sizeof(app_ext4_extent));
  }

  return true;
}

bool CFileContent::PraseExtentIndex(app_ext4_extent_header *extent_header) {
  app_ext4_extent_idx *extent_idx =
      (app_ext4_extent_idx *)((b8 *)extent_header +
                              sizeof(app_ext4_extent_header));
  for (int i = 0; i < extent_header->eh_entries; i++) {
    b64 block_idx = extent_idx->ei_leaf | ((b64)extent_idx->ei_leaf_hi << 32);
    if (!PraseExtentTree(block_idx)) return false;

    extent_idx =
        (app_ext4_extent_idx *)((b8 *)extent_idx + sizeof(app_ext4_extent_idx));
  }

  return true;
}

bool CFileContent::PraseBlockAddressing() {
  ub64 file_size = inode_info_->i_size | ((ub64)inode_info_->i_size_high << 32);
  total_block_count_ =
      (file_size + ext_volume_->block_size_ - 1) / ext_volume_->block_size_;

  // data block number need read
  b64 read_block = 0;
  for (int i = 0; i < EXT4_N_BLOCKS && read_block < total_block_count_; i++) {
    if (i < 12) {
      block_list_.push_back(inode_info_->i_block[i]);
      read_block++;
    } else if (i < 13) {
      if (!PraseIndirectBlock(inode_info_->i_block[i], &read_block))
        return false;
    } else if (i < 14) {
      if (!PraseDoubleIndirectBlock(inode_info_->i_block[i], &read_block))
        return false;
    } else {
      if (!PraseTripleIndirectBlock(inode_info_->i_block[i], &read_block))
        return false;
    }
  }
  return true;
}

bool CFileContent::PraseIndirectBlock(const b64 block_index, b64 *block_count) {
  scope::ScopedPtr<b32> direct_index;
  do {
    if (lseek64(ext_volume_->fd_, block_index * ext_volume_->block_size_,
                SEEK_SET) < 0)
      break;

    direct_index = (b32 *)new b8[ext_volume_->block_size_];
    if (ext_volume_->block_size_ !=
        read(ext_volume_->fd_, direct_index, ext_volume_->block_size_))
      break;

    for (b32 i = 0; *block_count < total_block_count_ &&
                    (ub64)i < ext_volume_->block_size_ / sizeof(b32);
         i++) {
      block_list_.push_back(direct_index[i]);
      ++(*block_count);
    }
    return true;
  } while (false);
  return false;
}

bool CFileContent::PraseDoubleIndirectBlock(const b64 block_index,
                                            b64 *block_count) {
  scope::ScopedPtr<b32> direct_index;
  do {
    if (lseek64(ext_volume_->fd_, block_index * ext_volume_->block_size_,
                SEEK_SET) < 0)
      break;

    direct_index = (b32 *)new b8[ext_volume_->block_size_];
    if (ext_volume_->block_size_ !=
        read(ext_volume_->fd_, direct_index, ext_volume_->block_size_))
      break;

    for (b32 i = 0; *block_count < total_block_count_ &&
                    (ub64)i < ext_volume_->block_size_ / sizeof(b32);
         i++) {
      if (!PraseIndirectBlock(direct_index[i], block_count)) break;
      (*block_count)++;
    }
    return true;
  } while (false);
  return false;
}

bool CFileContent::PraseTripleIndirectBlock(const b64 block_index,
                                            b64 *block_count) {
  scope::ScopedPtr<b32> direct_index;
  do {
    if (lseek64(ext_volume_->fd_, block_index * (b64)ext_volume_->block_size_,
                SEEK_SET) < 0)
      break;

    direct_index = (b32 *)new b8[ext_volume_->block_size_];
    if (ext_volume_->block_size_ !=
        read(ext_volume_->fd_, direct_index, ext_volume_->block_size_))
      break;

    for (b32 i = 0; *block_count < total_block_count_ &&
                    (ub64)i < ext_volume_->block_size_ / sizeof(b32);
         i++) {
      if (!PraseDoubleIndirectBlock(direct_index[i], block_count)) break;
      (*block_count)++;
    }
    return true;
  } while (false);
  return false;
}

b64 CFile::Seek(b64 offset, FilePointer fp) {
  switch (fp) {
    case SK_SET:
      cur_offset_ = offset;
      break;
    case SK_CUR:
      cur_offset_ += offset;
      break;
    case SK_END:
      cur_offset_ = (offset >= 0) ? file_size_ : file_size_ + offset;
      break;
  }
  return cur_offset_;
}

bool CFile::Read(void *buff, b32 size, b32 *length) {
  if (cur_offset_ >= file_size_) {
    return false;
  }

  if (cur_offset_ + size > file_size_) {
    size = file_size_ - cur_offset_;
  }
  *length = size;

  if ((inode_info_->i_flags & EXT4_INLINE_DATA_FL) &&
      inode_info_->i_blocks <= 1) {
    memcpy(buff, inline_data_ + cur_offset_, size);
  } else {
    b64 block_begin = cur_offset_ / ext_volume_->block_size_;
    b32 block_offset = cur_offset_ % ext_volume_->block_size_;
    for (b32 idx = block_begin; size > 0 && idx < (b32)block_list_.size();
         idx++) {
      if (lseek64(ext_volume_->fd_,
                  block_list_[idx] * (b64)ext_volume_->block_size_ + block_offset,
                  SEEK_SET) < 0)
        return false;

      b64 length_read = ext_volume_->block_size_ - block_offset;
      if (length_read > size) length_read = size;
      if (length_read != read(ext_volume_->fd_, buff, length_read))
        return false;

      buff = (b8 *)buff + length_read;
      size -= length_read;
      block_offset = 0;
    }
  }

  cur_offset_ += (*length);

  return true;
}

bool CDirectory::ReadData(CIndexList *dirlist) {
  scope::ScopedPtr<b8> dir_entry_data;
  if (((inode_info_->i_flags & EXT4_INLINE_DATA_FL) &&
       inode_info_->i_blocks <= 1)) {
    return ReadLinear(inline_data_ + 4, dirlist);
  }
  for (b32 i = 0; i < (b32)block_list_.size(); i++) {
    if (lseek64(ext_volume_->fd_,
                block_list_[i] * (b64)ext_volume_->block_size_, SEEK_SET) < 0)
      return false;

    if (!dir_entry_data) dir_entry_data = new b8[ext_volume_->block_size_];

    if (ext_volume_->block_size_ !=
        read(ext_volume_->fd_, dir_entry_data, ext_volume_->block_size_))
      return false;

    if (!ReadLinear(dir_entry_data, dirlist)) return false;

    if (inode_info_->i_flags & EXT4_INDEX_FL) {
      if (!ReadIndex((b8 *)dir_entry_data + 24, 0, 0, dirlist)) return false;

      break;
    }
  }

  return true;
}

bool CDirectory::ReadIndex(const void *entry_data, b32 depth, b32 cur_depth,
                           CIndexList *dirlist) {
  scope::ScopedPtr<b8> dir_entry_data;
  app_ext4_dx_info *dx_info = NULL;
  if (depth) {
    app_ext4_dir_entry *dir_entry = (app_ext4_dir_entry *)entry_data;
    if (dir_entry->inode || dir_entry->name_len) return false;

    dx_info = (app_ext4_dx_info *)((b8 *)dir_entry + 8);
    ++cur_depth;
  } else {
    app_ext4_dx_header *dx_header = (app_ext4_dx_header *)entry_data;
    depth = dx_header->indirect_levels;
    dx_info = (app_ext4_dx_info *)((b8 *)dx_header + dx_header->info_length);
    cur_depth = 0;
  }

  dir_entry_data = new b8[ext_volume_->block_size_];
  app_ext4_dx_entry *dx_entry =
      (app_ext4_dx_entry *)((b8 *)dx_info + sizeof(app_ext4_dx_info));
  for (int i = 0; i < dx_info->count; i++) {
    if (i) {
      if (lseek64(ext_volume_->fd_,
                  block_list_[dx_entry->block] * (b64)ext_volume_->block_size_,
                  SEEK_SET) < 0)
        return false;
    } else {
      if (lseek64(ext_volume_->fd_,
                  block_list_[dx_info->block] * (b64)ext_volume_->block_size_,
                  SEEK_SET) < 0)
        return false;
    }

    if (ext_volume_->block_size_ !=
        read(ext_volume_->fd_, dir_entry_data, ext_volume_->block_size_))
      return false;

    if (depth == cur_depth) {
      if (!ReadLinear(dir_entry_data, dirlist)) return false;
    } else {
      if (!ReadIndex(dir_entry_data, depth, cur_depth, dirlist)) return false;
    }

    if (i)
      dx_entry =
          (app_ext4_dx_entry *)((b8 *)dx_entry + sizeof(app_ext4_dx_entry));
  }

  return true;
}

bool CDirectory::ReadLinear(const void *entry_data, CIndexList *dirlist) {
  b32 offset = 0;
  app_ext4_dir_entry *dir_entry = (app_ext4_dir_entry *)entry_data;
  while (dir_entry->rec_len != 0x00 && offset < ext_volume_->block_size_) {
    if (dir_entry->inode != 0x00) {
      std::string name(dir_entry->name);
      name.resize(dir_entry->name_len);
      b8 file_type = ext_volume_->file_type_ ? dir_entry->file_type : (b8)Other;
      dirlist->operator[]((b64)dir_entry->inode) = {name, file_type};
    }

    offset += dir_entry->rec_len;
    dir_entry = (app_ext4_dir_entry *)((b8 *)entry_data + offset);
  }

  return true;
}
}  // namespace ext
}  // namespace filescanner
#endif
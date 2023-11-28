#ifdef __linux__
#include "ext_record.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ext_common.h"
#include "ext_content.h"

namespace filescanner {
namespace ext {
CExt4Volume::CExt4Volume() { fd_ = -1; }

CExt4Volume::~CExt4Volume() {
  if (fd_ != -1) {
    close(fd_);
  }
}

bool CExt4Volume::OpenVolume(os_string fs_name) {
  fd_ = open(fs_name.c_str(), O_RDONLY);
  if (fd_) {
    app_ext4_super_block ext4_sb;
    if (lseek(fd_, kExtBootSize, SEEK_SET) == kExtBootSize) {
      // Read super block
      if (read(fd_, &ext4_sb, sizeof(ext4_sb)) == sizeof(ext4_sb)) {
        if (ext4_sb.s_magic == kExtSuperMagic) {
          // Log important volume parameters
          inode_size_ = ext4_sb.s_inode_size;
          inode_count_ = ext4_sb.s_inodes_per_group;
          block_size_ = 1 << (10 + ext4_sb.s_log_block_size);
          if (ext4_sb.s_feature_ro_compat & RO_COMPAT_BIGALLOC) {
            // TODO support big alloc
            return false;
          }

          if (ext4_sb.s_feature_incompat & INCOMPAT_FILETYPE) {
            file_type_ = true;
          }

          group_size_ = block_size_ * ext4_sb.s_blocks_per_group;
          // If COMPAT_SPARSE_SUPER2 is set,
          // only s_backup_bgs points to bgs that contain backup
          if (ext4_sb.s_feature_compat & COMPAT_SPARSE_SUPER2) {
            sparse_super_version_ = 2;
            backup_block_group_[0] = ext4_sb.s_backup_bgs[0];
            backup_block_group_[1] = ext4_sb.s_backup_bgs[1];
          } else if (ext4_sb.s_feature_ro_compat & RO_COMPAT_SPARSE_SUPER) {
            sparse_super_version_ = 1;
          } else {
            sparse_super_version_ = 0;
          }

          if (ext4_sb.s_feature_incompat & INCOMPAT_META_BG) {
            meta_group_ = true;
          } else {
            meta_group_ = false;
          }

          if (ext4_sb.s_feature_incompat & INCOMPAT_64BIT) {
            gdt_size_ = ext4_sb.s_desc_size;
            extend64_ = true;
          } else {
            gdt_size_ = sizeof(app_ext4_group_desc);
            extend64_ = false;
          }

        } else {
          goto IOError;
        }
      }
    } else {
      goto IOError;
    }

    gdt_count_ = block_size_ / gdt_size_;
  } else {
  IOError:
    return false;
  }

  return true;
}

bool isPowerN(ub64 val, ub64 n) {
  while (val && val % n == 0) {
    val = val / n;
  };

  return val == 1;
}

b64 CFileRecord::GetGDTOffset(b64 group_no) {
  // cal offset of bg which contains gdt
  b64 gdt_offset = volume_->group_size_ * group_no;
  if (group_no) {
    // cal group if cotains super block
    switch (volume_->sparse_super_version_) {
      case 2:
        if (group_no == volume_->backup_block_group_[0] ||
            group_no == volume_->backup_block_group_[1]) {
          gdt_offset += volume_->block_size_;
        }
        break;
      case 1: {
        if (isPowerN(group_no, 3) || isPowerN(group_no, 5) ||
            isPowerN(group_no, 7)) {
          gdt_offset += volume_->block_size_;
        }
      } break;
      default:
        break;
    }
  } else {
    gdt_offset += volume_->block_size_;
    // if booter and spuer block no in same block
    if (volume_->block_size_ == kExtMinBlockSize) {
      gdt_offset += kExtMinBlockSize;
    }
  }

  return gdt_offset;
}

// if inode_no = 357, inode_count_ = 8192, so bg = (357 - 1) / 8192 = 0
// then the gdt_count = 4096 / 32 = 128, so gdt block = 0 / 128 = 0,
// and the index in gdt block = 0 % 128 = 0
// so the gdt of 357 is at first gdt block in first bg.
// if inode_no = 357, inode_count_ = 8192, so inode index in bg = (357 - 1) %
// 8192 = 356 the the inode_count in one inode table = 4096 / 256 = 16, so inode
// table block of 357 in bg = 356 / 16 = 22, the inode offset of 357 in it table
// 22 = 356 % 16 = 4.
app_ext4_inode *CFileRecord::ReadFileRecord(const b64 &inode_no) {
  // inode 0 is defined but not exist, so actual inode no begin with 1.
  // the bg number of the inode_no
  b32 bg_no = (inode_no - 1) / volume_->inode_count_;
  // the gdt number in bg
  b32 gdt_block_no = bg_no / volume_->gdt_count_;
  // the index of gdt in the bg which this inode in
  b32 gdt_index = bg_no % volume_->gdt_count_;
  // the index of inode in the bg which this inode in
  b32 inode_partition = (inode_no - 1) % volume_->inode_count_;
  // the inode count in one IT block
  b32 it_inode_count = volume_->block_size_ / volume_->inode_size_;
  // the index of IT block in the bg which this inode in
  b32 inode_block_no = inode_partition / it_inode_count;
  // move file pointer to gdt block
  if (gdt_block_no != gdt_block_no_ || !gdt_record_ || !inode_record_) {
    b64 file_offset = 0;
    if (volume_->meta_group_)
      file_offset = GetGDTOffset(gdt_block_no * (b64)volume_->gdt_count_);
    else
      // use gdt in first bg
      file_offset = GetGDTOffset(0) + gdt_block_no * (b64)volume_->block_size_;
    if (lseek64(volume_->fd_, file_offset, SEEK_SET) != file_offset) goto IOErr;

    gdt_record_ = (app_ext4_group_desc *)new char[volume_->block_size_];
    if (volume_->block_size_ !=
        read(volume_->fd_, gdt_record_, volume_->block_size_))
      goto IOErr;

    // get offset of block which inode in
    if (!volume_->extend64_) {
      file_offset = (gdt_record_[gdt_index].bg_inode_table + inode_block_no) *
                    (b64)volume_->block_size_;
    } else {
      app_ext4_group_desc64 *gdt_record =
          (app_ext4_group_desc64 *)((char *)gdt_record_.get() +
                                    volume_->gdt_size_ * gdt_index);
      b64 inode_table_no = gdt_record->bg_inode_table |
                           ((b64)gdt_record->bg_inode_table_hi << 32);
      file_offset = (inode_table_no + inode_block_no) * volume_->block_size_;
    }
    if (lseek64(volume_->fd_, file_offset, SEEK_SET) != file_offset) goto IOErr;

    inode_record_ = (app_ext4_inode *)new char[volume_->block_size_];
    if (volume_->block_size_ !=
        read(volume_->fd_, inode_record_, volume_->block_size_))
      goto IOErr;

    gdt_block_no_ = gdt_block_no;
    inode_block_no_ = inode_block_no;
  } else if (inode_block_no != inode_block_no_) {
    b64 file_offset = 0;
    if (!volume_->extend64_) {
      file_offset = (gdt_record_[gdt_index].bg_inode_table + inode_block_no) *
                    (b64)volume_->block_size_;
    } else {
      app_ext4_group_desc64 *gdt_record =
          (app_ext4_group_desc64 *)((char *)gdt_record_.get() +
                                    volume_->gdt_size_ * gdt_index);
      b64 inode_table_no = gdt_record->bg_inode_table |
                           ((b64)gdt_record->bg_inode_table_hi << 32);
      file_offset = (inode_table_no + inode_block_no) * volume_->block_size_;
    }
    if (lseek64(volume_->fd_, file_offset, SEEK_SET) != file_offset) goto IOErr;

    if (volume_->block_size_ !=
        read(volume_->fd_, inode_record_, volume_->block_size_))
      goto IOErr;

    inode_block_no_ = inode_block_no;
  }

  return (app_ext4_inode *)((char *)inode_record_.get() +
                            volume_->inode_size_ *
                                (inode_partition % it_inode_count));

IOErr:
  if (inode_record_) {
    inode_record_ = NULL;
  }

  if (gdt_record_) {
    gdt_record_ = NULL;
  }

  return NULL;
}

CFileContent *CFileRecord::ReadContent(const b64 &inode_no) {
  app_ext4_inode *inode = ReadFileRecord(inode_no);
  if (inode) {
    return new CFileContent(inode, this->volume_);
  }
  return NULL;
}
}  // namespace ext
}  // namespace filescanner
#endif
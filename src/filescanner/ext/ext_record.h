#ifdef __linux__
#ifndef _EXT_RECORD_H
#define _EXT_RECORD_H
#include "ext_structure.h"
#include "tool/resource.h"
#include "common/string.h"
#include "ext_structure.h"

namespace filescanner {
namespace ext {
class CFileRecord;
class CFileContent;
class CFile;
class CDirectory;

class CExt4Volume {
  friend class CFileRecord;
  friend class CFileContent;
  friend class CFile;
  friend class CDirectory;

 public:
  CExt4Volume();
  ~CExt4Volume();
  bool OpenVolume(os_string fs_name);

 private:
  int fd_;
  b64 block_size_;
  b64 group_size_;

  b32 inode_size_;
  b64 inode_count_;  // inode number in per block group

  b32 sparse_super_version_;
  b32 backup_block_group_[2];

  bool meta_group_;
  bool extend64_;

  bool file_type_;

  b32 gdt_size_;   // the size of block group descriptors
  b32 gdt_count_;  // the count of gdt in group
};

class CFileRecord {
 public:
  CFileRecord(const CExt4Volume *volume) : volume_(volume) {
    gdt_block_no_ = -1;
    inode_block_no_ = -1;
  }

  CFileContent *ReadContent(const b64 &inode_no);
 private:
  app_ext4_inode *ReadFileRecord(const b64 &inode_no);
  b64 GetGDTOffset(b64 group_no);

 private:
  const CExt4Volume *volume_;
  scope::ScopedPtr<app_ext4_group_desc> gdt_record_;
  scope::ScopedPtr<app_ext4_inode> inode_record_;
  b64 gdt_block_no_;
  b64 inode_block_no_;
};
}  // namespace ext
}  // namespace filescanner
#endif
#endif

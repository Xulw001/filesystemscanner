#ifdef __linux__
#ifndef _EXT_CONTENT_H
#define _EXT_CONTENT_H

#include <vector>

#include "common/map.h"
#include "ext_record.h"

namespace filescanner {
namespace ext {
typedef std::vector<b64> CFileBlockList;
class CFileContent {
 public:
  CFileContent(const app_ext4_inode *inode, const CExt4Volume *volume)
      : inode_info_(inode), ext_volume_(volume) {
    cur_offset_ = 0;
    file_size_ = (inode_info_->i_size | (b64)inode_info_->i_size_high << 32);
  }
  virtual ~CFileContent() { ; }

  bool PraseBlock();
  void set_file_mask(ub32 mask) { file_mask_ = mask; }
  bool IsDirectory() { return (inode_info_->i_mode & 0xF000) == TYPE_IFDIR; }
  bool IsRegularFile() { return (inode_info_->i_mode & 0xF000) == TYPE_IFREG; }
  b64 file_size() { return file_size_; }

 protected:
  bool PraseInlineData();
  bool PraseExtentTree();
  bool PraseExtentTree(const b64 block_start);
  bool PraseExtent(app_ext4_extent_header *extent_header);
  bool PraseExtentIndex(app_ext4_extent_header *extent_header);

  bool PraseBlockAddressing();
  bool PraseIndirectBlock(const b64 block_index, b64 *block_count);
  bool PraseDoubleIndirectBlock(const b64 block_index, b64 *block_count);
  bool PraseTripleIndirectBlock(const b64 block_index, b64 *block_count);

  const app_ext4_inode *inode_info_;
  const CExt4Volume *ext_volume_;
  ub32 file_mask_;

  CFileBlockList block_list_;
  // data block number the file or dir owns
  b64 total_block_count_;

  ub8 inline_data_[256];

  b64 cur_offset_;
  b64 file_size_;
};  // CFileContent

class CFile : public CFileContent {
 public:
  CFile(const app_ext4_inode *inode, const CExt4Volume *volume)
      : CFileContent(inode, volume) {}

  bool Read(void *buff, b32 size, b32 *length);
  b64 Seek(b64 offset, FilePointer fp);
  bool HasHardLink() { return inode_info_->i_links_count > 1; }

};  // CFile

typedef struct {
  os_string filename;
  b8 file_type;
} FileInfo;

typedef common::map<b64, FileInfo> CIndexList;
class CDirectory : public CFileContent {
 public:
  CDirectory(const app_ext4_inode *inode, const CExt4Volume *volume)
      : CFileContent(inode, volume) {
    ;
  }
  bool ReadData(CIndexList *dirlist);

 private:
  bool ReadIndex(const void *entry_data, b32 depth, b32 cur_depth,
                 CIndexList *dirlist);
  bool ReadLinear(const void *entry_data, CIndexList *dirlist);

};  // CDirectory
}  // namespace ext
}  // namespace filescanner
#endif
#endif
#ifndef NTFS_FILE_RECORD_H_
#define NTFS_FILE_RECORD_H_
#include <list>

#include "ntfs_structure.h"
#include "tool/resource.h"

namespace filescanner {
namespace ntfs {
class CFileRecord;
class CBaseAttr;
class CNtfsVolume {
  friend class CFileRecord;
  friend class CBaseAttr;

 public:
  bool OpenVolume(const os_string& path);

 private:
  scope::ScopedHandle volume_;

  b16 sector_size_;
  b32 cluster_size_;
  b32 mft_size_;
  b32 idx_size_;
  b64 mft_address_;

  CFileRecord* mft_record_;  // $MFT File Record
  CBaseAttr* mft_data_;      // $MFT Data Attribute
};

template <typename CPtr>
class CPtrList : public std::list<CPtr*> {
 public:
  void clearall() {
    std::list<CPtr*>::iterator it = begin();
    while (it != end()) {
      if (*it != NULL) {
        delete (CPtr*)*it;
      }
      it = erase(it);
    }
    this->clear();
  }

  void merge(const CPtrList& list) {
    this->insert(this->end(), list.begin(), list.end());
  }
};

typedef CPtrList<CBaseAttr> CAttrList;
typedef CAttrList::const_iterator CAttrListIter;

class CFileRecord {
  friend class CBaseAttr;
  template <class ATTR_BASE>
  friend class CAttributeListAttr;

 public:
  CFileRecord(const CNtfsVolume* ntfs_volume) : ntfs_volume_(ntfs_volume) {
    file_record_ = NULL;
    attr_mask_ = (b32)-1;
  }
  ~CFileRecord() {
    if (file_record_) {
      delete file_record_;
    }
  }

  bool ReadFileRecord(b64 file_record_idx);
  bool ReadAttrs();

  CAttrListIter AttrBegin(AttributeTypes type) const {
    b32 idx = AttrIndex(type);
    if (idx > kMaxAttributeType) {
      return attr_list_[0].end();
    }
    return attr_list_[idx].begin();
  }

  CAttrListIter AttrEnd(AttributeTypes type) const {
    b32 idx = AttrIndex(type);
    if (idx > kMaxAttributeType) {
      return attr_list_[0].end();
    }
    return attr_list_[idx].end();
  }

  inline void set_attr_mask(b32 mask) {
    // Standard Information and Attribute List is needed always
    attr_mask_ = mask | MASK_STANDARD_INFORMATION | MASK_ATTRIBUTE_LIST;
  }

  inline b32 cluster_size() const { return ntfs_volume_->cluster_size_; }
  inline b32 idx_size() const { return ntfs_volume_->idx_size_; }
  inline b16 sector_size() const { return ntfs_volume_->sector_size_; }

  CBaseAttr* FindData() const;

 private:
  FileRecordHeader* LoadFileRecord(b64& file_record_idx);

  CBaseAttr* Attr(AttributeHeader* attr_header);

  void ClearAttrs() {
    for (b32 i = 0; i < kMaxAttributeType; i++) {
      attr_list_[i].clearall();
    }
  }

  bool PatchUS(b16* sector_pointer, b32 sector_num, b16 usn,
               b16* us_data) const;

 private:
  const CNtfsVolume* ntfs_volume_;
  b32 attr_mask_;

  FileRecordHeader* file_record_;
  b64 file_record_idx_;
  CAttrList attr_list_[kMaxAttributeType];  // Attributes
};
}  // namespace ntfs
}  // namespace filescanner
#endif
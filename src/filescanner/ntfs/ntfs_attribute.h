#ifdef _WIN32
#ifndef NTFS_ATTR_H_
#define NTFS_ATTR_H_

#include <list>

#include "ntfs_record.h"
namespace filescanner {
namespace ntfs {

class CBaseAttr {
 public:
  CBaseAttr(const AttributeHeader *attr_header, const CFileRecord *file_record)
      : attr_header_(attr_header), file_record_(file_record) {
    volume_ = file_record_->ntfs_volume_->volume_;
  }

  virtual ~CBaseAttr() { ; }

  virtual bool Read(void *buff, b32 size, b32 *length) = 0;
  virtual b64 Seek(b64 offset, FilePointer fp) = 0;

  bool Named() const { return attr_header_->name_length != 0; }

 protected:
  const AttributeHeader *attr_header_;
  const CFileRecord *file_record_;

  HANDLE volume_;
};

class CResidentAttr : public CBaseAttr {
 public:
  CResidentAttr(const AttributeHeader *attr_header,
                const CFileRecord *file_record)
      : CBaseAttr(attr_header, file_record) {
    resident_header_ = (ResidentAttrHeader *)attr_header_;
    attr_body_ = (void *)((b8 *)resident_header_ + resident_header_->offset);
    attr_size_ = resident_header_->length;
    cur_offset_ = 0;
  }

  bool Read(void *buff, b32 size, b32 *length);
  virtual b64 Seek(b64 offset, FilePointer fp);

 protected:
  const ResidentAttrHeader *resident_header_;
  void *attr_body_;
  b32 attr_size_;

 private:
  b32 cur_offset_;
};

class CNonResidentAttr : public CBaseAttr {
 public:
  CNonResidentAttr(const AttributeHeader *attr_header,
                   const CFileRecord *file_record)
      : CBaseAttr(attr_header, file_record) {
    cluster_size_ = file_record_->cluster_size();
    non_resident_header_ = (NonResidentAttrHeader *)attr_header;
    cur_offset_ = 0;
    data_load_ = false;
  }

  ~CNonResidentAttr() { ; }

  bool Read(void *buff, b32 size, b32 *length);
  virtual b64 Seek(b64 offset, FilePointer fp);

 private:
  bool LoadDataRun();
  bool PraseDataRun(b8 **data_run_p, b64 *num_cluster, b64 *lcn_offset);

  b32 ReadCluster(b64 vcn, void *buff, b32 size);

 protected:
  struct DataRunEntry {
    b64 lcn;  // -1: indicate sparse file
    b64 num_clusters;
    b64 vcn_begin;
    b64 vcn_end;
  };

  typedef std::list<DataRunEntry> DataRunList;
  const NonResidentAttrHeader *non_resident_header_;
  DataRunList data_run_;

 private:
  b64 cur_offset_;
  bool data_load_;
  b32 cluster_size_;
};

class CStandardInfoAttr : public CResidentAttr {
 public:
  CStandardInfoAttr(const AttributeHeader *attr_header,
                    const CFileRecord *file_record)
      : CResidentAttr(attr_header, file_record) {
    std_info_ = (StdInformation *)attr_body_;
  }

 private:
  const StdInformation *std_info_;
};

template <class ATTR_BASE>
class CAttributeListAttr : public ATTR_BASE {
  typedef CPtrList<CFileRecord> CFileRecordList;

  void LoadAttrList() {
    CFileRecord *fr = const_cast<CFileRecord *>(file_record_);
    if (fr->file_record_idx_ == (b64)-1) {
      return;
    }

    b32 len;
    AttrList attr_entry;
    b64 offset = 0;
    while (Read(&attr_entry, (b32)sizeof(AttrList), &len)) {
      b32 attrIndex = AttrIndex(attr_entry.attr_type);
      if (attrIndex > kMaxAttributeType) {
        break;
      }

      b64 fr_ref = attr_entry.base_record_ref & 0x0000FFFFFFFFFFFFUL;
      if (fr_ref != fr->file_record_idx_)  // skip self
      {
        b32 mask = AttrMask(attr_entry.attr_type);

        if (mask & fr->attr_mask_)  // skip undefined attr
        {
          CFileRecord *frnew = new CFileRecord(fr->ntfs_volume_);
          fr_list.emplace_back(frnew);

          frnew->attr_mask_ = mask;
          if (!frnew->ReadFileRecord(fr_ref)) {
            break;
          }
          frnew->ReadAttrs();

          fr->attr_list_[attrIndex].merge(frnew->attr_list_[attrIndex]);

          frnew->attr_list_[attrIndex].clear();  // TODO(memory maybe leak)
        }
      }

      offset = Seek(attr_entry.record_length + offset, SK_SET);
    }
  }

 public:
  CAttributeListAttr(const AttributeHeader *attr_header,
                     const CFileRecord *file_record)
      : ATTR_BASE(attr_header, file_record) {
    LoadAttrList();
  }

  ~CAttributeListAttr() { fr_list.clearall(); }

 private:
  CFileRecordList fr_list;
};  // CAttr_AttrList

class CFileName {
 public:
  CFileName() {
    filename_ = NULL;
    name_length_ch_ = 0;
  }

  inline bool IsWin32Name() const {
    if (!filename_ || name_length_ch_ == 0) {
      return false;
    }

    return (filename_->name_space != DOS_STYLE);
  }
  inline void SetFileName(const FileName *filename) {
    filename_ = filename;
    name_length_ch_ = filename->filename_length;
  }
  inline void GetFileName(word *filename, b32 size) const {
    if (filename_) {
      memcpy(
          filename, (b8 *)filename_ + sizeof(FileName),
          ((name_length_ch_ > size) ? size : name_length_ch_) * sizeof(word));
    }
  }
  inline b64 GetFileSize() const { return filename_ ? filename_->byte_use : 0; }

  inline bool IsReadOnly() const {
    return filename_ ? (filename_->flags & READONLY) : false;
  }
  inline bool IsHidden() const {
    return filename_ ? (filename_->flags & HIDDEN) : false;
  }
  inline bool IsSystem() const {
    return filename_ ? (filename_->flags & SYSTEM) : false;
  }
  inline bool IsArchive() const {
    return filename_ ? (filename_->flags & ARCHIVE) : false;
  }
  inline bool IsTemporary() const {
    return filename_ ? (filename_->flags & TEMPORARY) : false;
  }
  inline bool IsSparse() const {
    return filename_ ? (filename_->flags & SPARSEFILE) : false;
  }
  inline bool IsReparse() const {
    return filename_ ? (filename_->flags & REPARES_Point) : false;
  }
  inline bool IsCompressed() const {
    return filename_ ? (filename_->flags & COMPRESSED) : false;
  }
  inline bool IsEncrypted() const {
    return filename_ ? (filename_->flags & ENCRYPTED) : false;
  }
  inline bool IsDirectory() const {
    return filename_ ? (filename_->flags & DIRECTORY) : false;
  }
  void GetFileTime(FILETIME *tm_write, FILETIME *tm_create, FILETIME *tm_access,
                   FILETIME *tm_change) const;

 private:
  void UTC2Local(const b64 &ulong_time, FILETIME *tm) const;

 private:
  const FileName *filename_;
  b32 name_length_ch_;
};

class CFileNameAttr : public CResidentAttr, public CFileName {
 public:
  CFileNameAttr(const AttributeHeader *attr_header,
                const CFileRecord *file_record)
      : CResidentAttr(attr_header, file_record) {
    filename_ = (FileName *)attr_body_;
    SetFileName(filename_);
  }

 private:
  void GetFileTime(FILETIME *tm_write, FILETIME *tm_create, FILETIME *tm_access,
                   FILETIME *tm_change) const {}
  inline bool IsReadOnly() const {}
  inline bool IsHidden() const {}
  inline bool IsSystem() const {}
  inline bool IsArchive() const {}
  inline bool IsTemporary() const {}
  inline bool IsSparse() const {}
  inline bool IsReparse() const {}
  inline bool IsCompressed() const {}
  inline bool IsEncrypted() const {}
  inline bool IsDirectory() const {}

 private:
  const FileName *filename_;
};

class CVolumeNameAttr : public CResidentAttr {
 public:
  CVolumeNameAttr(const AttributeHeader *attr_header,
                  const CFileRecord *file_record)
      : CResidentAttr(attr_header, file_record) {
    volume_name_ = (VolumeName *)attr_body_;
    name_length_ch_ = attr_size_ >> 1;
  }

  inline void GetVolumeName(word *volumename, b32 size) const {
    if (volume_name_) {
      memcpy(
          volumename, (byte *)volume_name_,
          ((name_length_ch_ > size) ? size : name_length_ch_) * sizeof(word));
    }
  }

 private:
  const VolumeName *volume_name_;
  b32 name_length_ch_;
};

class CVolumeInfoAttr : public CResidentAttr {
 public:
  CVolumeInfoAttr(const AttributeHeader *attr_header,
                  const CFileRecord *file_record)
      : CResidentAttr(attr_header, file_record) {
    volume_info_ = (VolumeInformation *)attr_body_;
  }

  inline b16 GetVersion() const {
    return MAKEWORD(volume_info_->minor_ver, volume_info_->major_ver);
  }

 private:
  const VolumeInformation *volume_info_;
};

template <class ATTR_BASE>
class CDataAttr : public ATTR_BASE {
 public:
  CDataAttr(const AttributeHeader *attr_header, const CFileRecord *file_record)
      : ATTR_BASE(attr_header, file_record) {
    ;
  }
};  // CAttr_AttrList

class CIndexEntry : public CFileName {
 public:
  CIndexEntry(const IndexEntry *entry) : index_entry_(entry) {
    if (entry->stream_size) {
      SetFileName((FileName *)(entry->stream));
    }
  }

  inline b64 GetMFTReference() const {
    if (index_entry_)
      return index_entry_->mft_ref & 0x0000FFFFFFFFFFFFUL;
    else
      return (b64)-1;
  }

  inline bool IsSubNodePtr() const {
    if (index_entry_)
      return (index_entry_->flags & ENTRY_SUBNODE);
    else
      return false;
  }

  inline b64 GetSubNodeVCN() const {
    if (index_entry_)
      return *(b64 *)((b8 *)index_entry_ + index_entry_->size - 8);
    else
      return (b64)-1;
  }

 private:
  const IndexEntry *index_entry_;
};

typedef std::list<CIndexEntry> CIndexList;
class CIndexRootAttr : public CResidentAttr, public CIndexList {
 public:
  CIndexRootAttr(const AttributeHeader *attr_header,
                 const CFileRecord *file_record)
      : CResidentAttr(attr_header, file_record) {
    index_root_ = (IndexRoot *)attr_body_;
  }

  bool LoadIndexEntry();

 private:
  const IndexRoot *index_root_;
};

class CIndexBlock;
class CIndexAllocAttr : public CNonResidentAttr {
 public:
  CIndexAllocAttr(const AttributeHeader *attr_header,
                  const CFileRecord *file_record)
      : CNonResidentAttr(attr_header, file_record) {
    cluster_size_ = file_record_->cluster_size();
    idx_block_size_ = file_record_->idx_size();
    sector_size_ = file_record_->sector_size();
  }

  bool LoadIndexEntry(b64 vcn, CIndexBlock *ib);

 private:
  bool PatchUS(b16 *sector_pointer, b32 sector_num, b16 usn,
               b16 *us_data) const;

 private:
  b32 cluster_size_;
  b32 idx_block_size_;
  b16 sector_size_;
};

class CIndexBlock : public CIndexList {
  friend class CIndexAllocAttr;

 private:
  scope::ScopedPtr<IndexBlock> index_block_;
};

};  // namespace ntfs
};  // namespace filescanner
#endif
#endif
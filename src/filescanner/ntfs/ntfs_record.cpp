#include "ntfs_record.h"

#include <assert.h>
#include <stdio.h>

#include "ntfs_attribute.h"

namespace filescanner {
namespace ntfs {

bool CNtfsVolume::OpenVolume(const os_string &path) {
  bool failed = false;
  do {
    volume_ = CreateFile(path.c_str(), GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                         OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (volume_ != INVALID_HANDLE_VALUE) {
      BootSector boot_sector;
      ulong num;

      // Read boot sector
      if (ReadFile(volume_, &boot_sector, sizeof(boot_sector), &num, NULL) &&
          num == sizeof(boot_sector)) {
        if (strncmp((const char *)boot_sector.oem_id, kNtfsSignature, 8) == 0) {
          sector_size_ = boot_sector.bpb.bytes;
          cluster_size_ = sector_size_ * boot_sector.bpb.sectors;

          b8 num_cluster = boot_sector.extend_bpb.clusters_per_mft;
          if (num_cluster > 0)
            mft_size_ = cluster_size_ * num_cluster;
          else
            mft_size_ = 1 << (-num_cluster);

          num_cluster = boot_sector.extend_bpb.clusters_per_idx;
          if (num_cluster > 0)
            idx_size_ = cluster_size_ * num_cluster;
          else
            idx_size_ = 1 << (-num_cluster);

          mft_address_ = boot_sector.extend_bpb.lcn_mft * cluster_size_;
        } else {
          failed = true;
        }
      } else {
        failed = true;
      }
    }
  } while (false);

  if (failed) {
    return false;
  }

  // Verify NTFS volume version (must >= 3.0)
  CFileRecord record(this);
  record.set_attr_mask(MASK_VOLUME_NAME | MASK_VOLUME_INFORMATION);
  if (!record.ReadFileRecord(MFT_IDX_VOLUME)) return false;

  record.ReadAttrs();

  CAttrListIter it = record.AttrBegin(VOLUME_INFORMATION);
  if (it == record.AttrEnd(VOLUME_INFORMATION)) {
    return false;
  }

  CVolumeInfoAttr *vi = (CVolumeInfoAttr *)*it;
  if (vi->GetVersion() < 0x0300) {
    return false;
  }

  mft_record_ = new CFileRecord(this);
  mft_record_->set_attr_mask(MASK_DATA);
  if (mft_record_->ReadFileRecord(MFT_IDX_MFT)) {
    mft_record_->ReadAttrs();
    mft_data_ = (CBaseAttr *)(*mft_record_->AttrBegin(DATA));
    if (mft_data_ == NULL) {
      delete mft_record_;
      mft_record_ = NULL;
    }
  }

  return true;
}

bool CFileRecord::ReadFileRecord(b64 file_record_idx) {
  ClearAttrs();
  if (file_record_) {
    delete file_record_;
    file_record_ = NULL;
  }

  FileRecordHeader *frh = LoadFileRecord(file_record_idx);
  if (frh == NULL) {
    file_record_idx_ = (b64)-1;
  } else {
    file_record_idx_ = file_record_idx;

    if (frh->magic == kFileRecordMagic) {
      // Patch US
      b16 *usn_addr = (b16 *)((b8 *)frh + frh->usn_offset);
      b16 usn = *usn_addr;
      b16 *usarray = usn_addr + 1;
      if (PatchUS((b16 *)frh,
                  ntfs_volume_->mft_size_ / ntfs_volume_->sector_size_, usn,
                  usarray)) {
        file_record_ = frh;
        return true;
      }
    }
    delete frh;
  }
  return false;
}

bool CFileRecord::PatchUS(b16 *sector_pointer, b32 sector_num, b16 usn,
                          b16 *us_data) const {
  for (b32 i = 0; i < sector_num; i++) {
    // offset to the end word of sector
    sector_pointer += ((ntfs_volume_->sector_size_ >> 1) - 1);
    if (*sector_pointer != usn) return false;  // USN check
    *sector_pointer = us_data[i];              // set data to update us
    sector_pointer++;
  }
  return true;
}

FileRecordHeader *CFileRecord::LoadFileRecord(b64 &file_record_idx) {
  FileRecordHeader *frh = NULL;
  b32 len;
  if (file_record_idx <= MFT_IDX_END || ntfs_volume_->mft_data_ == NULL) {
    // Take as continuous disk allocation
    LARGE_INTEGER fr_addr;
    fr_addr.QuadPart = ntfs_volume_->mft_address_ +
                       (ntfs_volume_->mft_size_) * file_record_idx;
    fr_addr.LowPart = SetFilePointer(ntfs_volume_->volume_, fr_addr.LowPart,
                                     &fr_addr.HighPart, FILE_BEGIN);

    if (fr_addr.LowPart == ulong(-1) && GetLastError() != NO_ERROR)
      return FALSE;
    else {
      frh = (FileRecordHeader *)new b8[ntfs_volume_->mft_size_];
      if (ReadFile(ntfs_volume_->volume_, frh, ntfs_volume_->mft_size_,
                   (LPDWORD)&len, NULL) &&
          len == ntfs_volume_->mft_size_)
        return frh;
      else {
        delete frh;
        return NULL;
      }
    }
  } else {
    // May be fragmented $MFT
    b64 fr_addr = (ntfs_volume_->mft_size_) * file_record_idx;
    ntfs_volume_->mft_data_->Seek(fr_addr, SK_SET);

    frh = (FileRecordHeader *)new b8[ntfs_volume_->mft_size_];
    if (ntfs_volume_->mft_data_->Read(frh, ntfs_volume_->mft_size_, &len) &&
        len == ntfs_volume_->mft_size_)
      return frh;
    else {
      delete frh;
      return NULL;
    }
  }
}

bool CFileRecord::ReadAttrs() {
  // Clear previous data
  ClearAttrs();

  // Visit all attributes
  b32 attr_addr = 0;
  AttributeHeader *attr_header =
      (AttributeHeader *)((b8 *)file_record_ + file_record_->attr_offset);
  attr_addr += file_record_->attr_offset;

  while (attr_header->attr_type != (b32)0xFFFFFFFF &&
         (attr_addr + attr_header->length) <= ntfs_volume_->mft_size_) {
    if (AttrMask(attr_header->attr_type) &
        attr_mask_)  // Skip unwanted attributes
    {
      b32 attrIndex = AttrIndex(attr_header->attr_type);
      if (attrIndex < kMaxAttributeType) {
        CBaseAttr *attr = Attr(attr_header);
        if (attr) {
          attr_list_[attrIndex].push_back(attr);
        } else {
          return false;
        }
      } else {
        return false;
      }
    }

    attr_addr += attr_header->length;
    attr_header = (AttributeHeader *)((b8 *)attr_header +
                                      attr_header->length);  // next attribute
  }

  return true;
}

CBaseAttr *CFileRecord::Attr(AttributeHeader *attr_header) {
  switch (attr_header->attr_type) {
    case STANDARD_INFORMATION:
      return new CStandardInfoAttr(attr_header, this);

    case ATTRIBUTE_LIST:
      if (attr_header->non_resident)
        return new CAttributeListAttr<CNonResidentAttr>(attr_header, this);
      else
        return new CAttributeListAttr<CResidentAttr>(attr_header, this);

    case FILE_NAME:
      return new CFileNameAttr(attr_header, this);

    case VOLUME_NAME:
      return new CVolumeNameAttr(attr_header, this);

    case VOLUME_INFORMATION:
      return new CVolumeInfoAttr(attr_header, this);

    case DATA:
      if (attr_header->non_resident)
        return new CDataAttr<CNonResidentAttr>(attr_header, this);
      else
        return new CDataAttr<CResidentAttr>(attr_header, this);

    case INDEX_ROOT:
      return new CIndexRootAttr(attr_header, this);

    case INDEX_ALLOCATION:
      return new CIndexAllocAttr(attr_header, this);

    default:
      if (attr_header->non_resident)
        return new CNonResidentAttr(attr_header, this);
      else
        return new CResidentAttr(attr_header, this);
  }
}

CBaseAttr *CFileRecord::FindData() const {
  CBaseAttr *pData = NULL;
  CAttrListIter it = AttrBegin(DATA);
  // find a stream with data
  while (it != AttrEnd(DATA)) {
    if (!(pData = *it)->Named()) {
      break;
    }
    it++;
  }

  return pData;
}

}  // namespace ntfs
}  // namespace filescanner
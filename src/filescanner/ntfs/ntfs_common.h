#ifdef _WIN32
#ifndef NTFS_COMMON_H_
#define NTFS_COMMON_H_
#include "common/type.h"
#include "common/string.h"

using namespace common;
namespace filescanner {
namespace ntfs {

const char kNtfsSignature[] = "NTFS    ";
const b32 kFileRecordMagic = 'ELIF';
const b32 kIndexBlockMagic = 'XDNI';

// MFT Index
typedef enum {
  MFT_IDX_MFT = 0,
  MFT_IDX_VOLUME = 3,
  MFT_IDX_ROOT = 5,
  MFT_IDX_END = 16,
} MftIdxs;

// Attribute Header
typedef enum {
  STANDARD_INFORMATION = 0x10,
  ATTRIBUTE_LIST = 0x20,
  FILE_NAME = 0x30,
  VOLUME_NAME = 0x60,
  VOLUME_INFORMATION = 0x70,
  DATA = 0x80,
  INDEX_ROOT = 0x90,
  INDEX_ALLOCATION = 0xA0,
  SYMBOLIC_LINK = 0xC0,  // win nt
  REPARSE_POINT = 0xC0,  // win 2k
} AttributeTypes;

// count of attr type
#define kMaxAttributeType (16)

// Bit masks of Attributes
inline constexpr b32 AttrIndex(b32 at) { return (((at) >> 4) - 1); }

inline constexpr b32 AttrMask(b32 at) { return (((b32)1) << AttrIndex(at)); }

typedef enum {
  MASK_STANDARD_INFORMATION = AttrMask(STANDARD_INFORMATION),
  MASK_ATTRIBUTE_LIST = AttrMask(ATTRIBUTE_LIST),
  MASK_FILE_NAME = AttrMask(FILE_NAME),
  MASK_VOLUME_NAME = AttrMask(VOLUME_NAME),                // 0x20
  MASK_VOLUME_INFORMATION = AttrMask(VOLUME_INFORMATION),  // 0x40
  MASK_DATA = AttrMask(DATA),                              // 0x80
  MASK_INDEX_ROOT = AttrMask(INDEX_ROOT),                  // 0x100
  MASK_INDEX_ALLOCATION = AttrMask(INDEX_ALLOCATION),      // 0x200
  MASK_REPARSE_POINT = AttrMask(REPARSE_POINT),            // 0x800
} AttributeMasks;

}  // namespace ntfs
}  // namespace filescanner
#endif
#endif

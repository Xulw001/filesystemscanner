#ifdef __linux__
#ifndef _EXT_COMMON_H
#define _EXT_COMMON_H
#include "common/type.h"

using namespace common;
namespace filescanner {
namespace ext {

// https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout
const ub16 kExtSuperMagic = 0xEF53;
const ub16 kExtExtentMagic = 0xF30A;
const ub32 kExtAttrMagic = 0xEA020000;

const b32 kExtMinBlockSize = 0x400;
const b32 kExtBootSize = 0x400;

typedef enum {
  INODE_ROOT = 2,          // Root directory.
  INODE_NO_RESERVED = 11,  // Traditional first non-reserved inode.
                           // Usually this is the lost+found directory.
                           // See s_first_ino in the superblock.
} InodeNo;

typedef enum {
  EXT4_INDEX_FL = 0x1000,            // Directory has hashed indexes
  EXT4_EXTENTS_FL = 0x80000,         // Inode uses extents
  EXT4_INLINE_DATA_FL = 0x10000000,  // Inode has inline data
} InodeFlg;

// Name Index	Key Prefix
// 0	(no prefix)
// 1	"user."
// 2	"system.posix_acl_access"
// 3	"system.posix_acl_default"
// 4	"trusted."
// 6	"security."
// 7	"system." (inline_data only?)
// 8	"system.richacl" (SuSE kernels only?)
// eg.
//    if the attribute key is "user.fubar",
//    the attribute name index is set to 1 and the "fubar" name is recorded on
//    disk.
const b32 kExtAttrDataIdx = 0x07;
const b32 kExtAttrDataName = 0x61746164;  // "data"

typedef enum {
  COMPAT_DIR_PREALLOC = 0x0001,
  COMPAT_IMAGIC_INODES = 0x0002,
  COMPAT_HAS_JOURNAL = 0x0004,
  COMPAT_EXT_ATTR = 0x0008,
  COMPAT_RESIZE_INODE = 0x0010,
  COMPAT_DIR_INDEX = 0x0020,
  COMPAT_LAZY_BG = 0x0040,
  COMPAT_EXCLUDE_INODE = 0x0080,
  COMPAT_EXCLUDE_BITMAP = 0x0100,
  // Sparse Super Block, v2.
  COMPAT_SPARSE_SUPER2 = 0x0200,
} CompactFeature;

typedef enum {
  // Sparse superblocks
  RO_COMPAT_SPARSE_SUPER = 0x0001,
  RO_COMPAT_LARGE_FILE = 0x0002,
  RO_COMPAT_HUGE_FILE = 0x0008,
  RO_COMPAT_GDT_CSUM = 0x0010,
  RO_COMPAT_DIR_NLINK = 0x0020,
  RO_COMPAT_EXTRA_ISIZE = 0x0040,
  RO_COMPAT_HAS_SNAPSHOT = 0x0080,
  RO_COMPAT_QUOTA = 0x0100,
  // bitmaps are tracked in units of clusters instead of blocks
  RO_COMPAT_BIGALLOC = 0x0200,
  RO_COMPAT_METADATA_CSUM = 0x0400,
  RO_COMPAT_REPLICA = 0x0800,
} RoCompactFeature;

typedef enum {
  INCOMPAT_COMPRESSION = 0x0001,
  INCOMPAT_FILETYPE = 0x0002,
  INCOMPAT_RECOVER = 0x0004,
  INCOMPAT_JOURNAL_DEV = 0x0008,
  INCOMPAT_META_BG = 0x0010,  // Meta block groups
  INCOMPAT_EXTENTS = 0x0040,
  INCOMPAT_64BIT = 0x0080,  // Enable a filesystem size over 2^32 blocks
  INCOMPAT_MMP = 0x0100,
  INCOMPAT_FLEX_BG = 0x0200,
  INCOMPAT_EA_INODE = 0x0400,
  INCOMPAT_DIRDATA = 0x1000,
  INCOMPAT_LARGEDIR = 0x4000,
  INCOMPAT_INLINEDATA = 0x8000,
} InCompactFeature;

typedef enum {
  TYPE_IFIFO = 0x1000,   // 1 << 0 = 0x01
  TYPE_IFCHR = 0x2000,   // 1 << 1 = 0x02
  TYPE_IFDIR = 0x4000,   // 1 << 3 = 0x08
  TYPE_IFBLK = 0x6000,   // 1 << 5 = 0x20
  TYPE_IFREG = 0x8000,   // 1 << 7 = 0x80
  TYPE_IFLNK = 0xA000,   // 1 << 9 = 0x200
  TYPE_IFSOCK = 0xC000,  // 1 << B = 0x800
} FileType;

typedef enum {
  Unknown,
  Regular,
  Directory,
  CharacterDevice,
  BlockDevice,
  FIFO,
  Socket,
  Symbolic,
  Other = 0xff,
} IdxFileType;

inline constexpr b32 TypeIndex(b32 at) { return (((at) >> 12) - 1); }
inline constexpr b32 TypeMask(b32 at) { return (((b32)1) << TypeIndex(at)); }

typedef enum {
  MASK_IFDIR = TypeMask(TYPE_IFDIR),
  MASK_IFREG = TypeMask(TYPE_IFREG),
  MASK_IFLNK = TypeMask(TYPE_IFLNK),
} FileTypeMask;

}  // namespace ext
}  // namespace filescanner
#endif
#endif
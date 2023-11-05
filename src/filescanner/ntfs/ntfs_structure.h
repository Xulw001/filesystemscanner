#ifndef NTFS_STRUCTRE_H_
#define NTFS_STRUCTRE_H_
#include "ntfs_common.h"
namespace filescanner {
namespace ntfs {
#pragma pack(1)

typedef struct {
  b16 bytes;            // The size of a hardware sector.
  b8 sectors;           // The number of sectors in a cluster.
  b8 resv1[7];          // unused
  b8 media_descriptor;  // Provides information about the media being used.
  b8 resv2[2];          // Value must be 0
  b16 resv3;            // The number of sectors in a track.
  b16 resv4;            // The number of heads
  b8 resv5[8];
} Bpb;

// LCN = Logical Cluster Number
typedef struct {
  b8 resv1[4];          // Usually 80 00 80 00
  b64 sectors;          // The total number of sectors on the hard disk.
  b64 lcn_mft;          // LCN of the $MFT
  b64 lcn_mft_mirr;     // LCN of the $MFTMirr
  b8 clusters_per_mft;  // The number of cluster of each record
  b8 resv2[3];
  b8 clusters_per_idx;  // The number of cluster of each index
  b8 resv3[3];
  b8 volume_serial_number[8];
  b8 resv4[4];
} ExtendBpb;

typedef struct {
  // jump instruction
  b8 jmp_code[3];

  // OEM ID
  b8 oem_id[8];

  // Bpb and extended Bpb
  Bpb bpb;
  ExtendBpb extend_bpb;

  // boot code
  b8 boot_code[426];

  // 0x55AA
  b8 mark_AA;
  b8 mark_55;
} BootSector;

// File Record Layout
// Record Header
// Attribute
// Attribute
// ...
// End Marker (0xFFFFFFFF)
typedef struct {
  b32 magic;          // "FILE"
  ub16 usn_offset;    // offset of update sequence number
  ub16 usn_size;      // size of update sequence number and array, by words
  b64 lsn;            // $LogFile sequence number
  ub16 sn;            // sequence number(used times of this record)
  ub16 hard_links;    //
  ub16 attr_offset;   // offset of the first attribute record
  ub16 flags;         //
  b32 used;           // real size of the record
  b32 alloced;        // alloc size of the record
  b64 record_ref;     // file reference to the base record
  ub16 next_attr;     // next attribute id
  ub16 border;        // boundary in xp
  b32 record_number;  // number of this mft record
} FileRecordHeader;

// 0xF
typedef struct {
  b32 attr_type;  // Attribute Type
  b32 length;
  b8 non_resident;  // 0 - resident, 1 - non resident
  b8 name_length;   // 0 - no name
  ub16 name_offset;
  ub16 flags;    // Flags
  ub16 attr_id;  // Attribute Id
} AttributeHeader;

// 0x8
typedef struct {
  AttributeHeader header;  // Common data structure
  b32 length;              // Length of the attribute body
  ub16 offset;             // offset to the Attribute
  b8 index_flag;           // Indexed flag
  b8 padding;              // Padding
} ResidentAttrHeader;

typedef struct {
  AttributeHeader header;  // Common data structure
  b64 vcn_begin;           // Starting VCN
  b64 vcn_end;             // Last VCN
  ub16 data_run_offset;    // offset to the Data Runs
  ub16 compression_size;   // Compression unit size
  b32 padding;             // Padding
  b64 byte_alloc;          // Allocated size of the attribute
  b64 byte_use;            // Real size of the attribute
  b64 init_size;           // Initialized data size of the stream
} NonResidentAttrHeader;

// STANDARD_INFORMATION = 0x10,
typedef struct {
  b64 time_create;      // File creation time
  b64 time_update;      // File altered time
  b64 time_mft_change;  // MFT changed time
  b64 time_access;      // File read time
  b32 file_attribute;   // Dos file permission
  b32 max_version;      // Maxim number of file versions
  b32 version;          // File version number
  b32 class_id;         // Class Id
  b32 owner_id;         // Owner Id
  b32 security_id;      // Security Id
  b64 quota_charged;    // Quota charged
  b64 usn;              // USN Journel
} StdInformation;

// ATTRIBUTE_LIST = 0x20,
typedef struct {
  b32 attr_type;  // Attribute Type
  ub16 record_length;
  b8 name_length;
  b8 name_offset;
  b64 start_vcn;
  b64 base_record_ref;  // file reference to the base record
  ub16 attr_id;
} AttrList;

typedef enum {
  READONLY = 0x0001,
  HIDDEN = 0x0002,
  SYSTEM = 0x0004,
  ARCHIVE = 0x0020,
  DEVICE = 0x0040,
  NORMAL = 0x0080,
  TEMPORARY = 0x0100,
  SPARSEFILE = 0x0200,
  REPARES_Point = 0x0400,
  COMPRESSED = 0x0800,
  OFFLINE = 0x1000,
  NOT_CONTENT_INDEXED = 0x2000,
  ENCRYPTED = 0x4000,
  DIRECTORY = 0x10000000,   //(copy from corresponding bit in MFT record)
  INDEX_VIEW = 0x20000000,  //(copy from corresponding bit in MFT record)
} FileAttrFlags;

typedef enum {
  POSIX_STYLE = 0,
  WIN32_STYLE,
  DOS_STYLE,
  WIN_DOS_STYLE,
} FileNameSpace;

// FILE_NAME = 0x30,
typedef struct {
  // low 6byte is parent mft record, and end 2 b8 is sn
  b64 parent_ref;
  b64 time_create;      // File creation time
  b64 time_update;      // File altered time
  b64 time_mft_change;  // MFT changed time
  b64 time_access;
  b64 byte_alloc;
  b64 byte_use;
  b32 flags;
  b32 ea_flags;
  ub8 filename_length;  // Filename length in characters (0-255)
  b8 name_space;
} FileName;

// VOLUME_NAME = 0x60,
typedef struct {
  ub16 volumename[0];  // volume name
} VolumeName;

// VOLUME_INFORMATION = 0x70,
typedef struct {
  b8 resv1[8];   // 00
  b8 major_ver;  // major version 1--winNT, 3--Win2000/XP
  b8 minor_ver;  // minor version 0--win2000, 1--WinXP/7
  ub16 flag;     // mark
  b8 resv2[4];   // 00
} VolumeInformation;

// DATA = 0x80,
// AutoRun

// INDEX_ROOT = 0x90,
typedef struct {
  // Index Root Header
  b32 attr_type;
  b32 collation_rule;
  b32 index_size;         // Size of index block
  b8 clusters_per_index;  // Clusters per index block (same as Bpb?)
  b8 padding[3];          // Padding
  // Index Header
  b32 entry_offset;  // Offset to the first index entry
  b32 total_size;    // Total size of the index entries
  b32 alloc_size;    // Allocated size of the index entries
  b8 flags;          // Non-leaf node Flag
  b8 padding2[3];    // Padding
} IndexRoot;

// 1	Index entry points to a sub-node
// 2	Last index entry in the node

typedef enum {
  ENTRY_SUBNODE = 1,  // Index entry points to a sub-node
  ENTRY_LAST = 2,     // Last index entry in the node
} IdxEntryTypes;

typedef struct {
  // low 6byte is MFT record index, and end 2 b8 is sn
  b64 mft_ref;
  ub16 size;         // Length of the index entry
  ub16 stream_size;  // Length of the stream
  b8 flags;          // Flags
  b8 padding[3];     // Padding
  // copy of body(without header) of attribute
  /* Name | Index Of              | Used By
   * --------------------------------------
   * $I30 | Filenames	            | Directories
   * $SDH | Security Descriptors  | $Secure
   * $SII | Security Ids	        | $Secure
   * $O   | Object Ids	          | $ObjId
   * $O   | Owner Ids	            | $Quota
   * $Q   | Quotas	              | $Quota
   * $R   | Reparse Points	      | $Reparse
   */
  b8 stream[1];  // align to 8
  // VCN of the sub-node in the Index Allocation
} IndexEntry;

// INDEX_ALLOCATION = 0xA0,
// SYMBOLIC_LINK = 0xC0, // win nt
// REPARSE_POINT = 0xC0, // win 2k
typedef struct {
  // Index Record Header
  b32 magic;        // "INDX"
  ub16 usn_offset;  // offset of update sequence number
  ub16 usn_size;    // size of update sequence number and array, by words
  b64 lsn;          // $LogFile sequence number
  b64 vcn;          // vcn of this index block in the index allocation
  // Index Header
  b32 entry_offset;  // Offset to the first index entry
  b32 total_size;    // Total size of the index entries
  b32 alloc_size;    // Allocated size of the index entries
  b8 flags;          // Non-leaf node Flag
  b8 padding[3];     // Padding
} IndexBlock;

// DataRun

// 21 18 34 56 00
// first b8:
//  - low bit is cluster of data run
//  - high bit is start lcn of data run when data run is frist, otherwise is
//    offset to first datarun
// end b8:
//  - when b8 >= 0x80, represent the offset is negative

#pragma pack()
}  // namespace ntfs
}  // namespace filescanner
#endif
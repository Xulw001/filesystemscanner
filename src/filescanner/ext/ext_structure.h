#ifdef __linux__
#ifndef EXT_STRUCTURE_H
#define EXT_STRUCTURE_H
#include "common/type.h"
#include "ext_common.h"

using namespace common;
namespace filescanner {
namespace ext {
#pragma pack(1)
typedef struct {
  ub32 s_inodes_count;       /* Inodes count */
  ub32 s_blocks_count;       /* Blocks count */
  ub32 s_r_blocks_count;     /* Reserved blocks count */
  ub32 s_free_blocks_count;  /* Free blocks count */
  ub32 s_free_inodes_count;  /* Free inodes count */
  ub32 s_first_data_block;   /* First Data Block */
  ub32 s_log_block_size;     /* Block size */
  ub32 s_log_cluster_size;   /* Allocation cluster size */
  ub32 s_blocks_per_group;   /* # Blocks per group */
  ub32 s_clusters_per_group; /* # Fragments per group */
  ub32 s_inodes_per_group;   /* # Inodes per group */
  ub32 s_mtime;              /* Mount time */
  ub32 s_wtime;              /* Write time */
  ub16 s_mnt_count;          /* Mount count */
  ub16 s_max_mnt_count;      /* Maximal mount count */
  ub16 s_magic;              /* Magic signature */
  ub16 s_state;              /* File system state */
  ub16 s_errors;             /* Behaviour when detecting errors */
  ub16 s_minor_rev_level;    /* minor revision level */
  ub32 s_lastcheck;          /* time of last check */
  ub32 s_checkinterval;      /* max. time between checks */
  ub32 s_creator_os;         /* OS */
  ub32 s_rev_level;          /* Revision level */
  ub16 s_def_resuid;         /* Default uid for reserved blocks */
  ub16 s_def_resgid;         /* Default gid for reserved blocks */
  /*
   * These fields are for EXT2_DYNAMIC_REV superblocks only.
   *
   * Note: the difference between the compatible feature set and
   * the incompatible feature set is that if there is a bit set
   * in the incompatible feature set that the kernel doesn't
   * know about, it should refuse to mount the filesystem.
   *
   * e2fsck's requirements are more strict; if it doesn't know
   * about a feature in either the compatible or incompatible
   * feature set, it must abort and not try to meddle with
   * things it doesn't understand...
   */
  ub32 s_first_ino;              /* First non-reserved inode */
  ub16 s_inode_size;             /* size of inode structure */
  ub16 s_block_group_nr;         /* block group # of this superblock */
  ub32 s_feature_compat;         /* compatible feature set */
  ub32 s_feature_incompat;       /* incompatible feature set */
  ub32 s_feature_ro_compat;      /* readonly-compatible feature set */
  ub8 s_uuid[16];                /* 128-bit uuid for volume */
  b8 s_volume_name[16];          /* volume name */
  b8 s_last_mounted[64];         /* directory where last mounted */
  ub32 s_algorithm_usage_bitmap; /* For compression */
  /*
   * Performance hints.  Directory preallocation should only
   * happen if the EXT2_FEATURE_COMPAT_DIR_PREALLOC flag is on.
   */
  ub8 s_prealloc_blocks;      /* Nr of blocks to try to preallocate*/
  ub8 s_prealloc_dir_blocks;  /* Nr to preallocate for dirs */
  ub16 s_reserved_gdt_blocks; /* Per group table for online growth */
  /*
   * Journaling support valid if EXT2_FEATURE_COMPAT_HAS_JOURNAL set.
   */
  ub8 s_journal_uuid[16]; /* uuid of journal superblock */
  ub32 s_journal_inum;    /* inode number of journal file */
  ub32 s_journal_dev;     /* device number of journal file */
  ub32 s_last_orphan;     /* start of list of inodes to delete */
  ub32 s_hash_seed[4];    /* HTREE hash seed */
  ub8 s_def_hash_version; /* Default hash version to use */
  ub8 s_jnl_backup_type;  /* Default type of journal backup */
  ub16 s_desc_size;       /* Group desc. size: INCOMPAT_64BIT */
  ub32 s_default_mount_opts;
  ub32 s_first_meta_bg;       /* First metablock group */
  ub32 s_mkfs_time;           /* When the filesystem was created */
  ub32 s_jnl_blocks[17];      /* Backup of the journal inode */
  ub32 s_blocks_count_hi;     /* Blocks count high 32bits */
  ub32 s_r_blocks_count_hi;   /* Reserved blocks count high 32 bits*/
  ub32 s_free_blocks_hi;      /* Free blocks count */
  ub16 s_min_extra_isize;     /* All inodes have at least # bytes */
  ub16 s_want_extra_isize;    /* New inodes should reserve # bytes */
  ub32 s_flags;               /* Miscellaneous flags */
  ub16 s_raid_stride;         /* RAID stride */
  ub16 s_mmp_update_interval; /* # seconds to wait in MMP checking */
  ub64 s_mmp_block;           /* Block for multi-mount protection */
  ub32 s_raid_stripe_width;   /* blocks on all data disks (N*stride)*/
  ub8 s_log_groups_per_flex;  /* FLEX_BG group size */
  ub8 s_reserved_char_pad;
  ub16 s_reserved_pad;            /* Padding to next 32bits */
  ub64 s_kbytes_written;          /* nr of lifetime kilobytes written */
  ub32 s_snapshot_inum;           /* Inode number of active snapshot */
  ub32 s_snapshot_id;             /* sequential ID of active snapshot */
  ub64 s_snapshot_r_blocks_count; /* reserved blocks for active
                      snapshot's future use */
  ub32 s_snapshot_list;     /* inode number of the head of the on-disk snapshot
                                   list */
  ub32 s_error_count;       /* number of fs errors */
  ub32 s_first_error_time;  /* first time an error happened */
  ub32 s_first_error_ino;   /* inode involved in first error */
  ub64 s_first_error_block; /* block involved of first error */
  ub8 s_first_error_func[32]; /* function where the error happened */
  ub32 s_first_error_line;    /* line number where error happened */
  ub32 s_last_error_time;     /* most recent time of an error */
  ub32 s_last_error_ino;      /* inode involved in last error */
  ub32 s_last_error_line;     /* line number where error happened */
  ub64 s_last_error_block;    /* block involved of last error */
  ub8 s_last_error_func[32];  /* function where the error happened */
  ub8 s_mount_opts[64];
  ub32 s_usr_quota_inum;  /* inode number of user quota file */
  ub32 s_grp_quota_inum;  /* inode number of group quota file */
  ub32 s_overhead_blocks; /* overhead blocks/clusters in fs */
  ub32 s_backup_bgs[2];   /* If sparse_super2 enabled */
  ub32 s_reserved[106];   /* Padding to the end of the block */
  ub32 s_checksum;        /* crc32c(superblock) */
} app_ext4_super_block;

typedef struct {
  ub32 bg_block_bitmap;         /* Blocks bitmap block */
  ub32 bg_inode_bitmap;         /* Inodes bitmap block */
  ub32 bg_inode_table;          /* Inodes table block */
  ub16 bg_free_blocks_count;    /* Free blocks count */
  ub16 bg_free_inodes_count;    /* Free inodes count */
  ub16 bg_used_dirs_count;      /* Directories count */
  ub16 bg_flags;                /* EXT4_BG_flags (INODE_UNINIT, etc) */
  ub32 bg_exclude_bitmap_lo;    /* Exclude bitmap for snapshots */
  ub16 bg_block_bitmap_csum_lo; /* crc32c(s_uuid+grp_num+bitmap) LSB */
  ub16 bg_inode_bitmap_csum_lo; /* crc32c(s_uuid+grp_num+bitmap) LSB */
  ub16 bg_itable_unused;        /* Unused inodes count */
  ub16 bg_checksum;             /* crc16(sb_uuid+group+desc) */
} app_ext4_group_desc;

typedef struct {
  ub32 bg_block_bitmap;         /* Blocks bitmap block */
  ub32 bg_inode_bitmap;         /* Inodes bitmap block */
  ub32 bg_inode_table;          /* Inodes table block */
  ub16 bg_free_blocks_count;    /* Free blocks count */
  ub16 bg_free_inodes_count;    /* Free inodes count */
  ub16 bg_used_dirs_count;      /* Directories count */
  ub16 bg_flags;                /* EXT4_BG_flags (INODE_UNINIT, etc) */
  ub32 bg_exclude_bitmap_lo;    /* Exclude bitmap for snapshots */
  ub16 bg_block_bitmap_csum_lo; /* crc32c(s_uuid+grp_num+bitmap) LSB */
  ub16 bg_inode_bitmap_csum_lo; /* crc32c(s_uuid+grp_num+bitmap) LSB */
  ub16 bg_itable_unused;        /* Unused inodes count */
  ub16 bg_checksum;             /* crc16(sb_uuid+group+desc) */
  ub32 bg_block_bitmap_hi;      /* Blocks bitmap block MSB */
  ub32 bg_inode_bitmap_hi;      /* Inodes bitmap block MSB */
  ub32 bg_inode_table_hi;       /* Inodes table block MSB */
  ub16 bg_free_blocks_count_hi; /* Free blocks count MSB */
  ub16 bg_free_inodes_count_hi; /* Free inodes count MSB */
  ub16 bg_used_dirs_count_hi;   /* Directories count MSB */
  ub16 bg_itable_unused_hi;     /* Unused inodes count MSB */
  ub32 bg_exclude_bitmap_hi;    /* Exclude bitmap block MSB */
  ub16 bg_block_bitmap_csum_hi; /* crc32c(s_uuid+grp_num+bitmap) MSB */
  ub16 bg_inode_bitmap_csum_hi; /* crc32c(s_uuid+grp_num+bitmap) MSB */
  ub32 bg_reserved;
} app_ext4_group_desc64;

#define EXT4_N_BLOCKS 15
typedef struct {
  ub16 i_mode;        /* File mode */
  ub16 i_uid;         /* Low 16 bits of Owner Uid */
  ub32 i_size;        /* Size in bytes */
  ub32 i_atime;       /* Access time */
  ub32 i_ctime;       /* Inode Change time */
  ub32 i_mtime;       /* Modification time */
  ub32 i_dtime;       /* Deletion Time */
  ub16 i_gid;         /* Low 16 bits of Group Id */
  ub16 i_links_count; /* Links count */
  ub32 i_blocks;      /* Blocks count */
  ub32 i_flags;       /* File flags */
  union {
    struct {
      ub32 l_i_version; /* was l_i_reserved1 */
    } linux1;
    struct {
      ub32 h_i_translator;
    } hurd1;
  } osd1;                      /* OS dependent 1 */
  ub32 i_block[EXT4_N_BLOCKS]; /* Pointers to blocks */
  ub32 i_generation;           /* File version (for NFS) */
  ub32 i_file_acl;             /* File ACL */
  ub32 i_size_high;            /* Formerly i_dir_acl, directory ACL */
  ub32 i_faddr;                /* Fragment address */
  union {
    struct {
      ub16 l_i_blocks_hi;
      ub16 l_i_file_acl_high;
      ub16 l_i_uid_high;    /* these 2 fields    */
      ub16 l_i_gid_high;    /* were reserved2[0] */
      ub16 l_i_checksum_lo; /* crc32c(uuid+inum+inode) */
      ub16 l_i_reserved;
    } linux2;
    struct {
      ub8 h_i_frag;  /* Fragment number */
      ub8 h_i_fsize; /* Fragment size */
      ub16 h_i_mode_high;
      ub16 h_i_uid_high;
      ub16 h_i_gid_high;
      ub32 h_i_author;
    } hurd2;
  } osd2; /* OS dependent 2 */
  ub16 i_extra_isize;
  ub16 i_checksum_hi;  /* crc32c(uuid+inum+inode) */
  ub32 i_ctime_extra;  /* extra Change time (nsec << 2 | epoch) */
  ub32 i_mtime_extra;  /* extra Modification time (nsec << 2 | epoch) */
  ub32 i_atime_extra;  /* extra Access time (nsec << 2 | epoch) */
  ub32 i_crtime;       /* File creation time */
  ub32 i_crtime_extra; /* extra File creation time (nsec << 2 | epoch)*/
  ub32 i_version_hi;   /* high 32 bits for 64-bit version */
} app_ext4_inode;

typedef struct {
  ub16 eh_magic;      /* probably will support different formats */
  ub16 eh_entries;    /* number of valid entries */
  ub16 eh_max;        /* capacity of store in entries */
  ub16 eh_depth;      /* has tree real underlaying blocks? */
  ub32 eh_generation; /* generation of the tree */
} app_ext4_extent_header;

typedef struct {
  ub32 ee_block;    /* first logical block extent covers */
  ub16 ee_len;      /* number of blocks covered by extent */
  ub16 ee_start_hi; /* high 16 bits of physical block */
  ub32 ee_start;    /* low 32 bigs of physical block */
} app_ext4_extent;

typedef struct {
  ub32 ei_block;   /* index covers logical blocks from 'block' */
  ub32 ei_leaf;    /* pointer to the physical block of the next *
                    * level. leaf or next index could bet here */
  ub16 ei_leaf_hi; /* high 16 bits of physical block */
  ub16 ei_unused;
} app_ext4_extent_idx;

typedef struct {
  ub32 h_magic;       /* magic number for identification */
  ub32 h_refcount;    /* reference count */
  ub32 h_blocks;      /* number of disk blocks used */
  ub32 h_hash;        /* hash value of all attributes */
  ub32 h_reserved[4]; /* zero right now */
} app_ext4_attr_header;

typedef struct {
  ub8 e_name_len;     /* length of name */
  ub8 e_name_index;   /* attribute name index */
  ub16 e_value_offs;  /* offset in disk block of value */
  ub32 e_value_block; /* disk block attribute is stored on (n/i) */
  ub32 e_value_size;  /* size of attribute value */
  ub32 e_hash;        /* hash value of name and value */
} app_ext4_attr_entry;

#define EXT2_NAME_LEN 255
typedef struct {
  ub32 inode;   /* Inode number */
  ub16 rec_len; /* Directory entry length */
  union {
    struct {
      ub8 name_len; /* Name length */
      ub8 file_type;
    };
    ub16 name_len_v2; /* Name length */
  };
  b8 name[EXT2_NAME_LEN]; /* File name */
} app_ext4_dir_entry;

typedef struct {
  ub32 reserved_zero;
  ub8 hash_version; /* 0 now, 1 at release */
  ub8 info_length;  /* 8 */
  ub8 indirect_levels;
  ub8 unused_flags;
} app_ext4_dx_header;

typedef struct {
  ub16 limit;  // maximum number of dx_entries that can follow this header
  ub16 count;  // actual number of dx_entries that can follow this header
  ub32 block;  // The block number that goes with the lowest hash value of
               // this block
} app_ext4_dx_info;

typedef struct {
  ub32 hash;
  ub32 block;
} app_ext4_dx_entry;
}  // namespace ext
}  // namespace filescanner
#endif
#endif
#ifndef DEFAULT_SCANNER_H_
#define DEFAULT_SCANNER_H_
#include "disk_scanner.h"
#include "folder_selector.h"
#include "volume_scanner.h"
#ifndef __linux__
#include <Windows.h>
#include <winternl.h>

typedef struct _FILE_DIRECTORY_INFORMATION {
  ULONG NextEntryOffset;
  ULONG FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG FileAttributes;
  ULONG FileNameLength;
  WCHAR FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

typedef NTSTATUS(WINAPI *PNTQUERYDIRECTORYFILE)(
    HANDLE FileHandle, PVOID Event, PVOID ApcRoutine, PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length,
    FILE_INFORMATION_CLASS FileInformationClass, BOOLEAN ReturnSingleEntry,
    PVOID FileName, BOOLEAN RestartScan);
#endif

namespace filescanner {
namespace default {

  class CDefaultScanner : public IVolumeScannerInterface {
   public:
    CDefaultScanner(const os_string &path, CDiskScanner *p_disk_scanner,
                    IDataHanderInterface *p_data_handler,
                    CFolderSelector *p_folder_selector);
    bool Scan();

   private:
    bool TraverseDir(const os_string &path);
#ifndef __linux__
    bool FasterTraverseDir(const os_string &path);
    PNTQUERYDIRECTORYFILE pNtQueryDirectoryFile_;
#endif

   private:
    os_string volume_name_;
    CDiskScanner *p_disk_scanner_;
    CFolderSelector *p_folder_selector_;
    IDataHanderInterface *p_data_handler_;
  };
}  // namespace default
}  // namespace filescanner
#endif

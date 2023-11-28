#include <filesystem>
#include <iostream>
#include <set>

#include "data_hander.h"
#include "disk_scanner.h"
using namespace filescanner;
#ifdef _WIN32
class DataStore : public IDataHanderInterface {
 public:
  virtual bool FileContentNeed() { return true; }

  virtual b32 OnFileContent(const os_string& filepath, const b8* contents,
                            const ulong& size) {
    FILE* fp = fopen("file.txt", "ab+");
    static b32 fsize = 0;
    if (fsize + size >= filesize) {
      fwrite(contents, 1, filesize - fsize, fp);
    } else {
      fwrite(contents, 1, size, fp);
      fsize += size;
    }

    fclose(fp);
    return 1;
  }
  virtual void OnFilePath(const common::os_string& filepath,
                          const b64& file_size) {
    fileset.emplace(filepath);
    filesize = file_size;
  }

  void Print() {
    setlocale(LC_ALL, "zh_CN");
    for (auto& it : fileset) {
      std::wcout << it.c_str() << std::endl;
    }
  }

 private:
  std::set<os_string> fileset;
  int filesize;
};

int main() {
  DataStore data;
  CDiskScanner scannner(&data);
  scannner.Scan(L"D:\\ibook\\游戏\\游戏系统");
  data.Print();

  return true;
}
#else
class DataStore : public IDataHanderInterface {
 public:
  virtual bool FileContentNeed() { return need; }

  virtual bool SetFileContentNeed(bool file) { need = file; }

  virtual b32 OnFileContent(const os_string& filepath, const b8* contents,
                            const ulong& size) {
    FILE* fp = fopen("file.txt", "ab+");
    static b32 fsize = 0;
    if (fsize + size >= filesize) {
      fwrite(contents, 1, filesize - fsize, fp);
    } else {
      fwrite(contents, 1, size, fp);
      fsize += size;
    }

    fclose(fp);

    return 1;
  }

  virtual void OnFilePath(const common::os_string& filepath,
                          const b64& file_size) {
    fileset.emplace(filepath);
    filesize = file_size;
  }

  void Print() {
    for (auto& it : fileset) {
      std::cout << it.c_str() << std::endl;
    }
  }

 private:
  std::set<os_string> fileset;
  int filesize;
  bool need;
};

int main() {
  DataStore data;
  data.SetFileContentNeed(true);
  CDiskScanner scannner(&data);
  scannner.Scan("/mnt/hgfs/ibook/都市/修真");
  data.Print();

  return true;
}
#endif
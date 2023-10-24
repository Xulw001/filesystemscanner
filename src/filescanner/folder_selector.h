#ifndef FOLDER_SELECTOR_H_
#define FOLDER_SELECTOR_H_
#include "string.h"
using common::os_string;
namespace filescanner {
class CFolderSelector {
 public:
  CFolderSelector(os_string path) : path_(path) { empty_ = true; };

  bool NeedScan(os_string path) {
    if (path.size() > path_.size()) {
      if (path.compare(0, path_.size(), path_) == 0) {
        return true;
      }
    } else {
      if (path_.compare(0, path.size(), path) == 0) {
        return true;
      }
    }
    return false;
  }

  void set_not_empty() { empty_ = false; }

  bool empty() { return empty_; }

 private:
  os_string path_;
  bool empty_;
};
}  // namespace filescanner
#endif
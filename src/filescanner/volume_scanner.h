#ifndef VOLUME_SCANNER_H_
#define VOLUME_SCANNER_H_

namespace filescanner {
class IVolumeScannerInterface {
 public:
  const b32 kBlockSize = 4096;
  virtual bool Scan() = 0;
  virtual ~IVolumeScannerInterface() { ; }
};
}  // namespace filescanner
#endif
#ifndef DIGEST_H_
#define DIGEST_H_
#include "common/type.h"

// reference openssl
namespace digest {
using namespace ::common;

#define DIGEST_LENGTH 20

class IDigestInterface {
 public:
  virtual ub32 Context_Init() = 0;
  virtual ub32 Context_Update(const ub8 *data, const b64 len) = 0;
  virtual ub32 Context_Final(ub8 *md) = 0;
  virtual ub8 *Digest(const ub8 *data, const b64 len, ub8 *md) = 0;
};
}  // namespace digest
#endif
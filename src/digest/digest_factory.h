#ifndef DIGEST_FACTORY_H_
#define DIGEST_FACTORY_H_
#include "sha/sha.h"

namespace digest {
class DigestFactory {
  enum DigestType {
    SHA1,
    SHA256,
    MD5,
  };

 public:
  static IDigestInterface *CreateDigest(const DigestType type) {
    switch (type) {
      case SHA1:
        return new Sha1Context();
      default:
        return nullptr;
    }
  }
};

#ifndef DIGEST_LENGTH
#define DIGEST_LENGTH SHA_DIGEST_LENGTH
#endif
}  // namespace digest
#endif
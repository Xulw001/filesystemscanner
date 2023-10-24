#ifndef _SCOPED_SOURCE_H_
#define _SCOPED_SOURCE_H_

namespace scope {
template <typename ResourceTraits>
class ScopedResource {
 public:
  typedef typename ResourceTraits::Type ResourceType;

  ScopedResource() = default;
  ScopedResource(const ScopedResource&) = delete;
  ScopedResource& operator=(const ScopedResource&) = delete;
  explicit ScopedResource(ResourceType resource) : resource_{resource} {}
  ~ScopedResource() { Clean(); }

  ScopedResource& operator=(const ResourceType& resource) {
    Clean();
    resource_ = resource;
    return *this;
  }

  // Check if resource is default
  operator bool() { return resource_ != ResourceTraits::Default; }

  // Returns resource
  operator const ResourceType&() const { return resource_; }

  // Returns the address of resource
  ResourceType* operator&() { return &resource_; }

 private:
  void Clean() {
    if (resource_ == ResourceTraits::Default) return;
    ResourceTraits::Clean(resource_);
    resource_ = ResourceTraits::Default;
  }

 private:
  ResourceType resource_ = ResourceTraits::Default;
};

template <typename Value>
class ScopedPtr {
 public:
  typedef Value* PtrType;

  ScopedPtr() = default;
  ScopedPtr(const ScopedPtr&) = delete;
  ScopedPtr& operator=(const ScopedPtr&) = delete;
  explicit ScopedPtr(PtrType ptr) : ptr_{ptr} {}
  ~ScopedPtr() { Clean(); }

  ScopedPtr& operator=(const PtrType& ptr) {
    Clean();
    ptr_ = ptr;
    return *this;
  }

  PtrType get() { return ptr_; }

  PtrType operator->() const { return ptr_; }

  Value operator*() const { return (*ptr_); }

  // Check if resource is default
  explicit operator bool() { return ptr_ != NULL; }

  // Returns resource
  operator const PtrType&() const { return ptr_; }

  // Returns the address of resource
  PtrType* operator&() { return &ptr_; }

 private:
  void Clean() {
    if (ptr_) {
      delete[] ptr_;
      ptr_ = NULL;
    }
  }

 private:
  PtrType ptr_ = NULL;
};

};  // namespace scope
#endif
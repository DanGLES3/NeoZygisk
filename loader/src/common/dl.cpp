#include "dl.h"

#include <android/dlext.h>
#include <dlfcn.h>
#include <libgen.h>

#include "logging.h"

extern "C" [[gnu::weak]] struct android_namespace_t*
// NOLINTNEXTLINE
__loader_android_create_namespace([[maybe_unused]] const char* name,
                                  [[maybe_unused]] const char* ld_library_path,
                                  [[maybe_unused]] const char* default_library_path,
                                  [[maybe_unused]] uint64_t type,
                                  [[maybe_unused]] const char* permitted_when_isolated_path,
                                  [[maybe_unused]] android_namespace_t* parent,
                                  [[maybe_unused]] const void* caller_addr);

void* DlopenExt(const char* path, int flags) {
    auto info = android_dlextinfo{};
    auto* dir = dirname(path);
    auto* ns = &__loader_android_create_namespace == nullptr
                   ? nullptr
                   : __loader_android_create_namespace(
                         path, dir, nullptr, 2, /* ANDROID_NAMESPACE_TYPE_SHARED */
                         nullptr, nullptr, reinterpret_cast<void*>(&DlopenExt));
    if (ns) {
        info.flags = ANDROID_DLEXT_USE_NAMESPACE;
        info.library_namespace = ns;

        LOGD("Open %s with namespace %p", path, ns);
    } else {
        LOGD("Cannot create namespace for %s", path);
    }

    auto* handle = android_dlopen_ext(path, flags, &info);
    if (handle) {
        LOGD("dlopen %s: %p", path, handle);
    } else {
        LOGE("dlopen %s: %s", path, dlerror());
    }
    return handle;
}

void* DlopenMem(int fd, int flags) {
    auto info = android_dlextinfo{.flags = ANDROID_DLEXT_USE_LIBRARY_FD,
                                  .reserved_addr = nullptr,
                                  .reserved_size = 0,
                                  .relro_fd = 0,
                                  .library_fd = fd,
                                  .library_fd_offset = 0,
                                  .library_namespace = nullptr};

    auto* handle = android_dlopen_ext("/jit-cache-zygisk", flags, &info);
    if (handle) {
        LOGV("dlopen fd %d: %p", fd, handle);
    } else {
        LOGE("dlopen fd %d: %s", fd, dlerror());
    }
    return handle;
}

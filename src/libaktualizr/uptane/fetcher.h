#ifndef UPTANE_FETCHER_H_
#define UPTANE_FETCHER_H_

#include <atomic>
#include <condition_variable>
#include <mutex>
#include "config/config.h"
#include "http/httpinterface.h"
#include "storage/invstorage.h"

namespace Uptane {

constexpr int64_t kMaxRootSize = 64 * 1024;
constexpr int64_t kMaxDirectorTargetsSize = 64 * 1024;
constexpr int64_t kMaxTimestampSize = 64 * 1024;
constexpr int64_t kMaxSnapshotSize = 64 * 1024;
constexpr int64_t kMaxImagesTargetsSize = 1024 * 1024;

class DownloadCounter {
 public:
  DownloadCounter(std::atomic_uint* value) : value_(value) { (*value_)++; }
  ~DownloadCounter() { (*value_)--; }

 private:
  std::atomic_uint* value_;
};

using FetcherProgressCb = std::function<void(const Uptane::Target&, const std::string&, unsigned int)>;

class Fetcher {
 public:
  Fetcher(const Config& config_in, std::shared_ptr<INvStorage> storage_in, std::shared_ptr<HttpInterface> http_in,
          FetcherProgressCb progress_cb_in = nullptr)
      : http(std::move(http_in)),
        storage(std::move(storage_in)),
        config(config_in),
        progress_cb(std::move(progress_cb_in)) {}
  bool fetchVerifyTarget(const Target& target);
  bool fetchRole(std::string* result, int64_t maxsize, RepositoryType repo, Uptane::Role role, Version version);
  bool fetchLatestRole(std::string* result, int64_t maxsize, RepositoryType repo, Uptane::Role role) {
    return fetchRole(result, maxsize, repo, role, Version());
  }
  void restoreHasherState(MultiPartHasher& hasher, StorageTargetRHandle* data);
  bool isPaused() {
    std::lock_guard<std::mutex> guard(mutex_);
    return pause_;
  }
  bool isDownloading() { return static_cast<bool>(downloading_); }
  void checkPause();
  void setRetry(bool retry) { retry_ = retry; }

  enum class PauseRet {
    /* Download was paused successfully. */
    kPaused = 0,
    /* Download was resumed successfully. */
    kResumed,
    /* Download was already paused, so there is nothing to do. */
    kAlreadyPaused,
    /* Download has already completed, so there is nothing to do. */
    kAlreadyComplete,
    /* No download is in progress, so there is nothing to do. */
    kNotDownloading,
    /* Download was not paused, so there is nothing to do. */
    kNotPaused,
  };
  PauseRet setPause(bool pause);

 private:
  std::shared_ptr<HttpInterface> http;
  std::shared_ptr<INvStorage> storage;
  const Config& config;
  std::atomic_uint downloading_{0};
  bool pause_{false};
  bool retry_{false};
  std::mutex mutex_;
  std::condition_variable cv_;
  FetcherProgressCb progress_cb;
};

struct DownloadMetaStruct {
  DownloadMetaStruct(Target target_in, FetcherProgressCb progress_cb_in)
      : hash_type{target_in.hashes()[0].type()},
        target{std::move(target_in)},
        fetcher{nullptr},
        progress_cb{std::move(progress_cb_in)} {}
  uint64_t downloaded_length{0};
  unsigned int last_progress{0};
  std::unique_ptr<StorageTargetWHandle> fhandle;
  const Hash::Type hash_type;
  MultiPartHasher& hasher() {
    switch (hash_type) {
      case Hash::Type::kSha256:
        return sha256_hasher;
      case Hash::Type::kSha512:
        return sha512_hasher;
      default:
        throw std::runtime_error("Unknown hash algorithm");
    }
  }
  Target target;
  Fetcher* fetcher;
  FetcherProgressCb progress_cb;

 private:
  MultiPartSHA256Hasher sha256_hasher;
  MultiPartSHA512Hasher sha512_hasher;
};

}  // namespace Uptane

#endif

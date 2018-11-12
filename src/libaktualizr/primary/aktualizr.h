#ifndef AKTUALIZR_H_
#define AKTUALIZR_H_

#include <atomic>
#include <memory>

#include <gtest/gtest.h>
#include <boost/signals2.hpp>

#include "config/config.h"
#include "sotauptaneclient.h"
#include "storage/invstorage.h"
#include "uptane/secondaryinterface.h"
#include "utilities/events.h"

class ApiQueue {
 public:
  void enqueue(const std::function<void()>& t) {
    std::lock_guard<std::mutex> lock(m_);
    q_.push(t);
    c_.notify_one();
  }

  std::function<void()> dequeue() {
    std::unique_lock<std::mutex> wait_lock(m_);
    while (q_.empty()) {
      c_.wait(wait_lock);
      if (shutdown_) {
        return std::function<void()>();
      }
    }
    std::function<void()> val = q_.front();
    q_.pop();
    return val;
  }
  void shutDown() {
    shutdown_ = true;
    enqueue(std::function<void()>());
  }

 private:
  std::queue<std::function<void()>> q_;
  mutable std::mutex m_;
  std::condition_variable c_;
  std::atomic_bool shutdown_{false};
};

/**
 * This class provides the main APIs necessary for launching and controlling
 * libaktualizr.
 */
class Aktualizr {
 public:
  /** Aktualizr requires a configuration object. Examples can be found in the
   *  config directory. */
  explicit Aktualizr(Config& config);
  ~Aktualizr() {
    Shutdown();
    if (api_thread_.joinable()) {
      api_thread_.join();
    }
  }
  Aktualizr(const Aktualizr&) = delete;
  Aktualizr& operator=(const Aktualizr&) = delete;

  /*
   * Initialize aktualizr. Any secondaries should be added before making this
   * call. This will provision with the server if required. This must be called
   * before using any other aktualizr functions except AddSecondary.
   */
  void Initialize();

  /**
   * Asynchronously run aktualizr indefinitely until Shutdown is called.
   * Intended to be used with the Full \ref RunningMode setting.
   * @return Empty std::future object
   */
  std::future<void> RunForever();

  /**
   * Asynchronously shut aktualizr down if it is running indefinitely with the
   * Full \ref RunningMode.
   */
  void Shutdown();

  /**
   * Check for campaigns.
   * Campaigns are a concept outside of Uptane, and allow for user approval of
   * updates before the contents of the update are known.
   * @return std::future object with data about available campaigns.
   */
  std::future<CampaignCheckResult> CampaignCheck();

  /**
   * Accept a campaign for the current device.
   * Campaigns are a concept outside of Uptane, and allow for user approval of
   * updates before the contents of the update are known.
   * @param campaign_id Campaign ID as provided by CampaignCheck.
   * @return Empty std::future object
   */
  std::future<void> CampaignAccept(const std::string& campaign_id);

  /**
   * Send local device data to the server.
   * This includes network status, installed packages, hardware etc.
   * @return Empty std::future object
   */
  std::future<void> SendDeviceData();

  /**
   * Fetch Uptane metadata and check for updates.
   * This collects a client manifest, PUTs it to the director, updates the
   * Uptane metadata (including root and targets), and then checks the metadata
   * for target updates.
   * @return Information about available updates.
   */
  std::future<UpdateCheckResult> CheckUpdates();

  /**
   * Download targets.
   * @param updates Vector of targets to download as provided by CheckUpdates.
   * @return std::future object with information about download results.
   */
  std::future<DownloadResult> Download(const std::vector<Uptane::Target>& updates);

  /**
   * Get target downloaded in Download call. Returned target is guaranteed to be verified and up-to-date
   * according to the Uptane metadata downloaded in CheckUpdates call.
   * @param filename Name of the binary in the storage
   * @return Handle to the stored binary. nullptr if none is found.
   */
  std::unique_ptr<StorageTargetRHandle> GetStoredTarget(const Uptane::Target& target);

  /**
   * Install targets.
   * @param updates Vector of targets to install as provided by CheckUpdates or
   * Download.
   * @return std::future object with information about installation results.
   */
  std::future<InstallResult> Install(const std::vector<Uptane::Target>& updates);

  /**
   * Send installation report to the backend.
   * @param custom Project-specific data to put in the custom field of Uptane manifest
   * @return std::future object with manifest update result.
   */
  std::future<bool> SendManifest(const Json::Value& custom = Json::nullValue);

  /**
   * Pause a download current in progress.
   * @return Information about pause results.
   */
  PauseResult Pause();

  /**
   * Resume a paused download.
   * @return Information about resume results.
   */
  PauseResult Resume();

  /**
   * Synchronously run an uptane cycle.
   *
   * Behaviour depends on the configured running mode (full cycle, check and
   * download or check and install)
   */
  void UptaneCycle();

  /**
   * Add new secondary to aktualizr. Must be called before Initialize.
   * @param secondary An object to perform installation on a secondary ECU.
   */
  void AddSecondary(const std::shared_ptr<Uptane::SecondaryInterface>& secondary);

  /**
   * Provide a function to receive event notifications.
   * @param handler a function that can receive event objects.
   * @return a signal connection object, which can be disconnected if desired.
   */
  boost::signals2::connection SetSignalHandler(std::function<void(std::shared_ptr<event::BaseEvent>)>& handler);

 private:
  FRIEND_TEST(Aktualizr, FullNoUpdates);
  FRIEND_TEST(Aktualizr, FullWithUpdates);
  FRIEND_TEST(Aktualizr, FullMultipleSecondaries);
  FRIEND_TEST(Aktualizr, CheckWithUpdates);
  FRIEND_TEST(Aktualizr, DownloadWithUpdates);
  FRIEND_TEST(Aktualizr, InstallWithUpdates);
  FRIEND_TEST(Aktualizr, CampaignCheck);
  FRIEND_TEST(Aktualizr, FullNoCorrelationId);
  FRIEND_TEST(Aktualizr, ManifestCustom);
  FRIEND_TEST(Aktualizr, APICheck);
  Aktualizr(Config& config, std::shared_ptr<INvStorage> storage_in, std::shared_ptr<SotaUptaneClient> uptane_client_in,
            std::shared_ptr<event::Channel> sig_in);
  void systemSetup();

  Config& config_;
  std::shared_ptr<INvStorage> storage_;
  std::shared_ptr<SotaUptaneClient> uptane_client_;
  std::shared_ptr<event::Channel> sig_;
  std::atomic<bool> shutdown_ = {false};
  ApiQueue api_queue_;
  std::thread api_thread_;
};

#endif  // AKTUALIZR_H_

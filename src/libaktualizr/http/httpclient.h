#ifndef HTTPCLIENT_H_
#define HTTPCLIENT_H_

#include <curl/curl.h>
#ifdef AKTUALIZR_ENABLE_TESTS
#include <gtest/gtest.h>
#endif
#include <memory>
#include "json/json.h"

#include "httpinterface.h"
#include "logging/logging.h"
#include "utilities/utils.h"

/**
 * Helper class to manage curl_global_init/curl_global_cleanup calls
 */
class CurlGlobalInitWrapper {
 public:
  CurlGlobalInitWrapper() { curl_global_init(CURL_GLOBAL_DEFAULT); }
  ~CurlGlobalInitWrapper() { curl_global_cleanup(); }
  CurlGlobalInitWrapper &operator=(const CurlGlobalInitWrapper &) = delete;
  CurlGlobalInitWrapper(const CurlGlobalInitWrapper &) = delete;
  CurlGlobalInitWrapper &operator=(CurlGlobalInitWrapper &&) = delete;
  CurlGlobalInitWrapper(CurlGlobalInitWrapper &&) = delete;
};

class HttpClient : public HttpInterface {
 public:
  HttpClient();
  HttpClient(const HttpClient & /*curl_in*/);
  ~HttpClient() override;
  HttpResponse get(const std::string &url, int64_t maxsize) override;
  HttpResponse post(const std::string &url, const Json::Value &data) override;
  HttpResponse put(const std::string &url, const Json::Value &data) override;

  HttpResponse download(const std::string &url, curl_write_callback callback, void *userp) override;
  void setCerts(const std::string &ca, CryptoSource ca_source, const std::string &cert, CryptoSource cert_source,
                const std::string &pkey, CryptoSource pkey_source) override;
  long http_code{};  // NOLINT

 private:
#ifdef AKTUALIZR_ENABLE_TESTS
  FRIEND_TEST(GetTest, download_speed_limit);
#endif
  /**
   * These are here to catch a common programming error where a Json::Value is
   * implicitly constructed from a std::string. By having an private overload
   * that takes string (and with no implementation), this will fail during
   * compilation.
   */
  HttpResponse post(const std::string &url, std::string data);
  HttpResponse put(const std::string &url, std::string data);

  static CurlGlobalInitWrapper manageCurlGlobalInit_;
  CURL *curl;
  curl_slist *headers;
  HttpResponse perform(CURL *curl_handler, int retry_times, int64_t size_limit);
  std::string user_agent;

  static CURLcode sslCtxFunction(CURL *handle, void *sslctx, void *parm);
  std::unique_ptr<TemporaryFile> tls_ca_file;
  std::unique_ptr<TemporaryFile> tls_cert_file;
  std::unique_ptr<TemporaryFile> tls_pkey_file;
  static const int RETRY_TIMES = 2;
  static const long kSpeedLimitTimeInterval = 60L;   // NOLINT
  static const long kSpeedLimitBytesPerSec = 5000L;  // NOLINT

  long speed_limit_time_interval_{kSpeedLimitTimeInterval};                // NOLINT
  long speed_limit_bytes_per_sec_{kSpeedLimitBytesPerSec};                 // NOLINT
  void overrideSpeedLimitParams(long time_interval, long bytes_per_sec) {  // NOLINT
    speed_limit_time_interval_ = time_interval;
    speed_limit_bytes_per_sec_ = bytes_per_sec;
  }
  bool pkcs11_key{false};
  bool pkcs11_cert{false};
};
#endif

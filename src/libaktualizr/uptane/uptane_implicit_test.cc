/**
 * \file
 */

#include <gtest/gtest.h>

#include <boost/filesystem.hpp>
#include <boost/smart_ptr/make_shared.hpp>

#include "httpfake.h"
#include "logging/logging.h"
#include "primary/initializer.h"
#include "primary/sotauptaneclient.h"
#include "storage/invstorage.h"
#include "uptane/uptanerepository.h"
#include "utilities/utils.h"

/**
 * Verify that when using implicit provisioning, aktualizr halts if credentials
 * are not available.
 */
TEST(UptaneImplicit, ImplicitFailure) {
  RecordProperty("zephyr_key", "OTA-1209,TST-185");
  TemporaryDirectory temp_dir;
  Config config;
  // Set device_id to prevent trying to read it from the certificate.
  config.provision.device_id = "device_id";
  config.storage.path = temp_dir.Path();
  EXPECT_EQ(config.provision.mode, ProvisionMode::kImplicit);

  auto storage = INvStorage::newStorage(config.storage);
  auto http = std::make_shared<HttpFake>(temp_dir.Path());
  KeyManager keys(storage, config.keymanagerConfig());

  Initializer initializer(config.provision, storage, http, keys, {});
  EXPECT_FALSE(initializer.isSuccessful());
}

/**
 * Verfiy that aktualizr halts when provided incomplete implicit provisioning
 * credentials.
 */
TEST(UptaneImplicit, ImplicitIncomplete) {
  RecordProperty("zephyr_key", "OTA-1209,TST-187");
  TemporaryDirectory temp_dir;
  Config config;
  // Set device_id to prevent trying to read it from the certificate.
  config.provision.device_id = "device_id";
  config.storage.path = temp_dir.Path();
  config.import.base_path = temp_dir / "import";
  EXPECT_EQ(config.provision.mode, ProvisionMode::kImplicit);

  auto http = std::make_shared<HttpFake>(temp_dir.Path());

  {
    config.import.tls_cacert_path = BasedPath("ca.pem");
    config.import.tls_clientcert_path = BasedPath("");
    config.import.tls_pkey_path = BasedPath("");
    Utils::createDirectories(temp_dir / "import", S_IRWXU);
    boost::filesystem::copy_file("tests/test_data/implicit/ca.pem", temp_dir / "import/ca.pem");
    auto storage = INvStorage::newStorage(config.storage);
    storage->importData(config.import);
    KeyManager keys(storage, config.keymanagerConfig());

    Initializer initializer(config.provision, storage, http, keys, {});
    EXPECT_FALSE(initializer.isSuccessful());
  }

  {
    config.import.tls_cacert_path = BasedPath("");
    config.import.tls_clientcert_path = BasedPath("client.pem");
    config.import.tls_pkey_path = BasedPath("");
    boost::filesystem::remove_all(temp_dir.Path());
    Utils::createDirectories(temp_dir / "import", S_IRWXU);
    boost::filesystem::copy_file("tests/test_data/implicit/client.pem", temp_dir / "import/client.pem");
    auto storage = INvStorage::newStorage(config.storage);
    storage->importData(config.import);
    KeyManager keys(storage, config.keymanagerConfig());

    Initializer initializer(config.provision, storage, http, keys, {});
    EXPECT_FALSE(initializer.isSuccessful());
  }

  {
    config.import.tls_cacert_path = BasedPath("");
    config.import.tls_clientcert_path = BasedPath("");
    config.import.tls_pkey_path = BasedPath("pkey.pem");
    boost::filesystem::remove_all(temp_dir.Path());
    Utils::createDirectories(temp_dir / "import", S_IRWXU);
    boost::filesystem::copy_file("tests/test_data/implicit/pkey.pem", temp_dir / "import/pkey.pem");
    auto storage = INvStorage::newStorage(config.storage);
    storage->importData(config.import);
    KeyManager keys(storage, config.keymanagerConfig());

    Initializer initializer(config.provision, storage, http, keys, {});
    EXPECT_FALSE(initializer.isSuccessful());
  }

  {
    config.import.tls_cacert_path = BasedPath("ca.pem");
    config.import.tls_clientcert_path = BasedPath("client.pem");
    config.import.tls_pkey_path = BasedPath("");
    boost::filesystem::remove_all(temp_dir.Path());
    Utils::createDirectories(temp_dir / "import", S_IRWXU);
    boost::filesystem::copy_file("tests/test_data/implicit/ca.pem", temp_dir / "import/ca.pem");
    boost::filesystem::copy_file("tests/test_data/implicit/client.pem", temp_dir / "import/client.pem");
    auto storage = INvStorage::newStorage(config.storage);
    storage->importData(config.import);
    KeyManager keys(storage, config.keymanagerConfig());

    Initializer initializer(config.provision, storage, http, keys, {});
    EXPECT_FALSE(initializer.isSuccessful());
  }

  {
    config.import.tls_cacert_path = BasedPath("ca.pem");
    config.import.tls_clientcert_path = BasedPath("");
    config.import.tls_pkey_path = BasedPath("pkey.pem");
    boost::filesystem::remove_all(temp_dir.Path());
    Utils::createDirectories(temp_dir / "import", S_IRWXU);
    boost::filesystem::copy_file("tests/test_data/implicit/ca.pem", temp_dir / "import/ca.pem");
    boost::filesystem::copy_file("tests/test_data/implicit/pkey.pem", temp_dir / "import/pkey.pem");
    auto storage = INvStorage::newStorage(config.storage);
    storage->importData(config.import);
    KeyManager keys(storage, config.keymanagerConfig());

    Initializer initializer(config.provision, storage, http, keys, {});
    EXPECT_FALSE(initializer.isSuccessful());
  }

  {
    config.import.tls_cacert_path = BasedPath("");
    config.import.tls_clientcert_path = BasedPath("client.pem");
    config.import.tls_pkey_path = BasedPath("pkey.pem");
    boost::filesystem::remove_all(temp_dir.Path());
    Utils::createDirectories(temp_dir / "import", S_IRWXU);
    boost::filesystem::copy_file("tests/test_data/implicit/client.pem", temp_dir / "import/client.pem");
    boost::filesystem::copy_file("tests/test_data/implicit/pkey.pem", temp_dir / "import/pkey.pem");
    auto storage = INvStorage::newStorage(config.storage);
    storage->importData(config.import);
    KeyManager keys(storage, config.keymanagerConfig());

    Initializer initializer(config.provision, storage, http, keys, {});
    EXPECT_FALSE(initializer.isSuccessful());
  }

  // Do one last round with all three files to make sure it actually works as
  // expected.
  config.import.tls_cacert_path = BasedPath("ca.pem");
  config.import.tls_clientcert_path = BasedPath("client.pem");
  config.import.tls_pkey_path = BasedPath("pkey.pem");
  boost::filesystem::remove_all(temp_dir.Path());
  Utils::createDirectories(temp_dir / "import", S_IRWXU);
  boost::filesystem::copy_file("tests/test_data/implicit/ca.pem", temp_dir / "import/ca.pem");
  boost::filesystem::copy_file("tests/test_data/implicit/client.pem", temp_dir / "import/client.pem");
  boost::filesystem::copy_file("tests/test_data/implicit/pkey.pem", temp_dir / "import/pkey.pem");
  auto storage = INvStorage::newStorage(config.storage);
  storage->importData(config.import);
  KeyManager keys(storage, config.keymanagerConfig());

  Initializer initializer(config.provision, storage, http, keys, {});
  EXPECT_TRUE(initializer.isSuccessful());
}

/**
 * Verify that aktualizr can implicitly provision with provided credentials.
 */
TEST(UptaneImplicit, ImplicitProvision) {
  RecordProperty("zephyr_key", "OTA-996,OTA-1210,TST-186");
  TemporaryDirectory temp_dir;
  Config config;
  Utils::createDirectories(temp_dir / "import", S_IRWXU);
  boost::filesystem::copy_file("tests/test_data/implicit/ca.pem", temp_dir / "import/ca.pem");
  boost::filesystem::copy_file("tests/test_data/implicit/client.pem", temp_dir / "import/client.pem");
  boost::filesystem::copy_file("tests/test_data/implicit/pkey.pem", temp_dir / "import/pkey.pem");
  config.storage.path = temp_dir.Path();
  config.import.base_path = temp_dir / "import";
  config.import.tls_cacert_path = BasedPath("ca.pem");
  config.import.tls_clientcert_path = BasedPath("client.pem");
  config.import.tls_pkey_path = BasedPath("pkey.pem");
  EXPECT_EQ(config.provision.mode, ProvisionMode::kImplicit);

  auto storage = INvStorage::newStorage(config.storage);
  storage->importData(config.import);
  auto http = std::make_shared<HttpFake>(temp_dir.Path());
  KeyManager keys(storage, config.keymanagerConfig());

  Initializer initializer(config.provision, storage, http, keys, {});
  EXPECT_TRUE(initializer.isSuccessful());
}

#ifndef __NO_MAIN__
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  logger_set_threshold(boost::log::trivial::trace);
  return RUN_ALL_TESTS();
}
#endif

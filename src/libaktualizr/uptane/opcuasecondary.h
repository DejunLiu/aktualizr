#ifndef UPTANE_OPCUASECONDARY_H_
#define UPTANE_OPCUASECONDARY_H_

#include "json/json.h"
#include "secondaryinterface.h"

#include "utilities/types.h"

namespace Uptane {

struct MetaPack;
class SecondaryConfig;

class OpcuaSecondary : public SecondaryInterface {
 public:
  explicit OpcuaSecondary(const SecondaryConfig& sconfig_in);
  ~OpcuaSecondary() override;

  Uptane::EcuSerial getSerial() override;
  Uptane::HardwareIdentifier getHwId() override;
  PublicKey getPublicKey() override;

  Json::Value getManifest() override;
  bool putMetadata(const RawMetaPack& meta_pack) override;

  bool sendFirmware(const std::shared_ptr<std::string>& data) override;

  int getRootVersion(bool director) override;
  bool putRoot(const std::string& root, bool director) override;
};

}  // namespace Uptane

#endif  // UPTANE_OPCUASECONDARY_H_

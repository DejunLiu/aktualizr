#include "imagesrepository.h"

namespace Uptane {

void ImagesRepository::resetMeta() {
  resetRoot();
  targets = Targets();
  snapshot = Snapshot();
  timestamp = TimestampMeta();
}

bool ImagesRepository::verifyTimestamp(const std::string& timestamp_raw) {
  try {
    timestamp =
        TimestampMeta(RepositoryType::Image(), Utils::parseJSON(timestamp_raw), root);  // signature verification
  } catch (const Exception& e) {
    LOG_ERROR << "Signature verification for timestamp metadata failed";
    last_exception = e;
    return false;
  }
  return true;
}

bool ImagesRepository::verifySnapshot(const std::string& snapshot_raw) {
  try {
    const std::string canonical = Utils::jsonToCanonicalStr(Utils::parseJSON(snapshot_raw));
    bool hash_exists = false;
    for (const auto& it : timestamp.snapshot_hashes()) {
      switch (it.type()) {
        case Hash::Type::kSha256:
          if (Hash(Hash::Type::kSha256, boost::algorithm::hex(Crypto::sha256digest(canonical))) != it) {
            LOG_ERROR << "Hash verification for snapshot metadata failed";
            return false;
          }
          hash_exists = true;
          break;
        case Hash::Type::kSha512:
          if (Hash(Hash::Type::kSha512, boost::algorithm::hex(Crypto::sha512digest(canonical))) != it) {
            LOG_ERROR << "Hash verification for snapshot metadata failed";
            return false;
          }
          hash_exists = true;
          break;
        default:
          break;
      }
    }
    if (!hash_exists) {
      LOG_ERROR << "No hash found for shapshot.json";
      return false;
    }
    snapshot = Snapshot(RepositoryType::Image(), Utils::parseJSON(snapshot_raw), root);  // signature verification
    if (snapshot.version() != timestamp.snapshot_version()) {
      return false;
    }
  } catch (const Exception& e) {
    LOG_ERROR << "Signature verification for snapshot metadata failed";
    last_exception = e;
    return false;
  }
  return true;
}

bool ImagesRepository::verifyTargets(const std::string& targets_raw, const bool top_level) {
  try {
    const Json::Value targets_json = Utils::parseJSON(targets_raw);
    const std::string canonical = Utils::jsonToCanonicalStr(targets_json);
    bool hash_exists = false;
    for (const auto& it : snapshot.targets_hashes()) {
      switch (it.type()) {
        case Hash::Type::kSha256:
          if (Hash(Hash::Type::kSha256, boost::algorithm::hex(Crypto::sha256digest(canonical))) != it) {
            LOG_ERROR << "Hash verification for targets metadata failed";
            return false;
          }
          hash_exists = true;
          break;
        case Hash::Type::kSha512:
          if (Hash(Hash::Type::kSha512, boost::algorithm::hex(Crypto::sha512digest(canonical))) != it) {
            LOG_ERROR << "Hash verification for targets metadata failed";
            return false;
          }
          hash_exists = true;
          break;
        default:
          break;
      }
    }
    if (!hash_exists) {
      LOG_ERROR << "No hash found for targets.json";
      return false;
    }
    Uptane::Targets new_targets = Targets(RepositoryType::Image(), targets_json, root);  // signature verification
    if (new_targets.version() != snapshot.targets_version()) {
      return false;
    }
    if (top_level) {
      // Start over with fresh targets.
      targets = std::move(new_targets);
    } else {
      // Add delegated targets to the existing list.
      // TODO: this assumes no nested delegations. To support nested
      // delegations, we either need better data structure management or to copy
      // the delegation and key structures.
      for (const Uptane::Target& target : new_targets.targets) {
        targets.targets.push_back(target);
      }
    }
  } catch (const Exception& e) {
    LOG_ERROR << "Signature verification for images targets metadata failed";
    last_exception = e;
    return false;
  }
  return true;
}

// TODO: nested delegation support. This should work for first-order delegations
// because we put all Targets into the same structure.
std::unique_ptr<Uptane::Target> ImagesRepository::getTarget(const Uptane::Target& director_target) {
  auto it = std::find(targets.targets.cbegin(), targets.targets.cend(), director_target);
  if (it == targets.targets.cend()) {
    return std::unique_ptr<Uptane::Target>(nullptr);
  } else {
    return std_::make_unique<Uptane::Target>(*it);
  }
}

}  // namespace Uptane

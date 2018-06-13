#include "utilities/types.h"

#include <stdexcept>
#include <utility>

namespace data {
Json::Value Package::toJson() {
  Json::Value json;
  json["name"] = name;
  json["version"] = version;
  return json;
}

Package Package::fromJson(const std::string& json_str) {
  Json::Reader reader;
  Json::Value json;
  reader.parse(json_str, json);
  Package package;
  package.name = json["name"].asString();
  package.version = json["version"].asString();
  return package;
}

OperationResult::OperationResult(std::string id_in, UpdateResultCode result_code_in, std::string result_text_in)
    : id(std::move(id_in)), result_code(result_code_in), result_text(std::move(result_text_in)) {}

InstallOutcome OperationResult::toOutcome() { return InstallOutcome(result_code, result_text); }

Json::Value OperationResult::toJson() {
  Json::Value json;
  json["id"] = id;
  json["result_code"] = static_cast<int>(result_code);
  json["result_text"] = result_text;
  return json;
}

OperationResult OperationResult::fromJson(const std::string& json_str) {
  Json::Reader reader;
  Json::Value json;
  reader.parse(json_str, json);
  OperationResult operation_result;
  operation_result.id = json["id"].asString();
  operation_result.result_code = static_cast<UpdateResultCode>(json["result_code"].asUInt());
  operation_result.result_text = json["result_text"].asString();
  return operation_result;
}

OperationResult OperationResult::fromOutcome(const std::string& id, const InstallOutcome& outcome) {
  OperationResult operation_result;
  operation_result.id = id;
  operation_result.result_code = outcome.first;
  operation_result.result_text = outcome.second;
  return operation_result;
}

}  // namespace data

RunningMode RunningModeFromString(const std::string& mode) {
  if (mode == "full" || mode.empty()) {
    return RunningMode::kFull;
  } else if (mode == "once") {
    return RunningMode::kOnce;
  } else if (mode == "check") {
    return RunningMode::kCheck;
  } else if (mode == "download") {
    return RunningMode::kDownload;
  } else if (mode == "install") {
    return RunningMode::kInstall;
  } else {
    throw std::runtime_error(std::string("Incorrect running mode: ") + mode);
  }
}

std::string StringFromRunningMode(RunningMode mode) {
  std::string mode_str = "full";
  if (mode == RunningMode::kFull) {
    mode_str = "full";
  } else if (mode == RunningMode::kOnce) {
    mode_str = "once";
  } else if (mode == RunningMode::kCheck) {
    mode_str = "check";
  } else if (mode == RunningMode::kDownload) {
    mode_str = "download";
  } else if (mode == RunningMode::kInstall) {
    mode_str = "install";
  }
  return mode_str;
}

// vim: set tabstop=2 shiftwidth=2 expandtab:

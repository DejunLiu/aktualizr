#include <string>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "accumulator.h"
#include "deploy.h"
#include "garage_common.h"
#include "logging/logging.h"
#include "ostree_dir_repo.h"
#include "ostree_repo.h"

namespace po = boost::program_options;

int main(int argc, char **argv) {
  logger_init();

  int verbosity;
  boost::filesystem::path repo_path;
  std::string ref;
  boost::filesystem::path credentials_path;
  std::string cacerts;
  int max_curl_requests;
  RunMode mode = RunMode::kDefault;
  po::options_description desc("garage-push command line options");
  // clang-format off
  desc.add_options()
    ("help", "print usage")
    ("version", "Current garage-deploy version")
    ("verbose,v", accumulator<int>(&verbosity), "Verbose logging (use twice for more information)")
    ("quiet,q", "Quiet mode")
    ("repo,C", po::value<boost::filesystem::path>(&repo_path)->required(), "location of ostree repo")
    ("ref,r", po::value<std::string>(&ref)->required(), "ref to push")
    ("credentials,j", po::value<boost::filesystem::path>(&credentials_path)->required(), "credentials (json or zip containing json)")
    ("cacert", po::value<std::string>(&cacerts), "override path to CA root certificates, in the same format as curl --cacert")
    ("jobs", po::value<int>(&max_curl_requests)->default_value(30), "maximum number of parallel requests")
    ("dry-run,n", "check arguments and authenticate but don't upload")
    ("walk-tree,w", "walk entire tree and upload all missing objects");
  // clang-format on

  po::variables_map vm;

  try {
    po::store(po::parse_command_line(argc, reinterpret_cast<const char *const *>(argv), desc), vm);

    if (vm.count("help") != 0u) {
      LOG_INFO << desc;
      return EXIT_SUCCESS;
    }
    if (vm.count("version") != 0) {
      LOG_INFO << "Current garage-push version is: " << GARAGE_TOOLS_VERSION;
      exit(EXIT_SUCCESS);
    }
    po::notify(vm);
  } catch (const po::error &o) {
    LOG_ERROR << o.what();
    LOG_ERROR << desc;
    return EXIT_FAILURE;
  }

  // Configure logging
  if (verbosity == 0) {
    // 'verbose' trumps 'quiet'
    if (static_cast<int>(vm.count("quiet")) != 0) {
      logger_set_threshold(boost::log::trivial::warning);
    } else {
      logger_set_threshold(boost::log::trivial::info);
    }
  } else if (verbosity == 1) {
    logger_set_threshold(boost::log::trivial::debug);
    LOG_DEBUG << "Debug level debugging enabled";
  } else if (verbosity > 1) {
    logger_set_threshold(boost::log::trivial::trace);
    LOG_TRACE << "Trace level debugging enabled";
  } else {
    assert(0);
  }

  if (cacerts != "") {
    if (!boost::filesystem::exists(cacerts)) {
      LOG_FATAL << "--cacert path " << cacerts << " does not exist";
      return EXIT_FAILURE;
    }
  }

  if (vm.count("dry-run") != 0u) {
    mode = RunMode::kDryRun;
  }
  if (vm.count("walk-tree") != 0u) {
    // If --walk-tree and --dry-run were provided, walk but do not push.
    if (mode == RunMode::kDryRun) {
      mode = RunMode::kWalkTree;
    } else {
      mode = RunMode::kPushTree;
    }
  }

  if (max_curl_requests < 1) {
    LOG_FATAL << "--jobs must be greater than 0";
    return EXIT_FAILURE;
  }

  OSTreeRepo::ptr src_repo = std::make_shared<OSTreeDirRepo>(repo_path);
  if (!src_repo->LooksValid()) {
    LOG_FATAL << "The OSTree src repository does not appear to contain a valid OSTree repository";
    return EXIT_FAILURE;
  }

  try {
    ServerCredentials push_credentials(credentials_path);
    OSTreeRef ostree_ref = src_repo->GetRef(ref);
    if (!ostree_ref.IsValid()) {
      LOG_FATAL << "Ref " << ref << " was not found in repository " << repo_path.string();
      return EXIT_FAILURE;
    }

    OSTreeHash commit(ostree_ref.GetHash());

    if (!UploadToTreehub(src_repo, push_credentials, commit, cacerts, mode, max_curl_requests)) {
      LOG_FATAL << "Upload to treehub failed";
      return EXIT_FAILURE;
    }

    if (push_credentials.CanSignOffline()) {
      LOG_INFO << "Credentials contain offline signing keys, not pushing root ref";
    } else {
      if (!PushRootRef(push_credentials, ostree_ref, cacerts, mode)) {
        LOG_FATAL << "Could not push root reference to treehub";
        return EXIT_FAILURE;
      }
    }
  } catch (const BadCredentialsArchive &e) {
    LOG_FATAL << e.what();
    return EXIT_FAILURE;
  } catch (const BadCredentialsContent &e) {
    LOG_FATAL << e.what();
    return EXIT_FAILURE;
  } catch (const BadCredentialsJson &e) {
    LOG_FATAL << e.what();
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
// vim: set tabstop=2 shiftwidth=2 expandtab:

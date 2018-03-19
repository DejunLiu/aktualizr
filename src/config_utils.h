#ifndef CONFIG_UTILS_H_
#define CONFIG_UTILS_H_

#include <string>

#include <boost/property_tree/ini_parser.hpp>

#include "logging.h"
#include "utils.h"

/*
 The following uses a small amount of template hackery to provide a nice
 interface to load the sota.toml config file. StripQuotesFromStrings is
 templated, and passes everything that isn't a string straight through.
 Strings in toml are always double-quoted, and we remove them by specializing
 StripQuotesFromStrings for std::string.

 The end result is that the sequence of calls in Config::updateFromToml are
 pretty much a direct expression of the required behaviour: load this variable
 from this config entry, and print a warning at the

 Note that default values are defined by Config's default constructor.
 */

template <typename T>
inline T StripQuotesFromStrings(const T& value);

template <>
inline std::string StripQuotesFromStrings<std::string>(const std::string& value) {
  return Utils::stripQuotes(value);
}

template <typename T>
inline T StripQuotesFromStrings(const T& value) {
  return value;
}

template <typename T>
inline T addQuotesToStrings(const T& value);

template <>
inline std::string addQuotesToStrings<std::string>(const std::string& value) {
  return Utils::addQuotes(value);
}

template <typename T>
inline T addQuotesToStrings(const T& value) {
  return value;
}

template <typename T>
inline void writeOption(std::ofstream& sink, const T& data, const std::string& option_name) {
  sink << option_name << " = " << addQuotesToStrings(data) << "\n";
}

template <typename T>
inline void CopyFromConfig(T& dest, const std::string& option_name, boost::log::trivial::severity_level warning_level,
                           const boost::property_tree::ptree& pt) {
  boost::optional<T> value = pt.get_optional<T>(option_name);
  if (value.is_initialized()) {
    dest = StripQuotesFromStrings(value.get());
  } else {
    static boost::log::sources::severity_logger<boost::log::trivial::severity_level> logger;
    BOOST_LOG_SEV(logger, static_cast<boost::log::trivial::severity_level>(warning_level))
        << option_name << " not in config file. Using default:" << dest;
  }
}

#endif  // CONFIG_UTILS_H_
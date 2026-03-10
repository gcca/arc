#pragma once

#include <filesystem>
#include <fmt/color.h>
#include <string>

#include <toml++/toml.hpp>

class ArcSession {
public:
  explicit ArcSession(std::string path = ".arc.toml")
      : path_(std::filesystem::absolute(path)) {}

  const std::filesystem::path &Path() const { return path_; }

  toml::table Load() const {
    fmt::print("{} Session file: {}\n",
               fmt::styled("[arc]", fmt::fg(fmt::color::steel_blue) |
                                        fmt::emphasis::bold),
               fmt::styled(path_.string(), fmt::emphasis::italic));
    if (!std::filesystem::exists(path_)) {
      fmt::print(stderr,
                 "{} Warning: session file not found, continuing with "
                 "defaults.\n",
                 fmt::styled("[arc]", fmt::fg(fmt::color::orange) |
                                          fmt::emphasis::bold));
      return {};
    }
    try {
      toml::table tbl = toml::parse_file(path_.string());
      fmt::print("{} Session file loaded.\n",
                 fmt::styled("[arc]", fmt::fg(fmt::color::steel_blue) |
                                          fmt::emphasis::bold));
      return tbl;
    } catch (const std::exception &e) {
      fmt::print(stderr, "{} Warning: could not parse session file '{}': {}\n",
                 fmt::styled("[arc]",
                             fmt::fg(fmt::color::orange) | fmt::emphasis::bold),
                 fmt::styled(path_.string(), fmt::emphasis::italic), e.what());
      return {};
    }
  }

private:
  std::filesystem::path path_;
};

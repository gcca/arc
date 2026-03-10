#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <toml++/toml.hpp>

class CmdSession {
public:
  explicit CmdSession(std::filesystem::path file = ".arc.toml");

  int Run(const std::vector<std::string> &args);

private:
  std::filesystem::path file_;

  int Create();
  int Set(const std::vector<std::string> &args);
  int Add(const std::vector<std::string> &args);

  toml::table ReadExisting() const;
  int WriteTable(toml::table &tbl) const;
};

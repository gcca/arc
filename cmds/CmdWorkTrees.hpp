#pragma once

#include <string>
#include <vector>

class CmdWorkTrees {
public:
  int Run(const std::vector<std::string> &args);

private:
  int List(const std::string &pattern);
  int Prune(const std::string &pattern);
  int Cp(const std::string &pattern, const std::string &fileTemplate);
  int Restore(const std::string &pattern);
  void Usage();
};

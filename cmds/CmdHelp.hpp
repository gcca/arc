#pragma once

#include <string>
#include <vector>

class CmdHelp {
public:
  int Run(const std::vector<std::string> &args = {});

private:
  void Usage();
};

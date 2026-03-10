#include "CmdDefault.hpp"
#include "CmdHelp.hpp"

int CmdDefault::Run(const std::vector<std::string> &) {
  return CmdHelp{}.Run();
}

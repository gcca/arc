#include "CmdHelp.hpp"

#include <iostream>

int CmdHelp::Run(const std::vector<std::string> &) {
  Usage();
  return 0;
}

void CmdHelp::Usage() {
  std::cout
      << "arc v" << ARC_VERSION
      << " - tmux session orchestrator for AI agents\n"
      << '\n'
      << "Usage:\n"
      << "  arc [-s <file>] <command> [options]\n"
      << '\n'
      << "Global options:\n"
      << "  -s, --session <file>   TOML session file (default: .arc.toml)\n"
      << '\n'
      << "Commands:\n"
      << "  start    Launch a named tmux session with N panes\n"
      << "  close    Kill a tmux session\n"
      << "  send     Send text to a pane or all panes (#N expands to pane "
         "index)\n"
      << "  session   Create or update the TOML session file (name, panes, "
         "all_panes)\n"
      << "  worktrees Manage git worktrees of the current repository\n"
      << "  help      Show this help message\n"
      << '\n'
      << "Use 'arc <command> --help' for command-specific options."
      << std::endl;
}

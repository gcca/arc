#include "cmds/ArcSession.hpp"
#include "cmds/CmdClose.hpp"
#include "cmds/CmdDefault.hpp"
#include "cmds/CmdHelp.hpp"
#include "cmds/CmdSend.hpp"
#include "cmds/CmdSession.hpp"
#include "cmds/CmdStart.hpp"
#include "cmds/CmdWorkTrees.hpp"

#include <boost/program_options.hpp>
#include <csignal>
#include <fmt/color.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>

static std::pair<ArcSession, std::vector<std::string>>
ParseGlobalArgs(std::vector<std::string> args) {
  std::string sessionFile = ".arc.toml";
  std::size_t i = 0;
  for (; i < args.size(); ++i) {
    if ((args[i] == "-s" || args[i] == "--session") && i + 1 < args.size()) {
      sessionFile = args[++i];
    } else if (args[i].rfind("--session=", 0) == 0) {
      sessionFile = args[i].substr(10);
    } else {
      break;
    }
  }
  return {ArcSession(sessionFile), {args.begin() + i, args.end()}};
}

static std::string ExtractSessionName(const std::vector<std::string> &args) {
  boost::program_options::options_description desc;
  desc.add_options()(
      "name,n",
      boost::program_options::value<std::string>()->default_value("arc-0"));

  boost::program_options::variables_map vm;
  boost::program_options::store(
      boost::program_options::command_line_parser(args)
          .options(desc)
          .allow_unregistered()
          .run(),
      vm);
  boost::program_options::notify(vm);
  return vm["name"].as<std::string>();
}

static int WaitAndClose(const std::string &name) {
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  sigprocmask(SIG_BLOCK, &mask, nullptr);

  fmt::print("{} Session {} running. Press Ctrl-C to close.\n",
             fmt::styled("[arc]",
                         fmt::fg(fmt::color::steel_blue) | fmt::emphasis::bold),
             fmt::styled(name, fmt::fg(fmt::color::cornflower_blue) |
                                   fmt::emphasis::bold));

  int sig;
  sigwait(&mask, &sig);

  fmt::print("\n{} Signal received.\n",
             fmt::styled("[arc]", fmt::fg(fmt::color::steel_blue) |
                                      fmt::emphasis::bold));

  if (std::system(("tmux has-session -t " + name + " 2>/dev/null").c_str()) !=
      0) {
    fmt::print("{} Session {} already closed.\n",
               fmt::styled("[arc]", fmt::fg(fmt::color::steel_blue) |
                                        fmt::emphasis::bold),
               fmt::styled(name, fmt::fg(fmt::color::medium_sea_green) |
                                     fmt::emphasis::bold));
    return 0;
  }

  fmt::print("{} Closing session {}...\n",
             fmt::styled("[arc]",
                         fmt::fg(fmt::color::steel_blue) | fmt::emphasis::bold),
             fmt::styled(name, fmt::fg(fmt::color::cornflower_blue) |
                                   fmt::emphasis::bold));
  return CmdClose{}.Run({"-n", name});
}

int main(int argc, char *argv[]) {
  std::vector<std::string> args(argv + 1, argv + argc);

  auto [session, rest] = ParseGlobalArgs(std::move(args));
  toml::table tbl = session.Load();

  if (!rest.empty()) {
    std::string command = rest.front();
    std::vector<std::string> cmdArgs(rest.begin() + 1, rest.end());

    if (command == "start") {
      std::map<std::string, std::string> paneCommands;
      auto arcTag = [] {
        return fmt::styled("[arc]", fmt::fg(fmt::color::steel_blue) |
                                        fmt::emphasis::bold);
      };
      if (auto *panes = tbl["panes"].as_table()) {
        for (auto &[key, val] : *panes) {
          if (auto cmd = val.value<std::string>())
            paneCommands.emplace(std::string(key.str()), *cmd);
        }
        fmt::print("{} Loaded {} pane command(s) from session file.\n",
                   arcTag(),
                   fmt::styled(std::to_string(paneCommands.size()),
                               fmt::fg(fmt::color::gold)));
      } else {
        fmt::print("{} No [panes] table found in session file.\n", arcTag());
      }

      std::string allPanesCommand;
      if (auto v = tbl["all_panes"].value<std::string>()) {
        allPanesCommand = *v;
        fmt::print("{} Loaded all_panes command from session file.\n",
                   arcTag());
      }

      bool isHelp = std::find_if(cmdArgs.begin(), cmdArgs.end(),
                                 [](const std::string &a) {
                                   return a == "--help" || a == "-h";
                                 }) != cmdArgs.end();
      int result = CmdStart(SystemExecutor{}, std::move(paneCommands),
                            std::move(allPanesCommand))
                       .Run(cmdArgs);
      if (result == 0 && !isHelp)
        return WaitAndClose(ExtractSessionName(cmdArgs));
      return result;
    }
    if (command == "close")
      return CmdClose{}.Run(cmdArgs);
    if (command == "send")
      return CmdSend{}.Run(cmdArgs);
    if (command == "session")
      return CmdSession(session.Path()).Run(cmdArgs);
    if (command == "help")
      return CmdHelp{}.Run(cmdArgs);
    if (command == "worktrees")
      return CmdWorkTrees{}.Run(cmdArgs);
  }

  return CmdDefault{}.Run(rest);
}

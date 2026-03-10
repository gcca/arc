#pragma once

#include "Executor.hpp"

#include <boost/program_options.hpp>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fmt/color.h>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

template <class E = SystemExecutor>
  requires Executor<E>
class CmdStart {
public:
  explicit CmdStart(E executor = {},
                    std::map<std::string, std::string> paneCommands = {},
                    std::string allPanesCommand = {})
      : executor_(std::move(executor)), paneCommands_(std::move(paneCommands)),
        allPanesCommand_(std::move(allPanesCommand)) {}

  int Run(const std::vector<std::string> &args) {
    fmt::print("{} Parsing start options...\n",
               fmt::styled("[arc]", fmt::emphasis::faint));
    auto desc = BuildOptions();
    auto vm = ParseOptions(args, desc);

    if (vm.count("help")) {
      std::cout << "Usage: arc start [-p <n>] [--shell <shell>] [-n <name>]\n\n"
                << desc << std::endl;
      return 0;
    }

    std::filesystem::path shell = vm["shell"].template as<std::string>();
    int panes = vm["panes"].template as<int>();
    std::string name = vm["name"].template as<std::string>();

    fmt::print("{} Session: {}  Shell: {}  Panes: {}\n", ArcTag(),
               fmt::styled(name, fmt::fg(fmt::color::cornflower_blue) |
                                     fmt::emphasis::bold),
               fmt::styled(shell.string(), fmt::emphasis::italic),
               fmt::styled(std::to_string(panes), fmt::fg(fmt::color::gold)));

    int result = Launch(panes, name, shell);
    if (result != 0)
      return result;

    return InitPanes(name, panes);
  }

private:
  E executor_;
  std::map<std::string, std::string> paneCommands_;
  std::string allPanesCommand_;

  static auto ArcTag() {
    return fmt::styled("[arc]",
                       fmt::fg(fmt::color::steel_blue) | fmt::emphasis::bold);
  }

  boost::program_options::options_description BuildOptions() {
    boost::program_options::options_description desc("Start options");
    desc.add_options()("help,h", "produce help message")(
        "panes,p", boost::program_options::value<int>()->default_value(2),
        "number of panes")(
        "shell,s",
        boost::program_options::value<std::string>()->default_value("fish -il"),
        "shell name or path")(
        "name,n",
        boost::program_options::value<std::string>()->default_value("arc-0"),
        "tmux session name");
    return desc;
  }

  boost::program_options::variables_map
  ParseOptions(const std::vector<std::string> &args,
               const boost::program_options::options_description &desc) {
    fmt::print("{} Parsing command line arguments...\n",
               fmt::styled("[arc]", fmt::emphasis::faint));
    boost::program_options::variables_map vm;
    boost::program_options::store(
        boost::program_options::command_line_parser(args).options(desc).run(),
        vm);
    boost::program_options::notify(vm);
    fmt::print("{} Arguments parsed.\n",
               fmt::styled("[arc]", fmt::emphasis::faint));
    return vm;
  }

  std::string BuildSplits(int panes, const std::filesystem::path &shell,
                          const std::string &target = "") {
    std::string splits;
    std::string tFlag = target.empty() ? "" : " -t " + target;
    for (int i = 1; i < panes; ++i)
      splits += " \\; split-window" + tFlag + " " + shell.string();
    if (panes > 1)
      splits += " \\; select-layout" + tFlag + " tiled";
    return splits;
  }

  int Launch(int panes, const std::string &name,
             const std::filesystem::path &shell) {
    if (panes < 1) {
      fmt::print(stderr, "{} Error: panes must be at least 1.\n",
                 fmt::styled("[arc]", fmt::fg(fmt::color::tomato) |
                                          fmt::emphasis::bold));
      return 1;
    }

    fmt::print("{} Checking session {}...\n", ArcTag(),
               fmt::styled(name, fmt::fg(fmt::color::cornflower_blue) |
                                     fmt::emphasis::bold));
    if (executor_("tmux has-session -t " + name + " 2>/dev/null") == 0) {
      fmt::print(stderr, "{} Error: session '{}' already exists.\n",
                 fmt::styled("[arc]",
                             fmt::fg(fmt::color::tomato) | fmt::emphasis::bold),
                 fmt::styled(name, fmt::fg(fmt::color::cornflower_blue) |
                                       fmt::emphasis::bold));
      return 1;
    }

    bool insideTmux = std::getenv("TMUX") != nullptr;
    std::string cmd;
    if (insideTmux) {
      fmt::print("{} Inside tmux — launching in existing server.\n", ArcTag());
      cmd = "tmux new-session -d -s " + name + " " + shell.string();
      cmd += BuildSplits(panes, shell, name);
    } else {
      fmt::print("{} Not inside tmux — launching with kitty.\n", ArcTag());
      cmd = "kitty tmux new-session -s " + name + " " + shell.string();
      cmd += BuildSplits(panes, shell);
      cmd += " >/dev/null 2>&1 &";
    }

    fmt::print("{} Launching {} ({} panes)...\n", ArcTag(),
               fmt::styled(name, fmt::fg(fmt::color::cornflower_blue) |
                                     fmt::emphasis::bold),
               fmt::styled(std::to_string(panes), fmt::fg(fmt::color::gold)));
    for (int i = 0; i < panes; ++i)
      fmt::print("{}   pane {}  {}\n", ArcTag(),
                 fmt::styled(std::to_string(i), fmt::fg(fmt::color::gold)),
                 fmt::styled(shell.string(), fmt::emphasis::italic));
    fmt::print("{}   cmd  {}\n", ArcTag(),
               fmt::styled(cmd, fmt::fg(fmt::color::dim_gray) |
                                    fmt::emphasis::italic));

    int result = executor_(cmd);

    if (result != 0) {
      fmt::print(
          stderr, "{} Launch failed — exit code {}.\n",
          fmt::styled("[arc]",
                      fmt::fg(fmt::color::tomato) | fmt::emphasis::bold),
          fmt::styled(std::to_string(result), fmt::fg(fmt::color::tomato)));
    } else {
      fmt::print("{} Session {} launched.\n", ArcTag(),
                 fmt::styled(name, fmt::fg(fmt::color::medium_sea_green) |
                                       fmt::emphasis::bold));
    }

    return result;
  }

  int WaitForSession(const std::string &name) {
    fmt::print("{} Waiting for session {}...\n", ArcTag(),
               fmt::styled(name, fmt::fg(fmt::color::cornflower_blue) |
                                     fmt::emphasis::bold));
    for (int i = 0; i < 20; ++i) {
      if (executor_("tmux has-session -t " + name + " 2>/dev/null") == 0) {
        fmt::print("{} Session ready.\n", ArcTag());
        return 0;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
    fmt::print(
        stderr, "{} Warning: session {} not ready after waiting.\n",
        fmt::styled("[arc]", fmt::fg(fmt::color::orange) | fmt::emphasis::bold),
        fmt::styled(name, fmt::fg(fmt::color::cornflower_blue) |
                              fmt::emphasis::bold));
    return 1;
  }

  static std::string ReplaceAll(std::string text, const std::string &from,
                                const std::string &to) {
    std::size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::string::npos) {
      text.replace(pos, from.size(), to);
      pos += to.size();
    }
    return text;
  }

  int InitPanes(const std::string &name, int paneCount) {
    if (paneCommands_.empty() && allPanesCommand_.empty()) {
      fmt::print("{} No pane commands configured.\n", ArcTag());
      return 0;
    }

    if (WaitForSession(name) != 0)
      return 0;

    if (!paneCommands_.empty()) {
      fmt::print("{} Sending pane commands...\n", ArcTag());
      for (const auto &[index, command] : paneCommands_) {
        std::string target = name + "." + index;
        std::string cmdText =
            "tmux send-keys -t " + target + " '" + command + "'";
        std::string cmdExec = "tmux send-keys -t " + target + " C-m";
        fmt::print("{}   pane {}  {}\n", ArcTag(),
                   fmt::styled(index, fmt::fg(fmt::color::gold)),
                   fmt::styled(command, fmt::fg(fmt::color::pale_green)));
        int result = executor_(cmdText);
        if (result != 0) {
          fmt::print(
              stderr,
              "{} Warning: failed to send text to pane {}, exit code "
              "{}.\n",
              fmt::styled("[arc]",
                          fmt::fg(fmt::color::orange) | fmt::emphasis::bold),
              fmt::styled(index, fmt::fg(fmt::color::gold)),
              fmt::styled(std::to_string(result), fmt::fg(fmt::color::orange)));
          continue;
        }
        result = executor_(cmdExec);
        if (result != 0)
          fmt::print(
              stderr,
              "{} Warning: failed to send C-m to pane {}, exit code "
              "{}.\n",
              fmt::styled("[arc]",
                          fmt::fg(fmt::color::orange) | fmt::emphasis::bold),
              fmt::styled(index, fmt::fg(fmt::color::gold)),
              fmt::styled(std::to_string(result), fmt::fg(fmt::color::orange)));
      }
      fmt::print("{} Pane commands sent.\n", ArcTag());
    }

    if (!allPanesCommand_.empty())
      SendAllPanes(name, allPanesCommand_, paneCount, 1.0);

    return 0;
  }

  int SendAllPanes(const std::string &name, const std::string &command,
                   int paneCount, double delay) {
    fmt::print(
        "{} Broadcasting all_panes to {} panes of {}...\n", ArcTag(),
        fmt::styled(std::to_string(paneCount), fmt::fg(fmt::color::gold)),
        fmt::styled(name, fmt::fg(fmt::color::cornflower_blue) |
                              fmt::emphasis::bold));
    for (int i = 0; i < paneCount; ++i) {
      std::string resolved = ReplaceAll(command, "#N", std::to_string(i));
      std::string target = name + "." + std::to_string(i);
      fmt::print("{}   pane {}  {}\n", ArcTag(),
                 fmt::styled(std::to_string(i), fmt::fg(fmt::color::gold)),
                 fmt::styled(resolved, fmt::fg(fmt::color::gray)));
      std::string cmdText =
          "tmux send-keys -t " + target + " '" + resolved + "'";
      std::string cmdExec = "tmux send-keys -t " + target + " C-m";
      int result = executor_(cmdText);
      if (result != 0) {
        fmt::print(
            stderr,
            "{} Warning: failed to send text to pane {}, exit code {}.\n",
            fmt::styled("[arc]",
                        fmt::fg(fmt::color::orange) | fmt::emphasis::bold),
            fmt::styled(std::to_string(i), fmt::fg(fmt::color::gold)),
            fmt::styled(std::to_string(result), fmt::fg(fmt::color::orange)));
        continue;
      }
      result = executor_(cmdExec);
      if (result != 0)
        fmt::print(
            stderr,
            "{} Warning: failed to send C-m to pane {}, exit code {}.\n",
            fmt::styled("[arc]",
                        fmt::fg(fmt::color::orange) | fmt::emphasis::bold),
            fmt::styled(std::to_string(i), fmt::fg(fmt::color::gold)),
            fmt::styled(std::to_string(result), fmt::fg(fmt::color::orange)));
      if (delay > 0.0 && i + 1 < paneCount) {
        std::ostringstream oss;
        oss << delay;
        executor_("sleep " + oss.str());
      }
    }
    fmt::print("{} All panes command sent.\n", ArcTag());
    return 0;
  }
};

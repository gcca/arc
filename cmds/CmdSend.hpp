#pragma once

#include "Executor.hpp"

#include <boost/program_options.hpp>
#include <chrono>
#include <fmt/color.h>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

template <class E = SystemExecutor>
  requires Executor<E>
class CmdSend {
public:
  explicit CmdSend(E executor = {}) : executor_(std::move(executor)) {}

  int Run(const std::vector<std::string> &args) {
    fmt::print("{} Parsing send options...\n",
               fmt::styled("[arc]", fmt::emphasis::faint));
    auto desc = BuildOptions();
    auto pos = BuildPositional();
    auto vm = ParseOptions(args, desc, pos);

    if (vm.count("help")) {
      std::cout << "Usage: arc send [-n session] [-d seconds] <pane|all> "
                   "<text...>\n\n"
                << desc << std::endl;
      return 0;
    }

    if (!vm.count("pane")) {
      fmt::print(stderr, "{} Error: pane index or 'all' required.\n",
                 fmt::styled("[arc]", fmt::fg(fmt::color::tomato) |
                                          fmt::emphasis::bold));
      return 1;
    }

    if (!vm.count("text")) {
      fmt::print(stderr, "{} Error: text to send is required.\n",
                 fmt::styled("[arc]", fmt::fg(fmt::color::tomato) |
                                          fmt::emphasis::bold));
      return 1;
    }

    std::string name = vm["name"].template as<std::string>();
    std::string pane = vm["pane"].template as<std::string>();
    double delay = vm["delay"].template as<double>();
    std::vector<std::string> words =
        vm["text"].template as<std::vector<std::string>>();
    std::string text = std::accumulate(
        std::next(words.begin()), words.end(), words.front(),
        [](const std::string &a, const std::string &b) { return a + " " + b; });
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
      text.pop_back();

    fmt::print("{} Session: {}  Pane: {}  Delay: {}s\n{}   text  {}\n",
               ArcTag(),
               fmt::styled(name, fmt::fg(fmt::color::cornflower_blue) |
                                     fmt::emphasis::bold),
               fmt::styled(pane, fmt::fg(fmt::color::gold)),
               fmt::styled(std::to_string(delay), fmt::fg(fmt::color::gold)),
               ArcTag(), fmt::styled(text, fmt::fg(fmt::color::gray)));

    if (pane == "all")
      return SendAll(name, text, delay);

    try {
      return Send(name, std::stoi(pane), text, delay);
    } catch (const std::exception &) {
      fmt::print(stderr, "{} Error: pane must be a number or 'all'.\n",
                 fmt::styled("[arc]", fmt::fg(fmt::color::tomato) |
                                          fmt::emphasis::bold));
      return 1;
    }
  }

private:
  E executor_;

  static auto ArcTag() {
    return fmt::styled("[arc]",
                       fmt::fg(fmt::color::steel_blue) | fmt::emphasis::bold);
  }

  static std::string ReplaceAll(std::string str, const std::string &from,
                                const std::string &to) {
    std::size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
      str.replace(pos, from.size(), to);
      pos += to.size();
    }
    return str;
  }

  static std::string FormatDelay(double delay) {
    std::ostringstream oss;
    oss << delay;
    return oss.str();
  }

  boost::program_options::options_description BuildOptions() {
    boost::program_options::options_description desc("Send options");
    desc.add_options()("help,h", "produce help message")(
        "name,n",
        boost::program_options::value<std::string>()->default_value("arc-0"),
        "tmux session name")(
        "delay,d", boost::program_options::value<double>()->default_value(0.0),
        "seconds to wait before sending (default: 0)")(
        "pane", boost::program_options::value<std::string>(),
        "pane index or 'all'")(
        "text",
        boost::program_options::value<std::vector<std::string>>()->multitoken(),
        "text to send; #N is replaced with the pane index at send time");
    return desc;
  }

  boost::program_options::positional_options_description BuildPositional() {
    boost::program_options::positional_options_description pos;
    pos.add("pane", 1);
    pos.add("text", -1);
    return pos;
  }

  boost::program_options::variables_map ParseOptions(
      const std::vector<std::string> &args,
      const boost::program_options::options_description &desc,
      const boost::program_options::positional_options_description &pos) {
    fmt::print("{} Parsing command line arguments...\n",
               fmt::styled("[arc]", fmt::emphasis::faint));
    boost::program_options::variables_map vm;
    boost::program_options::store(
        boost::program_options::command_line_parser(args)
            .options(desc)
            .positional(pos)
            .run(),
        vm);
    boost::program_options::notify(vm);
    fmt::print("{} Arguments parsed.\n",
               fmt::styled("[arc]", fmt::emphasis::faint));
    return vm;
  }

  int Send(const std::string &name, int pane, const std::string &text,
           double delay) {
    std::string resolved = ReplaceAll(text, "#N", std::to_string(pane));
    std::string target = name + "." + std::to_string(pane);
    std::string cmdText = "tmux send-keys -t " + target + " '" + resolved + "'";
    std::string cmdExec = "tmux send-keys -t " + target + " C-m";

    fmt::print("{} Sending to {}...\n{}   cmd  {}\n{}   exec {}\n", ArcTag(),
               fmt::styled(target, fmt::fg(fmt::color::cornflower_blue) |
                                       fmt::emphasis::bold),
               ArcTag(),
               fmt::styled(cmdText, fmt::fg(fmt::color::dim_gray) |
                                        fmt::emphasis::italic),
               ArcTag(),
               fmt::styled(cmdExec, fmt::fg(fmt::color::dim_gray) |
                                        fmt::emphasis::italic));

    if (delay > 0.0) {
      fmt::print("{} Sleeping {}s...\n", ArcTag(),
                 fmt::styled(FormatDelay(delay), fmt::fg(fmt::color::gold)));
      std::this_thread::sleep_for(std::chrono::duration<double>(delay));
    }

    int result = executor_(cmdText);
    if (result != 0) {
      fmt::print(
          stderr, "{} Failed to send text to pane {}, exit code {}.\n",
          fmt::styled("[arc]",
                      fmt::fg(fmt::color::tomato) | fmt::emphasis::bold),
          fmt::styled(std::to_string(pane), fmt::fg(fmt::color::gold)),
          fmt::styled(std::to_string(result), fmt::fg(fmt::color::tomato)));
      return result;
    }
    result = executor_(cmdExec);
    if (result != 0) {
      fmt::print(
          stderr, "{} Failed to send C-m to pane {}, exit code {}.\n",
          fmt::styled("[arc]",
                      fmt::fg(fmt::color::tomato) | fmt::emphasis::bold),
          fmt::styled(std::to_string(pane), fmt::fg(fmt::color::gold)),
          fmt::styled(std::to_string(result), fmt::fg(fmt::color::tomato)));
    } else {
      fmt::print("{} Sent to pane {}.\n", ArcTag(),
                 fmt::styled(std::to_string(pane),
                             fmt::fg(fmt::color::medium_sea_green) |
                                 fmt::emphasis::bold));
    }

    return result;
  }

  int SendAll(const std::string &name, const std::string &text, double delay) {
    std::string resolved = ReplaceAll(text, "#N", "'$p'");
    std::string sleepPart =
        delay > 0.0 ? " sleep " + FormatDelay(delay) + ";" : "";
    std::string cmd = "for p in $(tmux list-panes -t " + name +
                      " -F '#{pane_index}'); do"
                      " tmux send-keys -t " +
                      name + ".$p '" + resolved +
                      "';"
                      " tmux send-keys -t " +
                      name + ".$p C-m;" + sleepPart + " done";

    fmt::print("{} Broadcasting to all panes of {}...\n{}   cmd  {}\n",
               ArcTag(),
               fmt::styled(name, fmt::fg(fmt::color::cornflower_blue) |
                                     fmt::emphasis::bold),
               ArcTag(),
               fmt::styled(cmd, fmt::fg(fmt::color::dim_gray) |
                                    fmt::emphasis::italic));

    int result = executor_(cmd);
    if (result != 0)
      fmt::print(
          stderr, "{} Broadcast failed — exit code {}.\n",
          fmt::styled("[arc]",
                      fmt::fg(fmt::color::tomato) | fmt::emphasis::bold),
          fmt::styled(std::to_string(result), fmt::fg(fmt::color::tomato)));
    else
      fmt::print("{} Broadcast to {} sent.\n", ArcTag(),
                 fmt::styled(name, fmt::fg(fmt::color::medium_sea_green) |
                                       fmt::emphasis::bold));

    return result;
  }
};

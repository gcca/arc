#pragma once

#include "Executor.hpp"

#include <boost/program_options.hpp>
#include <fmt/color.h>
#include <iostream>
#include <string>
#include <vector>

template <class E = SystemExecutor>
  requires Executor<E>
class CmdClose {
public:
  explicit CmdClose(E executor = {}) : executor_(std::move(executor)) {}

  int Run(const std::vector<std::string> &args) {
    fmt::print("{} Parsing close options...\n",
               fmt::styled("[arc]", fmt::emphasis::faint));
    auto desc = BuildOptions();
    auto vm = ParseOptions(args, desc);

    if (vm.count("help")) {
      std::cout << "Usage: arc close [-n <name>]\n\n" << desc << std::endl;
      return 0;
    }

    std::string name = vm["name"].template as<std::string>();
    fmt::print("{} Session: {}\n", ArcTag(),
               fmt::styled(name, fmt::fg(fmt::color::cornflower_blue) |
                                     fmt::emphasis::bold));

    return Close(name);
  }

private:
  E executor_;

  static auto ArcTag() {
    return fmt::styled("[arc]",
                       fmt::fg(fmt::color::steel_blue) | fmt::emphasis::bold);
  }

  boost::program_options::options_description BuildOptions() {
    boost::program_options::options_description desc("Close options");
    desc.add_options()("help,h", "produce help message")(
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

  int Close(const std::string &name) {
    std::string cmd = "tmux kill-session -t " + name;

    fmt::print("{} Closing session {}...\n{}   cmd  {}\n", ArcTag(),
               fmt::styled(name, fmt::fg(fmt::color::cornflower_blue) |
                                     fmt::emphasis::bold),
               ArcTag(),
               fmt::styled(cmd, fmt::fg(fmt::color::dim_gray) |
                                    fmt::emphasis::italic));

    int result = executor_(cmd);

    if (result != 0) {
      fmt::print(
          stderr, "{} Failed to close session '{}' — exit code {}.\n",
          fmt::styled("[arc]",
                      fmt::fg(fmt::color::tomato) | fmt::emphasis::bold),
          fmt::styled(name, fmt::fg(fmt::color::cornflower_blue) |
                                fmt::emphasis::bold),
          fmt::styled(std::to_string(result), fmt::fg(fmt::color::tomato)));
    } else {
      fmt::print("{} Session {} closed.\n", ArcTag(),
                 fmt::styled(name, fmt::fg(fmt::color::medium_sea_green) |
                                       fmt::emphasis::bold));
    }

    return result;
  }
};

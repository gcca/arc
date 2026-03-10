#include "CmdSession.hpp"

#include <boost/program_options.hpp>
#include <fmt/color.h>
#include <fstream>
#include <iostream>

CmdSession::CmdSession(std::filesystem::path file) : file_(std::move(file)) {}

int CmdSession::Run(const std::vector<std::string> &args) {
  if (!args.empty() && (args.front() == "--help" || args.front() == "-h")) {
    std::cout
        << "Usage: arc session [subcommand] [options]\n"
        << '\n'
        << "Subcommands:\n"
        << "  (none)   Initialize or refresh the session file with the "
           "current version\n"
        << "  set      Set scalar fields (name, etc.)\n"
        << "  add      Add entries to collections (panes, etc.)\n"
        << '\n'
        << "Options for 'set':\n"
        << "  -n, --name <name>              tmux session name\n"
        << "      --all_panes <command>      command sent to all panes on "
           "start\n"
        << '\n'
        << "Options for 'add':\n"
        << "  -p, --pane <index>:<command>  add a pane entry (repeatable)\n"
        << '\n'
        << "  -h, --help                    produce help message" << std::endl;
    return 0;
  }
  if (!args.empty() && args.front() == "set")
    return Set({args.begin() + 1, args.end()});
  if (!args.empty() && args.front() == "add")
    return Add({args.begin() + 1, args.end()});
  return Create();
}

int CmdSession::Create() {
  auto arc = [] {
    return fmt::styled("[arc]",
                       fmt::fg(fmt::color::steel_blue) | fmt::emphasis::bold);
  };
  fmt::print("{} Initializing session file: {}\n", arc(),
             fmt::styled(file_.string(), fmt::emphasis::italic));
  toml::table tbl = ReadExisting();
  tbl.insert_or_assign("version", ARC_VERSION);
  return WriteTable(tbl);
}

int CmdSession::Set(const std::vector<std::string> &args) {
  auto arc = [] {
    return fmt::styled("[arc]",
                       fmt::fg(fmt::color::steel_blue) | fmt::emphasis::bold);
  };
  fmt::print("{} Setting session file fields: {}\n", arc(),
             fmt::styled(file_.string(), fmt::emphasis::italic));

  boost::program_options::options_description desc("Session set options");
  desc.add_options()("help,h", "produce help message")(
      "name,n", boost::program_options::value<std::string>(),
      "session name")("all_panes", boost::program_options::value<std::string>(),
                      "command sent to all panes on start");

  boost::program_options::variables_map vm;
  boost::program_options::store(
      boost::program_options::command_line_parser(args).options(desc).run(),
      vm);
  boost::program_options::notify(vm);

  if (vm.count("help")) {
    std::cout << "Usage: arc session set [options]\n\n" << desc << std::endl;
    return 0;
  }

  toml::table tbl = ReadExisting();
  tbl.insert_or_assign("version", ARC_VERSION);

  if (vm.count("name")) {
    std::string name = vm["name"].as<std::string>();
    tbl.insert_or_assign("name", name);
    fmt::print("{}   name       = {}\n", arc(),
               fmt::styled(name, fmt::fg(fmt::color::pale_green)));
  }

  if (vm.count("all_panes")) {
    std::string allPanes = vm["all_panes"].as<std::string>();
    tbl.insert_or_assign("all_panes", allPanes);
    fmt::print("{}   all_panes  = {}\n", arc(),
               fmt::styled(allPanes, fmt::fg(fmt::color::pale_green)));
  }

  return WriteTable(tbl);
}

int CmdSession::Add(const std::vector<std::string> &args) {
  auto arc = [] {
    return fmt::styled("[arc]",
                       fmt::fg(fmt::color::steel_blue) | fmt::emphasis::bold);
  };
  fmt::print("{} Adding to session file: {}\n", arc(),
             fmt::styled(file_.string(), fmt::emphasis::italic));

  boost::program_options::options_description desc("Session add options");
  desc.add_options()("help,h", "produce help message")(
      "pane,p",
      boost::program_options::value<std::vector<std::string>>()->composing(),
      "pane entry as <index>:<command>");

  boost::program_options::variables_map vm;
  boost::program_options::store(
      boost::program_options::command_line_parser(args).options(desc).run(),
      vm);
  boost::program_options::notify(vm);

  if (vm.count("help")) {
    std::cout << "Usage: arc session add [options]\n\n" << desc << std::endl;
    return 0;
  }

  toml::table tbl = ReadExisting();
  tbl.insert_or_assign("version", ARC_VERSION);

  if (vm.count("pane")) {
    if (!tbl.contains("panes") || !tbl["panes"].is_table())
      tbl.insert_or_assign("panes", toml::table{});

    auto &panes = *tbl["panes"].as_table();
    for (const std::string &entry : vm["pane"].as<std::vector<std::string>>()) {
      auto sep = entry.find(':');
      if (sep == std::string::npos) {
        fmt::print(stderr, "{} Error: pane entry '{}' missing ':' separator.\n",
                   fmt::styled("[arc]", fmt::fg(fmt::color::tomato) |
                                            fmt::emphasis::bold),
                   fmt::styled(entry, fmt::fg(fmt::color::gold)));
        return 1;
      }
      std::string index = entry.substr(0, sep);
      std::string command = entry.substr(sep + 1);
      panes.insert_or_assign(index, command);
      fmt::print("{}   panes.{}  = {}\n", arc(),
                 fmt::styled(index, fmt::fg(fmt::color::gold)),
                 fmt::styled(command, fmt::fg(fmt::color::pale_green)));
    }
  }

  return WriteTable(tbl);
}

toml::table CmdSession::ReadExisting() const {
  auto arc = [] {
    return fmt::styled("[arc]",
                       fmt::fg(fmt::color::steel_blue) | fmt::emphasis::bold);
  };
  if (!std::filesystem::exists(file_)) {
    fmt::print("{} No existing session file, creating new.\n", arc());
    return {};
  }
  fmt::print("{} Reading existing session file.\n", arc());
  try {
    return toml::parse_file(file_.string());
  } catch (const std::exception &e) {
    fmt::print(
        stderr, "{} Warning: could not parse existing file, overwriting: {}\n",
        fmt::styled("[arc]", fmt::fg(fmt::color::orange) | fmt::emphasis::bold),
        e.what());
    return {};
  }
}

int CmdSession::WriteTable(toml::table &tbl) const {
  auto arc = [] {
    return fmt::styled("[arc]",
                       fmt::fg(fmt::color::steel_blue) | fmt::emphasis::bold);
  };
  std::ofstream out(file_);
  if (!out) {
    fmt::print(
        stderr, "{} Error: could not open '{}' for writing.\n",
        fmt::styled("[arc]", fmt::fg(fmt::color::tomato) | fmt::emphasis::bold),
        fmt::styled(file_.string(), fmt::emphasis::italic));
    return 1;
  }
  out << tbl << '\n';
  fmt::print("{} Session file written: {}\n", arc(),
             fmt::styled(file_.string(), fmt::fg(fmt::color::medium_sea_green) |
                                             fmt::emphasis::bold));
  return 0;
}

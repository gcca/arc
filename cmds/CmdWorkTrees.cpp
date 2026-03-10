#include "CmdWorkTrees.hpp"

#include <boost/program_options.hpp>
#include <filesystem>
#include <fmt/color.h>
#include <git2.h>
#include <iostream>
#include <regex>

int CmdWorkTrees::Run(const std::vector<std::string> &args) {
  boost::program_options::options_description desc("worktrees options");
  desc.add_options()("help,h", "show this help")(
      "regex,r",
      boost::program_options::value<std::string>()->default_value("wt-\\d"),
      "regex pattern to filter worktrees by name (default: wt-\\d)")(
      "subcommand", boost::program_options::value<std::string>(),
      "subcommand to execute")("file",
                               boost::program_options::value<std::string>(),
                               "file template for cp (#N replaced with index)");

  boost::program_options::positional_options_description pos;
  pos.add("subcommand", 1);
  pos.add("file", 1);

  boost::program_options::variables_map vm;
  boost::program_options::store(
      boost::program_options::command_line_parser(args)
          .options(desc)
          .positional(pos)
          .allow_unregistered()
          .run(),
      vm);
  boost::program_options::notify(vm);

  if (vm.count("help") || !vm.count("subcommand")) {
    Usage();
    return 0;
  }

  std::string subcommand = vm["subcommand"].as<std::string>();
  std::string pattern = vm["regex"].as<std::string>();

  if (subcommand == "list")
    return List(pattern);

  if (subcommand == "prune")
    return Prune(pattern);

  if (subcommand == "restore")
    return Restore(pattern);

  if (subcommand == "cp") {
    if (!vm.count("file")) {
      fmt::print(stderr, "{} Error: file template required.\n",
                 fmt::styled("[arc]", fmt::fg(fmt::color::tomato) |
                                          fmt::emphasis::bold));
      return 1;
    }
    return Cp(pattern, vm["file"].as<std::string>());
  }

  std::cerr << "[arc] worktrees: unknown subcommand '" << subcommand << "'"
            << std::endl;
  return 1;
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

int CmdWorkTrees::List(const std::string &pattern) {
  fmt::print("{} worktrees list --regex {}\n",
             fmt::styled("[arc]",
                         fmt::fg(fmt::color::steel_blue) | fmt::emphasis::bold),
             fmt::styled(pattern, fmt::emphasis::italic));

  std::regex re;
  try {
    re = std::regex(pattern);
  } catch (const std::regex_error &e) {
    std::cerr << "[arc] invalid regex pattern '" << pattern << "': " << e.what()
              << std::endl;
    return 1;
  }

  git_libgit2_init();

  git_repository *repo = nullptr;
  int rc = git_repository_open_ext(&repo, ".", 0, nullptr);
  if (rc != 0) {
    const git_error *err = git_error_last();
    std::cerr << "[arc] failed to open repository: "
              << (err ? err->message : "unknown error") << std::endl;
    git_libgit2_shutdown();
    return 1;
  }

  git_strarray worktrees = {};
  rc = git_worktree_list(&worktrees, repo);
  if (rc != 0) {
    const git_error *err = git_error_last();
    std::cerr << "[arc] failed to list worktrees: "
              << (err ? err->message : "unknown error") << std::endl;
    git_repository_free(repo);
    git_libgit2_shutdown();
    return 1;
  }

  for (std::size_t i = 0; i < worktrees.count; ++i) {
    std::string wtName = worktrees.strings[i];
    if (!std::regex_search(wtName, re))
      continue;

    git_worktree *wt = nullptr;
    if (git_worktree_lookup(&wt, repo, worktrees.strings[i]) != 0)
      continue;

    std::string branchName;
    git_repository *wtRepo = nullptr;
    if (git_repository_open_from_worktree(&wtRepo, wt) == 0) {
      git_reference *head = nullptr;
      if (git_repository_head(&head, wtRepo) == 0) {
        branchName = git_reference_shorthand(head);
        git_reference_free(head);
      }
      git_repository_free(wtRepo);
    }

    fmt::print("{} worktree: {}  branch: {}\n",
               fmt::styled("[arc]", fmt::fg(fmt::color::steel_blue) |
                                        fmt::emphasis::bold),
               fmt::styled(wtName, fmt::fg(fmt::color::medium_sea_green) |
                                       fmt::emphasis::bold),
               fmt::styled(branchName, fmt::fg(fmt::color::gold)));
    git_worktree_free(wt);
  }

  git_strarray_dispose(&worktrees);
  git_repository_free(repo);
  git_libgit2_shutdown();

  return 0;
}

int CmdWorkTrees::Prune(const std::string &pattern) {
  fmt::print("{} worktrees prune --regex {}\n",
             fmt::styled("[arc]",
                         fmt::fg(fmt::color::steel_blue) | fmt::emphasis::bold),
             fmt::styled(pattern, fmt::emphasis::italic));

  std::regex re;
  try {
    re = std::regex(pattern);
  } catch (const std::regex_error &e) {
    std::cerr << "[arc] invalid regex pattern '" << pattern << "': " << e.what()
              << std::endl;
    return 1;
  }

  git_libgit2_init();

  git_repository *repo = nullptr;
  int rc = git_repository_open_ext(&repo, ".", 0, nullptr);
  if (rc != 0) {
    const git_error *err = git_error_last();
    std::cerr << "[arc] failed to open repository: "
              << (err ? err->message : "unknown error") << std::endl;
    git_libgit2_shutdown();
    return 1;
  }

  git_strarray worktrees = {};
  rc = git_worktree_list(&worktrees, repo);
  if (rc != 0) {
    const git_error *err = git_error_last();
    std::cerr << "[arc] failed to list worktrees: "
              << (err ? err->message : "unknown error") << std::endl;
    git_repository_free(repo);
    git_libgit2_shutdown();
    return 1;
  }

  for (std::size_t i = 0; i < worktrees.count; ++i) {
    std::string wtName = worktrees.strings[i];
    if (!std::regex_search(wtName, re))
      continue;

    git_worktree *wt = nullptr;
    if (git_worktree_lookup(&wt, repo, worktrees.strings[i]) != 0)
      continue;

    std::string branchName;
    git_repository *wtRepo = nullptr;
    if (git_repository_open_from_worktree(&wtRepo, wt) == 0) {
      git_reference *head = nullptr;
      if (git_repository_head(&head, wtRepo) == 0) {
        branchName = git_reference_shorthand(head);
        git_reference_free(head);
      }
      git_repository_free(wtRepo);
    }

    fmt::print("{} prune: found  worktree: {}  branch: {}\n",
               fmt::styled("[arc]", fmt::fg(fmt::color::steel_blue) |
                                        fmt::emphasis::bold),
               fmt::styled(wtName, fmt::fg(fmt::color::medium_sea_green) |
                                       fmt::emphasis::bold),
               fmt::styled(branchName.empty() ? "(none)" : branchName,
                           fmt::fg(fmt::color::gold)));

    git_worktree_prune_options pruneOpts = GIT_WORKTREE_PRUNE_OPTIONS_INIT;
    pruneOpts.flags =
        GIT_WORKTREE_PRUNE_VALID | GIT_WORKTREE_PRUNE_WORKING_TREE;
    if (git_worktree_prune(wt, &pruneOpts) == 0) {
      fmt::print("{} prune: removed worktree {}\n",
                 fmt::styled("[arc]", fmt::fg(fmt::color::steel_blue) |
                                          fmt::emphasis::bold),
                 fmt::styled(wtName, fmt::fg(fmt::color::medium_sea_green) |
                                         fmt::emphasis::bold));
    } else {
      fmt::print("{} prune: skip   worktree {} (remove failed)\n",
                 fmt::styled("[arc]", fmt::fg(fmt::color::steel_blue) |
                                          fmt::emphasis::bold),
                 fmt::styled(wtName, fmt::emphasis::italic));
    }
    git_worktree_free(wt);

    if (branchName.empty()) {
      fmt::print("{} prune: skip   branch (not resolved)\n",
                 fmt::styled("[arc]", fmt::fg(fmt::color::steel_blue) |
                                          fmt::emphasis::bold));
      continue;
    }

    git_reference *branchRef = nullptr;
    if (git_branch_lookup(&branchRef, repo, branchName.c_str(),
                          GIT_BRANCH_LOCAL) != 0) {
      fmt::print("{} prune: skip   branch {} (not found)\n",
                 fmt::styled("[arc]", fmt::fg(fmt::color::steel_blue) |
                                          fmt::emphasis::bold),
                 fmt::styled(branchName, fmt::emphasis::italic));
      continue;
    }

    if (git_branch_delete(branchRef) == 0) {
      fmt::print("{} prune: removed branch {}\n",
                 fmt::styled("[arc]", fmt::fg(fmt::color::steel_blue) |
                                          fmt::emphasis::bold),
                 fmt::styled(branchName, fmt::fg(fmt::color::gold)));
    } else {
      fmt::print("{} prune: skip   branch {} (delete failed)\n",
                 fmt::styled("[arc]", fmt::fg(fmt::color::steel_blue) |
                                          fmt::emphasis::bold),
                 fmt::styled(branchName, fmt::emphasis::italic));
    }
    git_reference_free(branchRef);
  }

  git_strarray_dispose(&worktrees);
  git_repository_free(repo);
  git_libgit2_shutdown();

  return 0;
}

int CmdWorkTrees::Cp(const std::string &pattern,
                     const std::string &fileTemplate) {
  fmt::print("{} worktrees cp --regex {}\n",
             fmt::styled("[arc]",
                         fmt::fg(fmt::color::steel_blue) | fmt::emphasis::bold),
             fmt::styled(pattern, fmt::emphasis::italic));

  std::regex re;
  try {
    re = std::regex(pattern);
  } catch (const std::regex_error &e) {
    std::cerr << "[arc] invalid regex pattern '" << pattern << "': " << e.what()
              << std::endl;
    return 1;
  }

  git_libgit2_init();

  git_repository *repo = nullptr;
  int rc = git_repository_open_ext(&repo, ".", 0, nullptr);
  if (rc != 0) {
    const git_error *err = git_error_last();
    std::cerr << "[arc] failed to open repository: "
              << (err ? err->message : "unknown error") << std::endl;
    git_libgit2_shutdown();
    return 1;
  }

  git_strarray worktrees = {};
  rc = git_worktree_list(&worktrees, repo);
  if (rc != 0) {
    const git_error *err = git_error_last();
    std::cerr << "[arc] failed to list worktrees: "
              << (err ? err->message : "unknown error") << std::endl;
    git_repository_free(repo);
    git_libgit2_shutdown();
    return 1;
  }

  struct WtEntry {
    std::string name;
    std::filesystem::path workdir;
    std::string branchName;
  };
  std::vector<WtEntry> matched;

  for (std::size_t i = 0; i < worktrees.count; ++i) {
    std::string wtName = worktrees.strings[i];
    if (!std::regex_search(wtName, re))
      continue;

    git_worktree *wt = nullptr;
    if (git_worktree_lookup(&wt, repo, worktrees.strings[i]) != 0)
      continue;

    git_repository *wtRepo = nullptr;
    if (git_repository_open_from_worktree(&wtRepo, wt) == 0) {
      const char *workdir = git_repository_workdir(wtRepo);
      std::string branchName;
      git_reference *head = nullptr;
      if (git_repository_head(&head, wtRepo) == 0) {
        branchName = git_reference_shorthand(head);
        git_reference_free(head);
      }
      if (workdir)
        matched.push_back({wtName, std::filesystem::path(workdir), branchName});
      git_repository_free(wtRepo);
    }
    git_worktree_free(wt);
  }

  git_strarray_dispose(&worktrees);
  git_repository_free(repo);
  git_libgit2_shutdown();

  int wtCount = static_cast<int>(matched.size());
  int branchCount = 0;
  for (const auto &entry : matched)
    if (!entry.branchName.empty())
      ++branchCount;

  fmt::print(
      "{} cp: found {} worktree(s), {} with resolved branch\n",
      fmt::styled("[arc]",
                  fmt::fg(fmt::color::steel_blue) | fmt::emphasis::bold),
      fmt::styled(std::to_string(wtCount), fmt::fg(fmt::color::gold)),
      fmt::styled(std::to_string(branchCount), fmt::fg(fmt::color::gold)));

  if (wtCount != branchCount) {
    fmt::print(
        stderr,
        "{} cp: warning: worktree count ({}) != branch count ({}); "
        "some worktrees have no resolved branch\n",
        fmt::styled("[arc]", fmt::fg(fmt::color::orange) | fmt::emphasis::bold),
        fmt::styled(std::to_string(wtCount), fmt::fg(fmt::color::gold)),
        fmt::styled(std::to_string(branchCount), fmt::fg(fmt::color::gold)));
  }

  if (wtCount == 0) {
    fmt::print("{} cp: skip   (no matching worktrees)\n",
               fmt::styled("[arc]", fmt::fg(fmt::color::steel_blue) |
                                        fmt::emphasis::bold));
    return 0;
  }

  for (int i = 0; i < wtCount; ++i) {
    std::string suffix = "wt-" + std::to_string(i);
    std::string srcName = ReplaceAll(fileTemplate, "#N", std::to_string(i));

    const WtEntry *target = nullptr;
    for (const auto &wt : matched) {
      if (wt.name.size() >= suffix.size() &&
          wt.name.substr(wt.name.size() - suffix.size()) == suffix) {
        target = &wt;
        break;
      }
    }

    if (!target) {
      fmt::print("{} cp: skip   index {}  (no worktree ending with {})\n",
                 fmt::styled("[arc]", fmt::fg(fmt::color::steel_blue) |
                                          fmt::emphasis::bold),
                 fmt::styled(std::to_string(i), fmt::fg(fmt::color::gold)),
                 fmt::styled(suffix, fmt::emphasis::italic));
      continue;
    }

    std::filesystem::path src = std::filesystem::current_path() / srcName;
    std::filesystem::path dst = target->workdir / srcName;

    fmt::print("{} cp: index {}  {} → {}\n",
               fmt::styled("[arc]", fmt::fg(fmt::color::steel_blue) |
                                        fmt::emphasis::bold),
               fmt::styled(std::to_string(i), fmt::fg(fmt::color::gold)),
               fmt::styled(src.string(), fmt::emphasis::italic),
               fmt::styled(dst.string(), fmt::fg(fmt::color::cornflower_blue) |
                                             fmt::emphasis::bold));

    try {
      std::filesystem::copy_file(
          src, dst, std::filesystem::copy_options::overwrite_existing);
      fmt::print(
          "{} cp: copied {}\n",
          fmt::styled("[arc]",
                      fmt::fg(fmt::color::steel_blue) | fmt::emphasis::bold),
          fmt::styled(dst.string(), fmt::fg(fmt::color::medium_sea_green) |
                                        fmt::emphasis::bold));
    } catch (const std::exception &e) {
      fmt::print(stderr, "{} cp: failed {} → {}: {}\n",
                 fmt::styled("[arc]",
                             fmt::fg(fmt::color::tomato) | fmt::emphasis::bold),
                 fmt::styled(src.string(), fmt::emphasis::italic),
                 fmt::styled(dst.string(), fmt::emphasis::italic), e.what());
    }
  }

  return 0;
}

int CmdWorkTrees::Restore(const std::string &pattern) {
  fmt::print("{} worktrees restore --regex {}\n",
             fmt::styled("[arc]",
                         fmt::fg(fmt::color::steel_blue) | fmt::emphasis::bold),
             fmt::styled(pattern, fmt::emphasis::italic));

  std::regex re;
  try {
    re = std::regex(pattern);
  } catch (const std::regex_error &e) {
    std::cerr << "[arc] invalid regex pattern '" << pattern << "': " << e.what()
              << std::endl;
    return 1;
  }

  git_libgit2_init();

  git_repository *repo = nullptr;
  int rc = git_repository_open_ext(&repo, ".", 0, nullptr);
  if (rc != 0) {
    const git_error *err = git_error_last();
    std::cerr << "[arc] failed to open repository: "
              << (err ? err->message : "unknown error") << std::endl;
    git_libgit2_shutdown();
    return 1;
  }

  git_strarray worktrees = {};
  rc = git_worktree_list(&worktrees, repo);
  if (rc != 0) {
    const git_error *err = git_error_last();
    std::cerr << "[arc] failed to list worktrees: "
              << (err ? err->message : "unknown error") << std::endl;
    git_repository_free(repo);
    git_libgit2_shutdown();
    return 1;
  }

  for (std::size_t i = 0; i < worktrees.count; ++i) {
    std::string wtName = worktrees.strings[i];
    if (!std::regex_search(wtName, re))
      continue;

    git_worktree *wt = nullptr;
    if (git_worktree_lookup(&wt, repo, worktrees.strings[i]) != 0)
      continue;

    git_repository *wtRepo = nullptr;
    if (git_repository_open_from_worktree(&wtRepo, wt) != 0) {
      git_worktree_free(wt);
      continue;
    }

    std::string branchName;
    git_reference *head = nullptr;
    if (git_repository_head(&head, wtRepo) == 0) {
      branchName = git_reference_shorthand(head);
      git_reference_free(head);
    }

    fmt::print("{} restore: worktree {}  branch {}\n",
               fmt::styled("[arc]", fmt::fg(fmt::color::steel_blue) |
                                        fmt::emphasis::bold),
               fmt::styled(wtName, fmt::fg(fmt::color::medium_sea_green) |
                                       fmt::emphasis::bold),
               fmt::styled(branchName.empty() ? "(none)" : branchName,
                           fmt::fg(fmt::color::gold)));

    git_checkout_options coOpts = GIT_CHECKOUT_OPTIONS_INIT;
    coOpts.checkout_strategy = GIT_CHECKOUT_FORCE;
    if (git_checkout_head(wtRepo, &coOpts) == 0) {
      fmt::print("{} restore: checkout HEAD ok  {}\n",
                 fmt::styled("[arc]", fmt::fg(fmt::color::steel_blue) |
                                          fmt::emphasis::bold),
                 fmt::styled(wtName, fmt::fg(fmt::color::medium_sea_green) |
                                         fmt::emphasis::bold));
    } else {
      const git_error *err = git_error_last();
      fmt::print(stderr, "{} restore: checkout HEAD failed  {}: {}\n",
                 fmt::styled("[arc]",
                             fmt::fg(fmt::color::orange) | fmt::emphasis::bold),
                 fmt::styled(wtName, fmt::emphasis::italic),
                 err ? err->message : "unknown error");
    }

    const char *workdirRaw = git_repository_workdir(wtRepo);
    if (workdirRaw) {
      std::filesystem::path workdir(workdirRaw);

      git_status_options statusOpts = GIT_STATUS_OPTIONS_INIT;
      statusOpts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED;
      git_status_list *statusList = nullptr;
      if (git_status_list_new(&statusList, wtRepo, &statusOpts) == 0) {
        std::size_t count = git_status_list_entrycount(statusList);
        for (std::size_t j = 0; j < count; ++j) {
          const git_status_entry *entry = git_status_byindex(statusList, j);
          if (!(entry->status & GIT_STATUS_WT_NEW))
            continue;
          const char *path = entry->index_to_workdir
                                 ? entry->index_to_workdir->new_file.path
                                 : nullptr;
          if (!path)
            continue;
          std::filesystem::path fullPath = workdir / path;
          try {
            std::filesystem::remove_all(fullPath);
            fmt::print("{} restore: restored {}\n",
                       fmt::styled("[arc]", fmt::fg(fmt::color::steel_blue) |
                                                fmt::emphasis::bold),
                       fmt::styled(fullPath.string(), fmt::emphasis::italic));
          } catch (const std::exception &e) {
            fmt::print(stderr, "{} restore: failed to remove {}: {}\n",
                       fmt::styled("[arc]", fmt::fg(fmt::color::orange) |
                                                fmt::emphasis::bold),
                       fmt::styled(fullPath.string(), fmt::emphasis::italic),
                       e.what());
          }
        }
        git_status_list_free(statusList);
      }
    }

    git_repository_free(wtRepo);
    git_worktree_free(wt);
  }

  git_strarray_dispose(&worktrees);
  git_repository_free(repo);
  git_libgit2_shutdown();

  return 0;
}

void CmdWorkTrees::Usage() {
  std::cout << "Usage:\n"
            << "  arc worktrees [--regex/-r PATTERN] <subcommand>\n"
            << '\n'
            << "Options:\n"
            << "  -r, --regex PATTERN  Regex pattern to filter worktrees by "
               "name (default: wt-\\d)\n"
            << '\n'
            << "Subcommands:\n"
            << "  list   List worktrees matching the regex pattern\n"
            << "  prune  Remove worktrees and branches matching the regex "
               "pattern\n"
            << "  cp      Copy a file to each matching worktree\n"
            << "          arc worktrees [--regex/-r PATTERN] cp <file>\n"
            << "  restore Restore each matching worktree: checkout HEAD and "
               "remove untracked files\n"
            << '\n'
            << "Use 'arc worktrees --help' for this help message." << std::endl;
}

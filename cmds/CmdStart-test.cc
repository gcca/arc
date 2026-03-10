#include <cstdlib>
#include <gtest/gtest.h>

#include "CmdStart.hpp"

TEST(CmdStartTest, HelpShowsUsage) {
  testing::internal::CaptureStdout();
  int result = CmdStart{}.Run({"--help"});
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  EXPECT_NE(out.find("Usage: arc start"), std::string::npos);
  EXPECT_NE(out.find("Start options"), std::string::npos);
}

TEST(CmdStartTest, ShortHelpShowsUsage) {
  testing::internal::CaptureStdout();
  int result = CmdStart{}.Run({"-h"});
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
}

TEST(CmdStartTest, DefaultsUsedInCommand) {
  std::string captured;
  auto mock = [&](const std::string &cmd) -> int {
    if (cmd.find("has-session") != std::string::npos)
      return 1;
    captured = cmd;
    return 0;
  };

  testing::internal::CaptureStdout();
  int result = CmdStart(mock).Run({});
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  EXPECT_NE(captured.find("arc-0"), std::string::npos);
  EXPECT_NE(captured.find("fish"), std::string::npos);
}

TEST(CmdStartTest, CustomNameAppearsInCommand) {
  std::string captured;
  auto mock = [&](const std::string &cmd) -> int {
    if (cmd.find("has-session") != std::string::npos)
      return 1;
    captured = cmd;
    return 0;
  };

  testing::internal::CaptureStdout();
  CmdStart(mock).Run({"-n", "my-session"});
  testing::internal::GetCapturedStdout();

  EXPECT_NE(captured.find("my-session"), std::string::npos);
}

TEST(CmdStartTest, CustomShellAppearsInCommand) {
  std::string captured;
  auto mock = [&](const std::string &cmd) -> int {
    if (cmd.find("has-session") != std::string::npos)
      return 1;
    captured = cmd;
    return 0;
  };

  testing::internal::CaptureStdout();
  CmdStart(mock).Run({"-s", "zsh"});
  testing::internal::GetCapturedStdout();

  EXPECT_NE(captured.find("zsh"), std::string::npos);
}

TEST(CmdStartTest, ZeroPanesReturnsError) {
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result = CmdStart{}.Run({"-p", "0"});
  testing::internal::GetCapturedStdout();
  testing::internal::GetCapturedStderr();

  EXPECT_EQ(result, 1);
}

TEST(CmdStartTest, SessionAlreadyExistsReturnsError) {
  auto mock = [](const std::string &cmd) -> int {
    if (cmd.find("has-session") != std::string::npos)
      return 0;
    return 0;
  };

  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result = CmdStart(mock).Run({});
  testing::internal::GetCapturedStdout();
  testing::internal::GetCapturedStderr();

  EXPECT_EQ(result, 1);
}

TEST(CmdStartTest, SinglePaneLaunchesWithoutSplit) {
  std::string captured;
  auto mock = [&](const std::string &cmd) -> int {
    if (cmd.find("has-session") != std::string::npos)
      return 1;
    captured = cmd;
    return 0;
  };

  testing::internal::CaptureStdout();
  int result = CmdStart(mock).Run({"-p", "1"});
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  EXPECT_EQ(captured.find("split-window"), std::string::npos);
}

TEST(CmdStartTest, ThreePanesProducesTwoSplits) {
  std::string captured;
  auto mock = [&](const std::string &cmd) -> int {
    if (cmd.find("has-session") != std::string::npos)
      return 1;
    captured = cmd;
    return 0;
  };

  testing::internal::CaptureStdout();
  int result = CmdStart(mock).Run({"-p", "3"});
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  std::size_t first = captured.find("split-window");
  EXPECT_NE(first, std::string::npos);
  EXPECT_NE(captured.find("split-window", first + 1), std::string::npos);
}

TEST(CmdStartTest, FourPanesProducesThreeSplitsAndTiled) {
  std::string captured;
  auto mock = [&](const std::string &cmd) -> int {
    if (cmd.find("has-session") != std::string::npos)
      return 1;
    captured = cmd;
    return 0;
  };

  testing::internal::CaptureStdout();
  int result = CmdStart(mock).Run({"-p", "4"});
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  std::size_t pos = 0;
  int count = 0;
  while ((pos = captured.find("split-window", pos)) != std::string::npos) {
    ++count;
    ++pos;
  }
  EXPECT_EQ(count, 3);
  EXPECT_NE(captured.find("select-layout tiled"), std::string::npos);
}

TEST(CmdStartTest, PaneCommandSentAfterLaunch) {
  std::vector<std::string> captured;
  int has_session_calls = 0;
  auto mock = [&](const std::string &cmd) -> int {
    if (cmd.find("has-session") != std::string::npos)
      return ++has_session_calls == 1 ? 1 : 0;
    captured.push_back(cmd);
    return 0;
  };

  std::map<std::string, std::string> panes = {{"0", "claude"}};
  testing::internal::CaptureStdout();
  int result = CmdStart(mock, panes).Run({"-n", "agents"});
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  ASSERT_GE(captured.size(), 2u);
  EXPECT_NE(captured[0].find("kitty"), std::string::npos);
  bool found = false;
  for (const auto &c : captured)
    if (c.find("send-keys") != std::string::npos &&
        c.find("agents.0") != std::string::npos &&
        c.find("claude") != std::string::npos)
      found = true;
  EXPECT_TRUE(found);
}

TEST(CmdStartTest, PaneWithoutTomlEntryReceivesNoCommand) {
  std::vector<std::string> captured;
  int has_session_calls = 0;
  auto mock = [&](const std::string &cmd) -> int {
    if (cmd.find("has-session") != std::string::npos)
      return ++has_session_calls == 1 ? 1 : 0;
    captured.push_back(cmd);
    return 0;
  };

  std::map<std::string, std::string> panes = {{"0", "claude"}};
  testing::internal::CaptureStdout();
  CmdStart(mock, panes).Run({"-n", "agents", "-p", "3"});
  testing::internal::GetCapturedStdout();

  int send_keys_count = 0;
  for (const auto &c : captured)
    if (c.find("send-keys") != std::string::npos)
      ++send_keys_count;
  EXPECT_EQ(send_keys_count, 2);
}

TEST(CmdStartTest, MultiplePaneCommandsAllSent) {
  std::vector<std::string> captured;
  int has_session_calls = 0;
  auto mock = [&](const std::string &cmd) -> int {
    if (cmd.find("has-session") != std::string::npos)
      return ++has_session_calls == 1 ? 1 : 0;
    captured.push_back(cmd);
    return 0;
  };

  std::map<std::string, std::string> panes = {
      {"0", "claude"}, {"1", "opencode"}, {"2", "git log"}};
  testing::internal::CaptureStdout();
  CmdStart(mock, panes).Run({"-n", "dev", "-p", "3"});
  testing::internal::GetCapturedStdout();

  int send_keys_count = 0;
  for (const auto &c : captured)
    if (c.find("send-keys") != std::string::npos)
      ++send_keys_count;
  EXPECT_EQ(send_keys_count, 6);
}

TEST(CmdStartTest, AllPanesCommandBroadcastAfterLaunch) {
  std::vector<std::string> captured;
  int has_session_calls = 0;
  auto mock = [&](const std::string &cmd) -> int {
    if (cmd.find("has-session") != std::string::npos)
      return ++has_session_calls == 1 ? 1 : 0;
    captured.push_back(cmd);
    return 0;
  };

  testing::internal::CaptureStdout();
  int result = CmdStart(mock, {}, "reset;agent").Run({"-n", "agents"});
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  bool foundBroadcast = false;
  for (const auto &c : captured)
    if (c.find("send-keys") != std::string::npos &&
        c.find("reset;agent") != std::string::npos)
      foundBroadcast = true;
  EXPECT_TRUE(foundBroadcast);
}

TEST(CmdStartTest, AllPanesCommandUsesDelay) {
  std::vector<std::string> captured;
  int has_session_calls = 0;
  auto mock = [&](const std::string &cmd) -> int {
    if (cmd.find("has-session") != std::string::npos)
      return ++has_session_calls == 1 ? 1 : 0;
    captured.push_back(cmd);
    return 0;
  };

  testing::internal::CaptureStdout();
  CmdStart(mock, {}, "reset;agent").Run({"-n", "agents"});
  testing::internal::GetCapturedStdout();

  bool foundSleep = false;
  for (const auto &c : captured)
    if (c.find("sleep") != std::string::npos)
      foundSleep = true;
  EXPECT_TRUE(foundSleep);
}

TEST(CmdStartTest, EmptyPaneCommandsNoWait) {
  std::string captured;
  auto mock = [&](const std::string &cmd) -> int {
    if (cmd.find("has-session") != std::string::npos)
      return 1;
    captured = cmd;
    return 0;
  };

  testing::internal::CaptureStdout();
  int result = CmdStart(mock, {}).Run({"-n", "bare"});
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  EXPECT_EQ(captured.find("send-keys"), std::string::npos);
}

TEST(CmdStartTest, DirectTmuxWhenInsideTmux) {
  const char *prev = std::getenv("TMUX");
  setenv("TMUX", "/tmp/tmux-0/default,0,0", 1);

  std::string captured;
  auto mock = [&](const std::string &cmd) -> int {
    if (cmd.find("has-session") != std::string::npos)
      return 1;
    captured = cmd;
    return 0;
  };

  testing::internal::CaptureStdout();
  int result = CmdStart(mock).Run({"-n", "test-session"});
  testing::internal::GetCapturedStdout();

  if (prev)
    setenv("TMUX", prev, 1);
  else
    unsetenv("TMUX");

  EXPECT_EQ(result, 0);
  EXPECT_EQ(captured.find("kitty"), std::string::npos);
  EXPECT_NE(captured.find("new-session"), std::string::npos);
  EXPECT_NE(captured.find(" -d "), std::string::npos);
  EXPECT_NE(captured.find("-t test-session"), std::string::npos);
}

TEST(CmdStartTest, KittyFallbackWhenNotInsideTmux) {
  unsetenv("TMUX");

  std::string captured;
  auto mock = [&](const std::string &cmd) -> int {
    if (cmd.find("has-session") != std::string::npos)
      return 1;
    captured = cmd;
    return 0;
  };

  testing::internal::CaptureStdout();
  int result = CmdStart(mock).Run({"-n", "test-session"});
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  EXPECT_NE(captured.find("kitty"), std::string::npos);
}

TEST(CmdStartTest, AllPanesHashNReplacedWithPaneIndex) {
  std::vector<std::string> captured;
  int has_session_calls = 0;
  auto mock = [&](const std::string &cmd) -> int {
    if (cmd.find("has-session") != std::string::npos)
      return ++has_session_calls == 1 ? 1 : 0;
    captured.push_back(cmd);
    return 0;
  };

  testing::internal::CaptureStdout();
  CmdStart(mock, {}, "echo pane #N").Run({"-n", "agents", "-p", "2"});
  testing::internal::GetCapturedStdout();

  bool found0 = false, found1 = false;
  for (const auto &c : captured) {
    if (c.find("echo pane 0") != std::string::npos)
      found0 = true;
    if (c.find("echo pane 1") != std::string::npos)
      found1 = true;
    EXPECT_EQ(c.find("#N"), std::string::npos);
  }
  EXPECT_TRUE(found0);
  EXPECT_TRUE(found1);
}

TEST(CmdStartTest, ExecutorFailurePropagatesExitCode) {
  auto mock = [](const std::string &cmd) -> int {
    if (cmd.find("has-session") != std::string::npos)
      return 1;
    return 42;
  };

  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result = CmdStart(mock).Run({});
  testing::internal::GetCapturedStdout();
  testing::internal::GetCapturedStderr();

  EXPECT_EQ(result, 42);
}

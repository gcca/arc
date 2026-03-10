#include <gtest/gtest.h>

#include "CmdClose.hpp"

TEST(CmdCloseTest, HelpShowsUsage) {
  testing::internal::CaptureStdout();
  int result = CmdClose{}.Run({"--help"});
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  EXPECT_NE(out.find("Usage: arc close"), std::string::npos);
  EXPECT_NE(out.find("Close options"), std::string::npos);
}

TEST(CmdCloseTest, ShortHelpShowsUsage) {
  testing::internal::CaptureStdout();
  int result = CmdClose{}.Run({"-h"});
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
}

TEST(CmdCloseTest, DefaultNameUsedInCommand) {
  std::string captured;
  auto mock = [&](const std::string &cmd) -> int {
    captured = cmd;
    return 0;
  };

  testing::internal::CaptureStdout();
  int result = CmdClose(mock).Run({});
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  EXPECT_NE(captured.find("arc-0"), std::string::npos);
}

TEST(CmdCloseTest, CustomNameUsedInCommand) {
  std::string captured;
  auto mock = [&](const std::string &cmd) -> int {
    captured = cmd;
    return 0;
  };

  testing::internal::CaptureStdout();
  CmdClose(mock).Run({"-n", "my-session"});
  testing::internal::GetCapturedStdout();

  EXPECT_NE(captured.find("my-session"), std::string::npos);
}

TEST(CmdCloseTest, KillSessionCommandUsed) {
  std::string captured;
  auto mock = [&](const std::string &cmd) -> int {
    captured = cmd;
    return 0;
  };

  testing::internal::CaptureStdout();
  CmdClose(mock).Run({});
  testing::internal::GetCapturedStdout();

  EXPECT_NE(captured.find("kill-session"), std::string::npos);
}

TEST(CmdCloseTest, ExecutorFailurePropagatesExitCode) {
  auto mock = [](const std::string &) -> int { return 1; };

  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result = CmdClose(mock).Run({});
  testing::internal::GetCapturedStdout();
  testing::internal::GetCapturedStderr();

  EXPECT_EQ(result, 1);
}

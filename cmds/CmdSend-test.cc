#include <gtest/gtest.h>

#include "CmdSend.hpp"

TEST(CmdSendTest, HelpShowsUsage) {
  testing::internal::CaptureStdout();
  int result = CmdSend{}.Run({"--help"});
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  EXPECT_NE(out.find("Usage: arc send"), std::string::npos);
  EXPECT_NE(out.find("#N"), std::string::npos);
  EXPECT_NE(out.find("Send options"), std::string::npos);
}

TEST(CmdSendTest, MissingPaneReturnsError) {
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result = CmdSend{}.Run({});
  testing::internal::GetCapturedStdout();
  testing::internal::GetCapturedStderr();

  EXPECT_EQ(result, 1);
}

TEST(CmdSendTest, MissingTextReturnsError) {
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result = CmdSend{}.Run({"0"});
  testing::internal::GetCapturedStdout();
  testing::internal::GetCapturedStderr();

  EXPECT_EQ(result, 1);
}

TEST(CmdSendTest, DefaultSessionNameUsed) {
  std::vector<std::string> captured;
  auto mock = [&](const std::string &cmd) -> int {
    captured.push_back(cmd);
    return 0;
  };

  testing::internal::CaptureStdout();
  int result = CmdSend(mock).Run({"0", "hello"});
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  ASSERT_EQ(captured.size(), 2u);
  EXPECT_NE(captured[0].find("arc-0"), std::string::npos);
}

TEST(CmdSendTest, CustomSessionNameUsed) {
  std::vector<std::string> captured;
  auto mock = [&](const std::string &cmd) -> int {
    captured.push_back(cmd);
    return 0;
  };

  testing::internal::CaptureStdout();
  CmdSend(mock).Run({"-n", "my-session", "0", "hello"});
  testing::internal::GetCapturedStdout();

  ASSERT_EQ(captured.size(), 2u);
  EXPECT_NE(captured[0].find("my-session"), std::string::npos);
}

TEST(CmdSendTest, PaneIndexAppearsInTarget) {
  std::vector<std::string> captured;
  auto mock = [&](const std::string &cmd) -> int {
    captured.push_back(cmd);
    return 0;
  };

  testing::internal::CaptureStdout();
  CmdSend(mock).Run({"1", "hello"});
  testing::internal::GetCapturedStdout();

  ASSERT_EQ(captured.size(), 2u);
  EXPECT_NE(captured[0].find(".1"), std::string::npos);
}

TEST(CmdSendTest, MultiWordTextJoined) {
  std::vector<std::string> captured;
  auto mock = [&](const std::string &cmd) -> int {
    captured.push_back(cmd);
    return 0;
  };

  testing::internal::CaptureStdout();
  CmdSend(mock).Run({"0", "another", "cmds", "here"});
  testing::internal::GetCapturedStdout();

  ASSERT_EQ(captured.size(), 2u);
  EXPECT_NE(captured[0].find("another cmds here"), std::string::npos);
}

TEST(CmdSendTest, TrailingNewlineStripped) {
  std::vector<std::string> captured;
  auto mock = [&](const std::string &cmd) -> int {
    captured.push_back(cmd);
    return 0;
  };

  testing::internal::CaptureStdout();
  CmdSend(mock).Run({"0", "hello\n"});
  testing::internal::GetCapturedStdout();

  ASSERT_EQ(captured.size(), 2u);
  EXPECT_EQ(captured[0].find("hello\n"), std::string::npos);
  EXPECT_NE(captured[0].find("hello"), std::string::npos);
  EXPECT_NE(captured[1].find("C-m"), std::string::npos);
}

TEST(CmdSendTest, SendKeysCommandUsed) {
  std::vector<std::string> captured;
  auto mock = [&](const std::string &cmd) -> int {
    captured.push_back(cmd);
    return 0;
  };

  testing::internal::CaptureStdout();
  CmdSend(mock).Run({"0", "hello"});
  testing::internal::GetCapturedStdout();

  ASSERT_EQ(captured.size(), 2u);
  EXPECT_NE(captured[0].find("send-keys"), std::string::npos);
  EXPECT_NE(captured[1].find("C-m"), std::string::npos);
}

TEST(CmdSendTest, InvalidPaneReturnsError) {
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result = CmdSend{}.Run({"foo", "hello"});
  testing::internal::GetCapturedStdout();
  testing::internal::GetCapturedStderr();

  EXPECT_NE(result, 0);
}

TEST(CmdSendTest, BroadcastAllUsesLoop) {
  std::string captured;
  auto mock = [&](const std::string &cmd) -> int {
    captured = cmd;
    return 0;
  };

  testing::internal::CaptureStdout();
  int result = CmdSend(mock).Run({"all", "hello"});
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  EXPECT_NE(captured.find("list-panes"), std::string::npos);
  EXPECT_NE(captured.find("hello"), std::string::npos);
  EXPECT_NE(captured.find("C-m"), std::string::npos);
}

TEST(CmdSendTest, BroadcastAllUsesSessionName) {
  std::string captured;
  auto mock = [&](const std::string &cmd) -> int {
    captured = cmd;
    return 0;
  };

  testing::internal::CaptureStdout();
  CmdSend(mock).Run({"-n", "my-session", "all", "cmd"});
  testing::internal::GetCapturedStdout();

  EXPECT_NE(captured.find("my-session"), std::string::npos);
}

TEST(CmdSendTest, DelayZeroNoSleepInBroadcast) {
  std::string captured;
  auto mock = [&](const std::string &cmd) -> int {
    captured = cmd;
    return 0;
  };

  testing::internal::CaptureStdout();
  CmdSend(mock).Run({"-d", "0", "all", "hello"});
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(captured.find("sleep"), std::string::npos);
}

TEST(CmdSendTest, DelayPositiveAppearsInBroadcast) {
  std::string captured;
  auto mock = [&](const std::string &cmd) -> int {
    captured = cmd;
    return 0;
  };

  testing::internal::CaptureStdout();
  CmdSend(mock).Run({"-d", "0.5", "all", "hello"});
  testing::internal::GetCapturedStdout();

  EXPECT_NE(captured.find("sleep"), std::string::npos);
  EXPECT_NE(captured.find("0.5"), std::string::npos);
}

TEST(CmdSendTest, DelayOneSecondBroadcast) {
  std::string captured;
  auto mock = [&](const std::string &cmd) -> int {
    captured = cmd;
    return 0;
  };

  testing::internal::CaptureStdout();
  CmdSend(mock).Run({"-d", "1", "all", "hello"});
  testing::internal::GetCapturedStdout();

  EXPECT_NE(captured.find("sleep"), std::string::npos);
  EXPECT_NE(captured.find("1"), std::string::npos);
}

TEST(CmdSendTest, DelayWithSinglePaneStillCallsExecutorTwice) {
  std::vector<std::string> captured;
  auto mock = [&](const std::string &cmd) -> int {
    captured.push_back(cmd);
    return 0;
  };

  testing::internal::CaptureStdout();
  int result = CmdSend(mock).Run({"-d", "0", "0", "hello"});
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  ASSERT_EQ(captured.size(), 2u);
  EXPECT_NE(captured[0].find("hello"), std::string::npos);
  EXPECT_NE(captured[1].find("C-m"), std::string::npos);
}

TEST(CmdSendTest, TextFailureSkipsExec) {
  std::vector<std::string> captured;
  auto mock = [&](const std::string &cmd) -> int {
    captured.push_back(cmd);
    return 1;
  };

  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result = CmdSend(mock).Run({"0", "hello"});
  testing::internal::GetCapturedStdout();
  testing::internal::GetCapturedStderr();

  EXPECT_NE(result, 0);
  EXPECT_EQ(captured.size(), 1u);
}

TEST(CmdSendTest, HashNReplacedWithPaneNumberSinglePane) {
  std::vector<std::string> captured;
  auto mock = [&](const std::string &cmd) -> int {
    captured.push_back(cmd);
    return 0;
  };

  testing::internal::CaptureStdout();
  CmdSend(mock).Run({"3", "mensaje con #N enviado"});
  testing::internal::GetCapturedStdout();

  ASSERT_GE(captured.size(), 1u);
  EXPECT_NE(captured[0].find("mensaje con 3 enviado"), std::string::npos);
  for (const auto &c : captured)
    EXPECT_EQ(c.find("#N"), std::string::npos);
}

TEST(CmdSendTest, HashNReplacedWithShellVarInBroadcast) {
  std::string captured;
  auto mock = [&](const std::string &cmd) -> int {
    captured = cmd;
    return 0;
  };

  testing::internal::CaptureStdout();
  CmdSend(mock).Run({"all", "mensaje con #N enviado"});
  testing::internal::GetCapturedStdout();

  EXPECT_NE(captured.find("'$p'"), std::string::npos);
  EXPECT_EQ(captured.find("#N"), std::string::npos);
}

TEST(CmdSendTest, HashNMultipleOccurrencesReplaced) {
  std::vector<std::string> captured;
  auto mock = [&](const std::string &cmd) -> int {
    captured.push_back(cmd);
    return 0;
  };

  testing::internal::CaptureStdout();
  CmdSend(mock).Run({"2", "#N:#N"});
  testing::internal::GetCapturedStdout();

  ASSERT_GE(captured.size(), 1u);
  EXPECT_NE(captured[0].find("2:2"), std::string::npos);
  for (const auto &c : captured)
    EXPECT_EQ(c.find("#N"), std::string::npos);
}

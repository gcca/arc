#include "CmdSession.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <toml++/toml.hpp>

static std::filesystem::path TempFile() {
  return std::filesystem::temp_directory_path() / "arc-session-test.toml";
}

class CmdSessionTest : public ::testing::Test {
protected:
  std::filesystem::path file_ = TempFile();

  void TearDown() override { std::filesystem::remove(file_); }
};

TEST_F(CmdSessionTest, CreateWritesVersion) {
  ASSERT_EQ(CmdSession(file_).Run({}), 0);
  auto tbl = toml::parse_file(file_.string());
  EXPECT_EQ(tbl["version"].value<std::string>(), ARC_VERSION);
}

TEST_F(CmdSessionTest, CreateIsIdempotent) {
  ASSERT_EQ(CmdSession(file_).Run({}), 0);
  ASSERT_EQ(CmdSession(file_).Run({}), 0);
  auto tbl = toml::parse_file(file_.string());
  EXPECT_EQ(tbl["version"].value<std::string>(), ARC_VERSION);
}

TEST_F(CmdSessionTest, SetWritesName) {
  ASSERT_EQ(CmdSession(file_).Run({"set", "--name", "my-session"}), 0);
  auto tbl = toml::parse_file(file_.string());
  EXPECT_EQ(tbl["name"].value<std::string>(), "my-session");
  EXPECT_EQ(tbl["version"].value<std::string>(), ARC_VERSION);
}

TEST_F(CmdSessionTest, SetShortFlag) {
  ASSERT_EQ(CmdSession(file_).Run({"set", "-n", "short-session"}), 0);
  auto tbl = toml::parse_file(file_.string());
  EXPECT_EQ(tbl["name"].value<std::string>(), "short-session");
}

TEST_F(CmdSessionTest, AddPaneWritesEntry) {
  ASSERT_EQ(CmdSession(file_).Run({"add", "--pane", "0:claude"}), 0);
  auto tbl = toml::parse_file(file_.string());
  ASSERT_TRUE(tbl["panes"].is_table());
  EXPECT_EQ(tbl["panes"]["0"].value<std::string>(), "claude");
  EXPECT_EQ(tbl["version"].value<std::string>(), ARC_VERSION);
}

TEST_F(CmdSessionTest, AddPaneShortFlag) {
  ASSERT_EQ(CmdSession(file_).Run({"add", "-p", "1:opencode"}), 0);
  auto tbl = toml::parse_file(file_.string());
  EXPECT_EQ(tbl["panes"]["1"].value<std::string>(), "opencode");
}

TEST_F(CmdSessionTest, AddMultiplePanes) {
  ASSERT_EQ(CmdSession(file_).Run({"add", "-p", "0:claude", "-p", "1:opencode",
                                   "-p", "2:agent --api-key $KEY"}),
            0);
  auto tbl = toml::parse_file(file_.string());
  EXPECT_EQ(tbl["panes"]["0"].value<std::string>(), "claude");
  EXPECT_EQ(tbl["panes"]["1"].value<std::string>(), "opencode");
  EXPECT_EQ(tbl["panes"]["2"].value<std::string>(), "agent --api-key $KEY");
}

TEST_F(CmdSessionTest, AddPanePreservesExistingPanes) {
  ASSERT_EQ(CmdSession(file_).Run({"add", "-p", "0:claude"}), 0);
  ASSERT_EQ(CmdSession(file_).Run({"add", "-p", "1:opencode"}), 0);
  auto tbl = toml::parse_file(file_.string());
  EXPECT_EQ(tbl["panes"]["0"].value<std::string>(), "claude");
  EXPECT_EQ(tbl["panes"]["1"].value<std::string>(), "opencode");
}

TEST_F(CmdSessionTest, AddPaneMissingSeparatorReturnsError) {
  testing::internal::CaptureStderr();
  int result = CmdSession(file_).Run({"add", "-p", "0"});
  testing::internal::GetCapturedStderr();
  EXPECT_NE(result, 0);
}

TEST_F(CmdSessionTest, SetPreservesExistingFields) {
  ASSERT_EQ(CmdSession(file_).Run({"set", "-n", "first"}), 0);
  ASSERT_EQ(CmdSession(file_).Run({"set", "-n", "second"}), 0);
  auto tbl = toml::parse_file(file_.string());
  EXPECT_EQ(tbl["name"].value<std::string>(), "second");
  EXPECT_EQ(tbl["version"].value<std::string>(), ARC_VERSION);
}

TEST_F(CmdSessionTest, SetAllPanesWritesEntry) {
  ASSERT_EQ(CmdSession(file_).Run(
                {"set", "--all_panes", "reset;agent --model composer-1.5"}),
            0);
  auto tbl = toml::parse_file(file_.string());
  EXPECT_EQ(tbl["all_panes"].value<std::string>(),
            "reset;agent --model composer-1.5");
}

TEST_F(CmdSessionTest, SetAllPanesPreservesName) {
  ASSERT_EQ(CmdSession(file_).Run({"set", "-n", "my-session"}), 0);
  ASSERT_EQ(CmdSession(file_).Run({"set", "--all_panes", "reset;agent"}), 0);
  auto tbl = toml::parse_file(file_.string());
  EXPECT_EQ(tbl["name"].value<std::string>(), "my-session");
  EXPECT_EQ(tbl["all_panes"].value<std::string>(), "reset;agent");
}

TEST_F(CmdSessionTest, SetHelpShowsUsageLine) {
  testing::internal::CaptureStdout();
  CmdSession(file_).Run({"set", "--help"});
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_NE(out.find("Usage: arc session set"), std::string::npos);
}

TEST_F(CmdSessionTest, AddHelpShowsUsageLine) {
  testing::internal::CaptureStdout();
  CmdSession(file_).Run({"add", "--help"});
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_NE(out.find("Usage: arc session add"), std::string::npos);
}

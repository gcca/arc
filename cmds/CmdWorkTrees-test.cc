#include "CmdWorkTrees.hpp"

#include <gtest/gtest.h>

TEST(CmdWorkTreesTest, NoArgsShowsUsage) {
  testing::internal::CaptureStdout();
  int result = CmdWorkTrees{}.Run({});
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  EXPECT_NE(out.find("Usage:"), std::string::npos);
  EXPECT_NE(out.find("list"), std::string::npos);
}

TEST(CmdWorkTreesTest, HelpFlagShowsUsage) {
  testing::internal::CaptureStdout();
  int result = CmdWorkTrees{}.Run({"--help"});
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  EXPECT_NE(out.find("Usage:"), std::string::npos);
  EXPECT_NE(out.find("--regex"), std::string::npos);
}

TEST(CmdWorkTreesTest, ShortHelpFlagShowsUsage) {
  testing::internal::CaptureStdout();
  int result = CmdWorkTrees{}.Run({"-h"});
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  EXPECT_NE(out.find("Usage:"), std::string::npos);
}

TEST(CmdWorkTreesTest, UnknownSubcommandReturnsError) {
  testing::internal::CaptureStderr();
  int result = CmdWorkTrees{}.Run({"unknown"});
  std::string err = testing::internal::GetCapturedStderr();

  EXPECT_NE(result, 0);
  EXPECT_NE(err.find("unknown subcommand"), std::string::npos);
}

TEST(CmdWorkTreesTest, ListDefaultPatternSucceeds) {
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result = CmdWorkTrees{}.Run({"list"});
  std::string out = testing::internal::GetCapturedStdout();
  testing::internal::GetCapturedStderr();

  EXPECT_EQ(result, 0);
  EXPECT_NE(out.find("wt-\\d"), std::string::npos);
}

TEST(CmdWorkTreesTest, ListCustomRegexLongFlag) {
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result = CmdWorkTrees{}.Run({"--regex", ".*", "list"});
  std::string out = testing::internal::GetCapturedStdout();
  testing::internal::GetCapturedStderr();

  EXPECT_EQ(result, 0);
  EXPECT_NE(out.find(".*"), std::string::npos);
}

TEST(CmdWorkTreesTest, ListCustomRegexShortFlag) {
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result = CmdWorkTrees{}.Run({"-r", "feature-.*", "list"});
  std::string out = testing::internal::GetCapturedStdout();
  testing::internal::GetCapturedStderr();

  EXPECT_EQ(result, 0);
  EXPECT_NE(out.find("feature-.*"), std::string::npos);
}

TEST(CmdWorkTreesTest, HelpShowsRegexOption) {
  testing::internal::CaptureStdout();
  int result = CmdWorkTrees{}.Run({"--help"});
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  EXPECT_NE(out.find("--regex"), std::string::npos);
}

TEST(CmdWorkTreesTest, ListInvalidRegexReturnsError) {
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result = CmdWorkTrees{}.Run({"-r", "[invalid", "list"});
  testing::internal::GetCapturedStdout();
  std::string err = testing::internal::GetCapturedStderr();

  EXPECT_NE(result, 0);
  EXPECT_NE(err.find("invalid regex pattern"), std::string::npos);
}

TEST(CmdWorkTreesTest, PruneNoMatchPatternSucceeds) {
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result = CmdWorkTrees{}.Run({"--regex", "no-match-prune-.*", "prune"});
  std::string out = testing::internal::GetCapturedStdout();
  testing::internal::GetCapturedStderr();

  EXPECT_EQ(result, 0);
  EXPECT_NE(out.find("no-match-prune-.*"), std::string::npos);
}

TEST(CmdWorkTreesTest, PruneCustomRegexLongFlag) {
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result = CmdWorkTrees{}.Run({"--regex", "no-match-.*", "prune"});
  std::string out = testing::internal::GetCapturedStdout();
  testing::internal::GetCapturedStderr();

  EXPECT_EQ(result, 0);
  EXPECT_NE(out.find("no-match-.*"), std::string::npos);
}

TEST(CmdWorkTreesTest, PruneCustomRegexShortFlag) {
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result = CmdWorkTrees{}.Run({"-r", "no-match-.*", "prune"});
  std::string out = testing::internal::GetCapturedStdout();
  testing::internal::GetCapturedStderr();

  EXPECT_EQ(result, 0);
  EXPECT_NE(out.find("no-match-.*"), std::string::npos);
}

TEST(CmdWorkTreesTest, PruneInvalidRegexReturnsError) {
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result = CmdWorkTrees{}.Run({"-r", "[invalid", "prune"});
  testing::internal::GetCapturedStdout();
  std::string err = testing::internal::GetCapturedStderr();

  EXPECT_NE(result, 0);
  EXPECT_NE(err.find("invalid regex pattern"), std::string::npos);
}

TEST(CmdWorkTreesTest, HelpMentionsPruneSubcommand) {
  testing::internal::CaptureStdout();
  CmdWorkTrees{}.Run({"--help"});
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_NE(out.find("prune"), std::string::npos);
}

TEST(CmdWorkTreesTest, HelpMentionsCpSubcommand) {
  testing::internal::CaptureStdout();
  CmdWorkTrees{}.Run({"--help"});
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_NE(out.find("cp"), std::string::npos);
}

TEST(CmdWorkTreesTest, CpMissingFileReturnsError) {
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result = CmdWorkTrees{}.Run({"cp"});
  testing::internal::GetCapturedStdout();
  std::string err = testing::internal::GetCapturedStderr();

  EXPECT_NE(result, 0);
  EXPECT_NE(err.find("file template required"), std::string::npos);
}

TEST(CmdWorkTreesTest, CpHelpFlag) {
  testing::internal::CaptureStdout();
  int result = CmdWorkTrees{}.Run({"--help"});
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  EXPECT_NE(out.find("cp"), std::string::npos);
}

TEST(CmdWorkTreesTest, CpInvalidRegexReturnsError) {
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result = CmdWorkTrees{}.Run({"-r", "[invalid", "cp", "file-#N.md"});
  testing::internal::GetCapturedStdout();
  std::string err = testing::internal::GetCapturedStderr();

  EXPECT_NE(result, 0);
  EXPECT_NE(err.find("invalid regex pattern"), std::string::npos);
}

TEST(CmdWorkTreesTest, HelpMentionsRestoreSubcommand) {
  testing::internal::CaptureStdout();
  CmdWorkTrees{}.Run({"--help"});
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_NE(out.find("restore"), std::string::npos);
}

TEST(CmdWorkTreesTest, RestoreNoMatchPatternSucceeds) {
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result =
      CmdWorkTrees{}.Run({"--regex", "no-match-restore-.*", "restore"});
  std::string out = testing::internal::GetCapturedStdout();
  testing::internal::GetCapturedStderr();

  EXPECT_EQ(result, 0);
  EXPECT_NE(out.find("no-match-restore-.*"), std::string::npos);
}

TEST(CmdWorkTreesTest, RestoreInvalidRegexReturnsError) {
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result = CmdWorkTrees{}.Run({"-r", "[invalid", "restore"});
  testing::internal::GetCapturedStdout();
  std::string err = testing::internal::GetCapturedStderr();

  EXPECT_NE(result, 0);
  EXPECT_NE(err.find("invalid regex pattern"), std::string::npos);
}

TEST(CmdWorkTreesTest, CpNoMatchingWorktreesSkipsAll) {
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  int result =
      CmdWorkTrees{}.Run({"--regex", "no-match-xyz-.*", "cp", "file-#N.md"});
  std::string out = testing::internal::GetCapturedStdout();
  testing::internal::GetCapturedStderr();

  EXPECT_EQ(result, 0);
  EXPECT_NE(out.find("skip"), std::string::npos);
}

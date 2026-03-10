#include <gtest/gtest.h>

#include "CmdHelp.hpp"

TEST(CmdHelpTest, RunShowsUsage) {
  testing::internal::CaptureStdout();
  int result = CmdHelp{}.Run({});
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  EXPECT_NE(out.find("arc v"), std::string::npos);
}

TEST(CmdHelpTest, ListsStartCommand) {
  testing::internal::CaptureStdout();
  CmdHelp{}.Run({});
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_NE(out.find("start"), std::string::npos);
  EXPECT_NE(out.find("tmux session"), std::string::npos);
  EXPECT_EQ(out.find("kitty"), std::string::npos);
}

TEST(CmdHelpTest, SessionMentionsAllPanes) {
  testing::internal::CaptureStdout();
  CmdHelp{}.Run({});
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_NE(out.find("all_panes"), std::string::npos);
}

TEST(CmdHelpTest, ListsHelpCommand) {
  testing::internal::CaptureStdout();
  CmdHelp{}.Run({});
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_NE(out.find("help"), std::string::npos);
}

#include <gtest/gtest.h>

#include "CmdDefault.hpp"

TEST(CmdDefaultTest, NoArgsShowsReady) {
  testing::internal::CaptureStdout();
  int result = CmdDefault{}.Run({});
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  EXPECT_NE(out.find("arc v"), std::string::npos);
}

TEST(CmdDefaultTest, HelpShowsUsage) {
  testing::internal::CaptureStdout();
  int result = CmdDefault{}.Run({"--help"});
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
  EXPECT_NE(out.find("Commands:"), std::string::npos);
  EXPECT_NE(out.find("Global options"), std::string::npos);
}

TEST(CmdDefaultTest, ShortHelpShowsUsage) {
  testing::internal::CaptureStdout();
  int result = CmdDefault{}.Run({"-h"});
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(result, 0);
}

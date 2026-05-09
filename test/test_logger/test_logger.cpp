#include <gtest/gtest.h>

#include "logger.h"
#include "logger.cpp"

struct Logger_test : public ::testing::Test
{
  void SetUp() override
  {
    SD.reset();
    Logger::setup("/test.log", 50);
    Logger::unmute();
  }
};

TEST_F(Logger_test, initially_not_muted)
{
  EXPECT_FALSE(Logger::is_muted());
}

TEST_F(Logger_test, mute_sets_flag)
{
  Logger::mute();
  EXPECT_TRUE(Logger::is_muted());
}

TEST_F(Logger_test, unmute_clears_flag)
{
  Logger::mute();
  Logger::unmute();
  EXPECT_FALSE(Logger::is_muted());
}

TEST_F(Logger_test, info_opens_sd_when_not_muted)
{
  Logger::info("TAG", "message");
  EXPECT_GT(SD.open_count, 0);
}

TEST_F(Logger_test, warn_opens_sd_when_not_muted)
{
  Logger::warn("TAG", "message");
  EXPECT_GT(SD.open_count, 0);
}

TEST_F(Logger_test, error_opens_sd_when_not_muted)
{
  Logger::error("TAG", "message");
  EXPECT_GT(SD.open_count, 0);
}

TEST_F(Logger_test, info_does_not_open_sd_when_muted)
{
  Logger::mute();
  Logger::info("TAG", "message");
  EXPECT_EQ(SD.open_count, 0);
}

TEST_F(Logger_test, warn_does_not_open_sd_when_muted)
{
  Logger::mute();
  Logger::warn("TAG", "message");
  EXPECT_EQ(SD.open_count, 0);
}

TEST_F(Logger_test, error_does_not_open_sd_when_muted)
{
  Logger::mute();
  Logger::error("TAG", "message");
  EXPECT_EQ(SD.open_count, 0);
}

TEST_F(Logger_test, unmute_restores_sd_writes)
{
  Logger::mute();
  Logger::info("TAG", "while muted");
  EXPECT_EQ(SD.open_count, 0);

  Logger::unmute();
  Logger::info("TAG", "after unmute");
  EXPECT_GT(SD.open_count, 0);
}

TEST_F(Logger_test, no_sd_access_when_path_empty)
{
  Logger::setup("", 50);
  Logger::info("TAG", "no path");
  EXPECT_EQ(SD.open_count, 0);
}

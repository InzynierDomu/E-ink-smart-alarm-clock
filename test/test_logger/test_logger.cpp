#include <gtest/gtest.h>

#include "logger.h"
#include "logger.cpp"

struct Logger_test : public ::testing::Test
{
  void SetUp() override
  {
    SD.reset();
    Logger::setup("/test.log", 50);
  }
};

TEST_F(Logger_test, info_opens_sd_when_path_set)
{
  Logger::info("TAG", "message");
  EXPECT_GT(SD.open_count, 0);
}

TEST_F(Logger_test, warn_opens_sd_when_path_set)
{
  Logger::warn("TAG", "message");
  EXPECT_GT(SD.open_count, 0);
}

TEST_F(Logger_test, error_opens_sd_when_path_set)
{
  Logger::error("TAG", "message");
  EXPECT_GT(SD.open_count, 0);
}

TEST_F(Logger_test, no_sd_access_when_path_empty)
{
  Logger::setup("", 50);
  Logger::info("TAG", "no path");
  EXPECT_EQ(SD.open_count, 0);
}

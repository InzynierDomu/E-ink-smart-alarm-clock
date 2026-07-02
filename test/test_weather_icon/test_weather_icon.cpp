#include <gtest/gtest.h>
#include <Arduino.h>

#include "weather_icon.h"
#include "weather_icon.cpp"

// ---------------------------------------------------------------------------
// Day icons — is_night = false
// ---------------------------------------------------------------------------

TEST(Weather_icon_test, clear_day_returns_sun)
{
  EXPECT_STREQ("\xEF\x80\x8D", weather_icon(0, 0, false));
}

TEST(Weather_icon_test, low_cloud_no_rain_day_returns_sun)
{
  EXPECT_STREQ("\xEF\x80\x8D", weather_icon(30, 0, false));
}

TEST(Weather_icon_test, high_cloud_no_rain_day_returns_partly_cloudy)
{
  EXPECT_STREQ("\xEF\x80\x82", weather_icon(31, 0, false));
}

TEST(Weather_icon_test, light_rain_day_returns_light_rain)
{
  EXPECT_STREQ("\xEF\x80\x9A", weather_icon(50, 1, false));
}

TEST(Weather_icon_test, rain_below_threshold_day_returns_light_rain)
{
  EXPECT_STREQ("\xEF\x80\x9A", weather_icon(50, 4, false));
}

TEST(Weather_icon_test, heavy_rain_at_threshold_day_returns_heavy_rain)
{
  EXPECT_STREQ("\xEF\x80\x99", weather_icon(50, 5, false));
}

TEST(Weather_icon_test, heavy_rain_above_threshold_day_returns_heavy_rain)
{
  EXPECT_STREQ("\xEF\x80\x99", weather_icon(80, 20, false));
}

// ---------------------------------------------------------------------------
// Night icons — is_night = true
// ---------------------------------------------------------------------------

TEST(Weather_icon_test, clear_night_returns_moon)
{
  EXPECT_STREQ("\xEF\x80\xAE", weather_icon(0, 0, true));
}

TEST(Weather_icon_test, low_cloud_no_rain_night_returns_moon)
{
  EXPECT_STREQ("\xEF\x80\xAE", weather_icon(30, 0, true));
}

TEST(Weather_icon_test, high_cloud_no_rain_night_returns_cloudy_moon)
{
  EXPECT_STREQ("\xEF\x82\x86", weather_icon(31, 0, true));
}

TEST(Weather_icon_test, light_rain_night_returns_light_rain)
{
  EXPECT_STREQ("\xEF\x80\x9A", weather_icon(50, 1, true));
}

TEST(Weather_icon_test, heavy_rain_night_returns_heavy_rain)
{
  EXPECT_STREQ("\xEF\x80\x99", weather_icon(50, 5, true));
}

// ---------------------------------------------------------------------------
// Rain takes priority over cloud/night
// ---------------------------------------------------------------------------

TEST(Weather_icon_test, rain_overrides_night_flag)
{
  EXPECT_STREQ("\xEF\x80\x99", weather_icon(0, 5, true));
}

TEST(Weather_icon_test, light_rain_overrides_cloud_cover)
{
  EXPECT_STREQ("\xEF\x80\x9A", weather_icon(0, 1, false));
}

#include "test_weather_model.h"

#include "weather_model.cpp"

TEST_F(Weather_model_test, update_and_get_today_forecast)
{
  Simple_weather w{};
  w.temperature_morning = 10;
  w.temperature_afternoon = 20;
  w.temperature_evening = 15;
  w.temperature_night = 5;
  w.cloud_cover = 30;
  w.precipitation = 2;

  uut.update_at(0, w);

  Simple_weather out;
  uut.get_forecast(out, 0);
  EXPECT_EQ(10, out.temperature_morning);
  EXPECT_EQ(20, out.temperature_afternoon);
  EXPECT_EQ(15, out.temperature_evening);
  EXPECT_EQ(5, out.temperature_night);
  EXPECT_EQ(30, out.cloud_cover);
  EXPECT_EQ(2, out.precipitation);
}

TEST_F(Weather_model_test, update_multiple_days_independently)
{
  Simple_weather day0{};
  day0.temperature_afternoon = 20;
  Simple_weather day1{};
  day1.temperature_afternoon = 25;

  uut.update_at(0, day0);
  uut.update_at(1, day1);

  Simple_weather out;
  uut.get_forecast(out, 0);
  EXPECT_EQ(20, out.temperature_afternoon);
  uut.get_forecast(out, 1);
  EXPECT_EQ(25, out.temperature_afternoon);
}

TEST_F(Weather_model_test, get_forecast_out_of_bounds_returns_zeroed)
{
  Simple_weather w{};
  w.temperature_morning = 99;
  uut.update_at(0, w);

  Simple_weather out;
  out.temperature_morning = 99;
  uut.get_forecast(out, WEATHER_DAYS);

  EXPECT_EQ(0, out.temperature_morning);
}

TEST_F(Weather_model_test, update_at_out_of_bounds_does_not_crash)
{
  Simple_weather w{};
  w.temperature_morning = 99;
  uut.update_at(WEATHER_DAYS, w);

  Simple_weather out;
  uut.get_forecast(out, 0);
  EXPECT_EQ(0, out.temperature_morning);
}

TEST_F(Weather_model_test, set_and_get_day_part_morning)
{
  uut.set_day_part(Day_part::morning);
  EXPECT_EQ(Day_part::morning, uut.get_day_part());
}

TEST_F(Weather_model_test, set_and_get_day_part_afternoon)
{
  uut.set_day_part(Day_part::afternoon);
  EXPECT_EQ(Day_part::afternoon, uut.get_day_part());
}

TEST_F(Weather_model_test, set_and_get_day_part_evening)
{
  uut.set_day_part(Day_part::evening);
  EXPECT_EQ(Day_part::evening, uut.get_day_part());
}

TEST_F(Weather_model_test, set_and_get_day_part_night)
{
  uut.set_day_part(Day_part::night);
  EXPECT_EQ(Day_part::night, uut.get_day_part());
}

TEST_F(Weather_model_test, set_and_get_config)
{
  Open_weather_config cfg;
  cfg.api_key = "testkey123";
  cfg.lat = 52.2297f;
  cfg.lon = 21.0122f;

  uut.set_config(cfg);

  Open_weather_config out;
  uut.get_config(out);
  EXPECT_FALSE(out.api_key.isEmpty());
  EXPECT_FLOAT_EQ(52.2297f, out.lat);
  EXPECT_FLOAT_EQ(21.0122f, out.lon);
}

TEST_F(Weather_model_test, config_api_key_empty_by_default)
{
  Open_weather_config out;
  uut.get_config(out);
  EXPECT_TRUE(out.api_key.isEmpty());
}

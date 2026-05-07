#pragma once

#include "weather_model.h"

#include "gtest/gtest.h"

struct Weather_model_test : public ::testing::Test
{
  protected:
  Weather_model_test() {}

  Weather_model uut;
};

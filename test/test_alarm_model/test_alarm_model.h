#pragma once

#include "alarm_model.h"

#include "gtest/gtest.h"

struct Alarm_model_test : public ::testing::Test
{
  protected:
  Alarm_model_test() {}

  Alarm_model uut;
};

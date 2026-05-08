#pragma once

#include "calendar_model.h"

#include "gtest/gtest.h"

struct Calendar_model_test : public ::testing::Test
{
  protected:
  Calendar_model_test() {}

  Calendar_model uut;
};

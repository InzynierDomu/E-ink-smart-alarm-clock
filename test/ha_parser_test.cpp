#include "ha_parser_test.h"

#include "ha_parser.cpp"

// --- is_ha_response_ok ---

TEST_F(Ha_parser_test, response_200_is_ok)
{
  EXPECT_TRUE(is_ha_response_ok("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n"));
}

TEST_F(Ha_parser_test, response_404_is_not_ok)
{
  EXPECT_FALSE(is_ha_response_ok("HTTP/1.1 404 Not Found\r\n"));
}

TEST_F(Ha_parser_test, response_401_is_not_ok)
{
  EXPECT_FALSE(is_ha_response_ok("HTTP/1.1 401 Unauthorized\r\n"));
}

TEST_F(Ha_parser_test, empty_headers_is_not_ok)
{
  EXPECT_FALSE(is_ha_response_ok(""));
}

// --- parse_ha_state ---

TEST_F(Ha_parser_test, parse_valid_positive_temperature)
{
  String body = "{\"entity_id\":\"sensor.temperatura\",\"state\":\"21\","
                "\"attributes\":{\"unit_of_measurement\":\"\\u00b0C\"}}";
  EXPECT_EQ(21, parse_ha_state(body));
}

TEST_F(Ha_parser_test, parse_valid_negative_temperature)
{
  String body = "{\"entity_id\":\"sensor.temperatura\",\"state\":\"-7\","
                "\"attributes\":{\"unit_of_measurement\":\"\\u00b0C\"}}";
  EXPECT_EQ(-7, parse_ha_state(body));
}

TEST_F(Ha_parser_test, parse_zero_temperature)
{
  String body = "{\"entity_id\":\"sensor.temperatura\",\"state\":\"0\","
                "\"attributes\":{}}";
  EXPECT_EQ(0, parse_ha_state(body));
}

TEST_F(Ha_parser_test, parse_empty_body_returns_zero)
{
  EXPECT_EQ(0, parse_ha_state(""));
}

TEST_F(Ha_parser_test, parse_body_without_state_field_returns_zero)
{
  String body = "{\"entity_id\":\"sensor.temperatura\",\"attributes\":{}}";
  EXPECT_EQ(0, parse_ha_state(body));
}

TEST_F(Ha_parser_test, parse_body_with_malformed_state_value_returns_zero)
{
  String body = "{\"state\":";
  EXPECT_EQ(0, parse_ha_state(body));
}

TEST_F(Ha_parser_test, parse_state_field_in_long_realistic_body)
{
  String body = "{\"entity_id\":\"sensor.temperatura\",\"state\":\"18\","
                "\"attributes\":{\"unit_of_measurement\":\"\\u00b0C\","
                "\"friendly_name\":\"Temperatura\"},"
                "\"last_changed\":\"2024-01-15T07:30:00+00:00\","
                "\"last_updated\":\"2024-01-15T07:30:00+00:00\"}";
  EXPECT_EQ(18, parse_ha_state(body));
}

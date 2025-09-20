namespace config
{
constexpr uint8_t sd_cs_pin = 10;
constexpr uint8_t sd_power_pin = 42;
constexpr uint8_t screen_power_pin = 7;
constexpr uint8_t alarm_enable_button_pin = 2;

const String config_path = "/config.json";

constexpr unsigned int local_port = 2390; // local port to listen for UDP packets
constexpr char time_server[] = "tempus1.gum.gov.pl"; // extenral NTP server
constexpr int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
} // namespace config
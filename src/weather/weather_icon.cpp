#include "weather_icon.h"

#include "config.h"

const char* weather_icon(uint8_t cloud_cover, uint8_t precipitation, bool is_night)
{
  if (precipitation >= config::weather_icon_precip_light_threshold)
    return "\xEF\x80\x99"; // f019 heavy rain

  if (precipitation > 0)
    return "\xEF\x80\x9A"; // f01a light rain

  if (cloud_cover > config::weather_icon_cloud_threshold)
    return is_night ? "\xEF\x82\x86"  // f086 cloudy moon
                    : "\xEF\x80\x82"; // f002 partly cloudy day

  return is_night ? "\xEF\x80\xAE"  // f02e moon
                  : "\xEF\x80\x8D"; // f00d sun
}

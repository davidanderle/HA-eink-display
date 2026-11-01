#ifndef BATTERY_H
#define BATTERY_H

#include <stdint.h>

void battery_init(void);
/// @brief Battery state of charge in percent (0-100), averaged over a few ADC 
/// samples. Returns 100 if the ADC is not available.
uint8_t battery_get_percent(void);

#endif

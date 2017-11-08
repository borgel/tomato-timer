#pragma once

#include <stdint.h>
#include <stdbool.h>

bool eeprom_Write(uint8_t addr, uint8_t * buffer, unsigned length);
bool eeprom_Read(uint8_t addr, uint8_t * buffer, unsigned length);

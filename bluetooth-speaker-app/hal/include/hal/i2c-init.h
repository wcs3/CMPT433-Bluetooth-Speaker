// helper functions for reading and writing to i2c devices
#ifndef _I2C_INNIT_H_
#define _I2C_INNIT_H_

#include <stdint.h>

int init_i2c_bus(const char* bus, int address, int* result);
int write_i2c_reg16(int i2c_file_desc, uint8_t reg_addr, uint16_t value);
// result* contains the raw register value
int read_i2c_reg16(int i2c_file_desc, uint8_t reg_addr, uint16_t* result);
void close_i2c_bus(int i2c_file_desc);
#endif
#include "hal/i2c_init.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


// from https://opencoursehub.cs.sfu.ca/bfraser/grav-cms/cmpt433/guides/files_byai/I2CGuide.pdf
int init_i2c_bus(const char* bus, int address, int* result)
{
    int i2c_file_desc = open(bus, O_RDWR);
    if (i2c_file_desc == -1) {
        return 1;
    }
    
    if (ioctl(i2c_file_desc, I2C_SLAVE, address) == -1) {
        close(i2c_file_desc);
        return 2;
    }
    *result = i2c_file_desc;
    return 0;
}

// from https://opencoursehub.cs.sfu.ca/bfraser/grav-cms/cmpt433/guides/files_byai/I2CGuide.pdf
int write_i2c_reg16(int i2c_file_desc, uint8_t reg_addr, uint16_t value)
{
    int tx_size = 1 + sizeof(value);
    uint8_t buff[tx_size];
    buff[0] = reg_addr;
    buff[1] = (value & 0xFF);
    buff[2] = (value & 0xFF00) >> 8;
    // printf("write %hx %hx\n", buff[1], buff[2]);
    int bytes_written = write(i2c_file_desc, buff, tx_size);
    if (bytes_written != tx_size) {
        return 1;
    }
    return 0;
}

// from https://opencoursehub.cs.sfu.ca/bfraser/grav-cms/cmpt433/guides/files_byai/I2CGuide.pdf
// does not reverse byte order
int read_i2c_reg16(int i2c_file_desc, uint8_t reg_addr, uint16_t* result)
{
    // To read a register, must first write the address 
    int bytes_written = write(i2c_file_desc, &reg_addr, sizeof(reg_addr));
    if (bytes_written != sizeof(reg_addr)) {
        return 1;
    }
    // Now read the value and return it
    uint16_t value = 0;
    int bytes_read = read(i2c_file_desc, &value, sizeof(value));
    if (bytes_read != sizeof(value)) {
        return 2;
    }
    *result = value;
    return 0;
}

void close_i2c_bus(int i2c_file_desc)
{
    close(i2c_file_desc);
}
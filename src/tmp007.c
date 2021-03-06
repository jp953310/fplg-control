/**
 *  \file   tmp007.c
 *  \author James Petersen <jpetersenames@gmail.com>
 */

#include "../include/base.h"
#include "../include/tmp007.h"
#include "../include/logging.h"

/*
 *  2 byte data union
 */
union data16
{
    uint16_t value;
    uint8_t  byte[2];
};


/*
 *  static function prototypes
 */
static inline int configure_reg(const int fd);
static uint16_t tmp_read_data16(const int fd, const uint8_t reg);
static ssize_t __tmp_write_data__(const int fd, const uint8_t reg, const uint8_t *data, const size_t size);
static float convert_temp_f(const union data16 data);

/**
 *  tmp_tmp_write_data MACRO
 */
#define tmp_write_data(fd, reg, data) __tmp_write_data__(fd, reg, data, sizeof(data)/sizeof(uint8_t))

/**
 *  setup()
 *
 *  \returns Fully allocated i2c_device struct if setup is good, NULL on error.
 */
struct i2c_device *tmp007_setup()
{
    struct i2c_device *tmp;
    int stat = 0;

    tmp = malloc(sizeof(struct i2c_device));
    tmp->addr  = TMP007_ID;
    tmp->dev   = TMP007_DEV;

    /*
     *   open up the device
     */
    if ((tmp->fd = open(tmp->dev, O_RDWR)) < 0)
    {
        LOG_ERROR_S("Opening TMP007\n");
        return NULL;
    }
    else
        LOG_INFO_S("TMP007 Opened Successfully\n");

    /*
     *   initialize the device as a slave
     */
    if (ioctl(tmp->fd, I2C_SLAVE, tmp->addr) < 0)
    {
        LOG_ERROR_S("Initializing Communication with TMP007\n");
        return NULL;
    }
    else
        LOG_INFO_S("TMP007 Communication Initialized\n");

    /*
     *  send the configuration to the configure register
     */
    if (configure_reg(tmp->fd) < 0)
    {
        LOG_ERROR_S("Error: Configuring TMP007\n");
        return NULL;
    }
    else
        LOG_INFO_S("TMP007 Configured\n");

    return tmp;
}


/**
 *  configure_reg()
 *  \returns number of bytes written.
 */
static inline int configure_reg(const int fd)
{
    uint8_t config[2];
    config[0] = 0x15;
    config[1] = 0x40;
    return tmp_write_data(fd, CONFIGURATION_REGISTER, config);
}


/**
 *  tmp_read_data16()
 *  \returns 2 bytes of read data, ERROR otherwise.
 */
static uint16_t tmp_read_data16(const int fd, const uint8_t reg)
{
    uint16_t data;
    if (write(fd, &reg, 1) != 1)
        return ERROR;

    if (read(fd, &data, sizeof(data)) != sizeof(data))
    {
        LOG_ERROR("Error on read %d\n", __LINE__);
        LOG_ERROR_S("Register not written, value returned\n");
        return ERROR;
    }
    return data;
}

/**
 *  __tmp_write_data__()
 *  This is the main function for the tmp_write_data() macro
 *  \returns Number of bytes written, ERROR otherwise.
 */
static ssize_t __tmp_write_data__(const int fd, const uint8_t reg, const uint8_t *data, const size_t size)
{
    if (size > 2)
    {
        LOG_ERROR_S("Error tmp_write_data: cannot write more than 2 bytes, registers are 16 bit\n");
        return ERROR;
    }
    int len = size+1;
    uint8_t w[len];
    w[0] = reg;
    w[1] = data[0];
    if (len == 3)
        w[2] = data[1];

    return write(fd, w, len);
}

/**
 *  convert_temp_f()
 *  \returns Converted temperature data in fahrenheit.
 */
static float convert_temp_f(const union data16 data)
{
    /*
     *  flip the Intel little-endianess
     */
    uint16_t hb = data.byte[0] << 8;
    uint8_t  lb = data.byte[1] & 0xFC;
    int tmp     = (hb + lb) >> 2;

    float celsius = tmp * 0.03125;
    return celsius * 1.8 + 32;
}


/**
 *  tmp007_read_temp()
 *
 *  \returns The temperature in Fahrenheit.
 *  TODO: add error checks
 */
float tmp007_read_temp(const struct i2c_device *self)
{
    union data16 data;
    if ((data.value = tmp_read_data16(self->fd, OBJECT_TEMP_REGISTER)) == ERROR)
    {
        LOG_ERROR_S("Error getting temp\n");
        return ERROR;
    }
    return convert_temp_f(data);
}

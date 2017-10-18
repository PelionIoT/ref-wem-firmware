#ifndef _COMPAT_H
#define _COMPAT_H

#if TARGET_UBLOX_EVK_ODIN_W2
#define I2C_SDA PF_0
#define I2C_SCL PF_1
#define SPIF_SPI_CS D10
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#endif /* #ifndef _COMPAT_H_ */

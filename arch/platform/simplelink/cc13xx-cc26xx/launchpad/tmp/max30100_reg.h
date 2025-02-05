#ifndef MAX30100_REG_H
#define MAX30100_REG_H

//definitions
#define MAX30100_ADDRESS 0x57
// Registers
#define MAX30100_SPC_SPO2_HI_RES_EN             (1 << 6)
typedef enum {
    MAX30100_INT_STATUS       =       0x00,  // Which interrupts are tripped
    MAX30100_INT_ENABLE       =       0x01,  // Which interrupts are active
    MAX30100_FIFO_WR_PTR      =       0x02,  // Where data is being written
    MAX30100_OVRFLOW_CTR      =       0x03,  // Number of lost samples
    MAX30100_FIFO_RD_PTR      =       0x04,  // Where to read from
    MAX30100_FIFO_DATA        =       0x05,  // Ouput data buffer
    MAX30100_MODE_CONFIG      =       0x06,  // Control register
    MAX30100_SPO2_CONFIG      =       0x07,  // Oximetry settings
    MAX30100_LED_CONFIG       =       0x09,  // Pulse width and power of LEDs
    MAX30100_TEMP_INTG        =       0x16,  // Temperature value, whole number
    MAX30100_TEMP_FRAC        =       0x17,  // Temperature value, fraction
    MAX30100_REV_ID           =       0xFE,  // Part revision
    MAX30100_PART_ID          =       0xFF   // Part ID, normally 0x11
}max30100_reg_t;


#define POR_PART_ID             0x11

#endif


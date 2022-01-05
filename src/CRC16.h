/**
 * @file CRC16.h
 * @author Ruben Neurink-Sluiman (ruben.neurink@gmail.com)
 * @brief CRC 16 calculation for the P1 telegram
 * @version 0.1
 * @date 2021-12-28
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef CRC16_H
#define CRC16_H

#include <Arduino.h>

#define POLYNOMIAL_IBM_REVERSED 0xA001 // See https://en.wikipedia.org/wiki/Cyclic_redundancy_check on the Reversed CRC-16-IBM

/***************** CRC16 calulation *****************/
uint16_t CRC16_IBM_REVERSED(char *buf, uint16_t len)
{
    uint16_t crc = 0x00;

    for (uint16_t pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];    // XOR byte into least sig. byte of crc

        for (int16_t i = 8; i != 0; i--) {    // Loop over each bit
            if ((crc & 0x0001) != 0) {      // If the LSB is set
                crc >>= 1;                    // Shift right and XOR 0xA001 
                crc ^= POLYNOMIAL_IBM_REVERSED;
            } else {                        // Else LSB is not set
                crc >>= 1;                    // Just shift right
            }
        }
    }

    return crc;
}

#endif // CRC16_H
/**
 * @file P1Meter.cpp
 * @author Ruben Neurink-Sluiman (ruben.neurink@gmail.com)
 * @brief P1 Meter class to retrieve and parse the P1 telegram
 * @version 0.1
 * @date 2021-12-28
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "P1MeterParser.h"
#include "CRC16.h"

/**
 * @brief Basic constructor with only a serial object. Make sure that the CTS pin of the P1 connection is pulled high
 * 
 * @param serial The Hardware serial port to which the P1 meter is attached to
 */
P1Meter::P1Meter(HardwareSerial *serial) {
    // Serial P1 connection
    mySerial = serial;

    // Allocate buffer
    char *newBuffer = (char *)realloc(buffer, BUFFER_SIZE + 1);
    buffer = newBuffer;

    memset(buffer, 0, BUFFER_SIZE);
    bufferIndex = 0;
}

/**
 * @brief Constructor with an option to pass the CTS pin. This pin will be set high to receive and pulled low to stop
 * 
 * @param serial The Hardware serial port to which the P1 meter is attached to
 * @param ctsPin The arduino pin which is connected to the CTS pin of the P1 Smart meter connection
 */
P1Meter::P1Meter(HardwareSerial *serial, uint8_t ctsPin) {
    this->ctsPin = ctsPin;
    
    // Serial P1 connection
    mySerial = serial;
    
    // Allocate buffer
    char *newBuffer = (char *)realloc(buffer, BUFFER_SIZE + 1);
    buffer = newBuffer;

    memset(buffer, 0, BUFFER_SIZE);
    bufferIndex = 0;
}

/**
 * @brief Receives the telegram. This function is non-blocking as long as there is no telegram send. When the telegram is send this function will block untill it is fully received
 * 
 */
void P1Meter::ReceiveTelegram() {
    if (!ctsHigh && ctsPin != 0xFF) {
        pinMode(ctsPin, OUTPUT);
        digitalWrite(ctsPin, HIGH); // Clear to send. Receive data from the P1 meter
        ctsHigh = true;
    }

    if (mySerial->available()) {
        uint8_t data = mySerial->read();
        if (data != '/') { // Start of the telegram
            return;
        }

        bufferIndex = 0;
        buffer[bufferIndex] = data;
        bufferIndex++;

        while (!DataReady) {
            if (mySerial->available()) {
                buffer[bufferIndex] = mySerial->read();
                
                if (buffer[bufferIndex - 6] == '!') { // End of telegram with CRC and \r\n after it
                    DataReady = true;
                    if (ctsPin != 0xFF) {
                        // Setting CTS low is against the P1 standard. It should be set to high impedance / pinmode input could do it
                        pinMode(ctsPin, INPUT); // Pause the telegram sending to be sure it won't mess up de rx buffer
                        ctsHigh = false;
                    }
                } else {
                    bufferIndex++;
                }
            }
        }
    }
}

/**
 * @brief Parses the telegram and provides its data in an easily accessible struct @see P1Data
 * 
 * @return P1Data The parsed P1 telegram data
 */
P1Data P1Meter::ProcessTelegram() {
    // Check the number of MBus devices attached
    data.NumberOfMBusDevices = strtoul(&buffer[lastIndexOf('-', 0) + 1], NULL, 10);
    
    int16_t startOfLine = 0, endOfLine = 0;
    startOfLine = indexOf('/', 0); // Find the first character of the telegram
    endOfLine = indexOf('\n', 0);
    // Fill in the header info
    data.HeaderInfo = getSubString(startOfLine + 1, endOfLine);

    char valueBuffer[10];

    // Parse the telegram
    while (endOfLine < bufferIndex && endOfLine != -1) {
        startOfLine = endOfLine + 1;
        endOfLine = indexOf('\n', startOfLine + 1);

        memset(valueBuffer, 0, 10);

        // Default OBIS codes
        if (startsWith(OBIS_VERSION, startOfLine)) {
            data.P1Version = strtoul(&buffer[indexOf('(', startOfLine) + 1], NULL, 10);

        } else if (startsWith(OBIS_DATETIME, startOfLine)) {
            strncpy(data.DateTime, buffer + indexOf('(', startOfLine) + 1, 13);

        } else if (startsWith(OBIS_EQUIPMENTID, startOfLine)) {
            data.EquipmentID = getSubString(indexOf('(', startOfLine) + 1, endOfLine - 2);

        } else if (startsWith(OBIS_TARIFF1_DELIVERED, startOfLine)) {
            strncpy(valueBuffer, buffer + indexOf('(', startOfLine) + 1, 6);
            strncpy(&valueBuffer[6], buffer + indexOf('.', startOfLine + 10) + 1, 3);
            data.DeliveredTariff1 = strtoul(valueBuffer, NULL, 10);

        } else if (startsWith(OBIS_TARIFF2_DELIVERED, startOfLine)) {
            strncpy(valueBuffer, buffer + indexOf('(', startOfLine) + 1, 6);
            strncpy(&valueBuffer[6], buffer + indexOf('.', startOfLine + 10) + 1, 3);
            data.DeliveredTariff2 = strtoul(valueBuffer, NULL, 10);

        } else if (startsWith(OBIS_TARIFF1_PRODUCED, startOfLine)) {
            strncpy(valueBuffer, buffer + indexOf('(', startOfLine) + 1, 6);
            strncpy(&valueBuffer[6], buffer + indexOf('.', startOfLine + 10) + 1, 3);
            data.ProducedTariff1 = strtoul(valueBuffer, NULL, 10);

        } else if (startsWith(OBIS_TARIFF2_PRODUCED, startOfLine)) {
            strncpy(valueBuffer, buffer + indexOf('(', startOfLine) + 1, 6);
            strncpy(&valueBuffer[6], buffer + indexOf('.', startOfLine + 10) + 1, 3);
            data.ProducedTariff2 = strtoul(valueBuffer, NULL, 10);

        } else if (startsWith(OBIS_TARIFF_INDICATOR, startOfLine)) {
            data.CurrentTariff = strtoul(&buffer[indexOf('(', startOfLine) + 1], NULL, 10);

        } else if (startsWith(OBIS_ACTUAL_DELIVERED, startOfLine)) {
            strncpy(valueBuffer, buffer + indexOf('(', startOfLine) + 1, 2);
            strncpy(&valueBuffer[2], buffer + indexOf('.', startOfLine + 10) + 1, 3);
            data.ActualDelivered = strtoul(valueBuffer, NULL, 10);

        } else if (startsWith(OBIS_ACTUAL_PRODUCED, startOfLine)) {
            strncpy(valueBuffer, buffer + indexOf('(', startOfLine) + 1, 2);
            strncpy(&valueBuffer[2], buffer + indexOf('.', startOfLine + 10) + 1, 3);
            data.ActualProduced = strtoul(valueBuffer, NULL, 10);

        } else if (startsWith(OBIS_NUMBER_POWER_FAIL, startOfLine)) {
            data.PowerFailures = strtoul(&buffer[indexOf('(', startOfLine) + 1], NULL, 10);

        } else if (startsWith(OBIS_LONG_POWER_FAIL, startOfLine)) {
            data.LongPowerFailures = strtoul(&buffer[indexOf('(', startOfLine) + 1], NULL, 10);

        } else if (startsWith(OBIS_POWER_LOG, startOfLine)) {
            uint16_t index1 = indexOf('(', startOfLine) + 1;
            uint16_t index2 = index1 + 3;
            uint8_t numberOfLogs = strtoul(&buffer[index1], NULL, 10);
            
            for (uint8_t i = 0; i < numberOfLogs; i++) {
                index1 = indexOf('(', index2 + 1);
                strncpy(data.PowerFailureLogs[i].DateTime, buffer + index1 + 1, 13);
                index2 = indexOf('(', index1 + 1);
                data.PowerFailureLogs[i].Duration = strtod(buffer + index2 + 1, NULL);
            }

        } else if (startsWith(OBIS_NUM_VOLTAGE_SAG_L1, startOfLine)) {
            data.VoltageSags[0] = strtoul(&buffer[indexOf('(', startOfLine) + 1], NULL, 10);

        } else if (startsWith(OBIS_NUM_VOLTAGE_SAG_L2, startOfLine)) {
            data.VoltageSags[1] = strtoul(&buffer[indexOf('(', startOfLine) + 1], NULL, 10);
        
        } else if (startsWith(OBIS_NUM_VOLTAGE_SAG_L3, startOfLine)) {
            data.VoltageSags[2] = strtoul(&buffer[indexOf('(', startOfLine) + 1], NULL, 10);

        } else if (startsWith(OBIS_NUM_VOLTAGE_SWL_L1, startOfLine)) {
            data.VoltageSwells[0] = strtoul(&buffer[indexOf('(', startOfLine) + 1], NULL, 10);

        } else if (startsWith(OBIS_NUM_VOLTAGE_SWL_L2, startOfLine)) {
            data.VoltageSwells[1] = strtoul(&buffer[indexOf('(', startOfLine) + 1], NULL, 10);

        } else if (startsWith(OBIS_NUM_VOLTAGE_SWL_L3, startOfLine)) {
            data.VoltageSwells[2] = strtoul(&buffer[indexOf('(', startOfLine) + 1], NULL, 10);

        } else if (startsWith(OBIS_TEXT_MESSAGE, startOfLine)) {
            data.TextMessage = getSubString(indexOf('(', startOfLine) + 1, endOfLine - 2);

        } else if (startsWith(OBIS_VOLTAGE_L1, startOfLine)) {
            strncpy(valueBuffer, buffer + indexOf('(', startOfLine) + 1, 3);
            strncpy(&valueBuffer[3], buffer + indexOf('.', startOfLine + 10) + 1, 1);
            data.Voltage[0] = strtoul(valueBuffer, NULL, 10);

        } else if (startsWith(OBIS_VOLTAGE_L2, startOfLine)) {
            strncpy(valueBuffer, buffer + indexOf('(', startOfLine) + 1, 3);
            strncpy(&valueBuffer[3], buffer + indexOf('.', startOfLine + 10) + 1, 1);
            data.Voltage[1] = strtoul(valueBuffer, NULL, 10);

        } else if (startsWith(OBIS_VOLTAGE_L3, startOfLine)) {
            strncpy(valueBuffer, buffer + indexOf('(', startOfLine) + 1, 3);
            strncpy(&valueBuffer[3], buffer + indexOf('.', startOfLine + 10) + 1, 1);
            data.Voltage[2] = strtoul(valueBuffer, NULL, 10);

        } else if (startsWith(OBIS_CURRENT_L1, startOfLine)) {
            data.Current[0] = strtoul(&buffer[indexOf('(', startOfLine) + 1], NULL, 10);

        } else if (startsWith(OBIS_CURRENT_L2, startOfLine)) {
            data.Current[1] = strtoul(&buffer[indexOf('(', startOfLine) + 1], NULL, 10);

        } else if (startsWith(OBIS_CURRENT_L3, startOfLine)) {
            data.Current[2] = strtoul(&buffer[indexOf('(', startOfLine) + 1], NULL, 10);

        } else if (startsWith(OBIS_POWER_POS_L1, startOfLine)) {
            strncpy(valueBuffer, buffer + indexOf('(', startOfLine) + 1, 2);
            strncpy(&valueBuffer[2], buffer + indexOf('.', startOfLine + 10) + 1, 3);
            data.PowerDelivered[0] = strtoul(valueBuffer, NULL, 10);

        } else if (startsWith(OBIS_POWER_POS_L2, startOfLine)) {
            strncpy(valueBuffer, buffer + indexOf('(', startOfLine) + 1, 2);
            strncpy(&valueBuffer[2], buffer + indexOf('.', startOfLine + 10) + 1, 3);
            data.PowerDelivered[1] = strtoul(valueBuffer, NULL, 10);

        } else if (startsWith(OBIS_POWER_POS_L3, startOfLine)) {
            strncpy(valueBuffer, buffer + indexOf('(', startOfLine) + 1, 2);
            strncpy(&valueBuffer[2], buffer + indexOf('.', startOfLine + 10) + 1, 3);
            data.PowerDelivered[2] = strtoul(valueBuffer, NULL, 10);

        } else if (startsWith(OBIS_POWER_POS_L1, startOfLine)) {
            strncpy(valueBuffer, buffer + indexOf('(', startOfLine) + 1, 2);
            strncpy(&valueBuffer[2], buffer + indexOf('.', startOfLine + 10) + 1, 3);
            data.PowerProduced[0] = strtoul(valueBuffer, NULL, 10);

        } else if (startsWith(OBIS_POWER_POS_L2, startOfLine)) {
            strncpy(valueBuffer, buffer + indexOf('(', startOfLine) + 1, 2);
            strncpy(&valueBuffer[2], buffer + indexOf('.', startOfLine + 10) + 1, 3);
            data.PowerProduced[1] = strtoul(valueBuffer, NULL, 10);

        } else if (startsWith(OBIS_POWER_POS_L3, startOfLine)) {
            strncpy(valueBuffer, buffer + indexOf('(', startOfLine) + 1, 2);
            strncpy(&valueBuffer[2], buffer + indexOf('.', startOfLine + 10) + 1, 3);
            data.PowerProduced[2] = strtoul(valueBuffer, NULL, 10);

        } else if (startsWith(OBIS_DEVICE_TYPE, startOfLine + 3)) {
            int8_t currentDeviceNumber = strtoul(&buffer[indexOf('-', startOfLine) + 1], NULL, 10);
            data.MBusDevices[currentDeviceNumber - 1].DeviceType = (EMBusDeviceType)strtoul(&buffer[indexOf('(', startOfLine) + 1], NULL, 10);

        } else if (startsWith(OBIS_EQUIPMENT_IDENT, startOfLine + 3)) {
            int8_t currentDeviceNumber = strtoul(&buffer[indexOf('-', startOfLine) + 1], NULL, 10);
            data.MBusDevices[currentDeviceNumber - 1].EquipmentID = getSubString(indexOf('(', startOfLine) + 1, endOfLine -2);

        } else if (startsWith(OBIS_DEVICE_VALUE, startOfLine + 3)) {
            int8_t currentDeviceNumber = strtoul(&buffer[indexOf('-', startOfLine) + 1], NULL, 10);

            int16_t valueIndex = indexOf('(', startOfLine);
            strncpy(data.MBusDevices[currentDeviceNumber - 1].Reading.DateTime, buffer + valueIndex + 1, 13);
            
            valueIndex = indexOf('(', valueIndex + 1);
            strncpy(valueBuffer, buffer + valueIndex + 1, 5);
            strncpy(&valueBuffer[5], buffer + indexOf('.', valueIndex + 1) + 1, 3);
            data.MBusDevices[currentDeviceNumber - 1].Reading.Value = strtoul(valueBuffer, NULL, 10);
            
            valueIndex = indexOf('*', valueIndex + 1);
            strncpy(data.MBusDevices[currentDeviceNumber - 1].Reading.Unit, buffer + valueIndex + 1, indexOf(')', valueIndex) - valueIndex - 1);
        }
    }

    // Calculate CRC
    uint16_t crcIndex = indexOf('!', 0);
    uint16_t calculatedCRC = calcCRC16(buffer, crcIndex + 1);

    // Check against message CRC
    char messageCRC[5];
    strncpy(messageCRC, buffer + crcIndex + 1, 4); // Copy message CRC
    messageCRC[4] = 0; // 0 terminate the string
    data.CRC = strtoul(messageCRC, NULL, 16);
    data.ValidCRC = (data.CRC == calculatedCRC); // Convert message CRC from ascii to hex and check against calculated CRC

    // Clear buffer
    memset(buffer, 0, BUFFER_SIZE);
    DataReady = false;

    return data;
}

/**
 * @brief Gets a pointer to the telegram buffer. Only mess with this if you know what you're doing
 * 
 * @return char* a pointer to the telegram buffer.
 */
char *P1Meter::GetBuffer() {
    return buffer;
}

/**
 * @brief Gets the buffer length.
 * 
 * @return int16_t The buffer length. 0 if the buffer has been cleared by @see ProcessTelegram
 */
int16_t P1Meter::GetBufferLength() {
    return bufferIndex;
}


/***************** Helper functions *****************/

uint16_t P1Meter::calcCRC16(char *buffer, uint16_t len) {
    return CRC16_IBM_REVERSED(buffer, len);
}

int16_t P1Meter::indexOf(uint8_t character, int16_t startIndex) {
    const char* temp = strchr(buffer + startIndex, character);
    if (temp == NULL) return -1;
    return temp - buffer;
}

int16_t P1Meter::lastIndexOf(uint8_t character, uint16_t fromIndex) {
    char* temp = strrchr(buffer + fromIndex, character);
    if (temp == NULL) return -1;
    return temp - buffer;
}

uint8_t P1Meter::startsWith(const char *findString, uint16_t offset) {
    uint16_t stringLength = strlen(findString);
    return strncmp(&buffer[offset], findString, stringLength) == 0;
}

uint8_t P1Meter::endsWith(const char *findString) {
  uint16_t dataLength = strlen(buffer);
  uint16_t stringLength = strlen(findString);
	return strcmp(&buffer[dataLength - stringLength], findString) == 0;
}

String P1Meter::getSubString(uint16_t startIndex, uint16_t endIndex) {
  String output;
  char temp = buffer[endIndex];
  buffer[endIndex] = '\0';
  output = buffer + startIndex;
  buffer[endIndex] = temp;
  return output;
}
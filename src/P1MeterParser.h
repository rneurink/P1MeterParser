/**
 * @file P1Meter.h
 * @author Ruben Neurink-Sluiman (ruben.neurink@gmail.com)
 * @brief P1 Meter class to retrieve and parse the P1 telegram 
 * @version 0.1
 * @date 2021-12-28
 * 
 * @copyright Copyright (c) 2021
 * 
 * @note Some info https://www.netbeheernederland.nl/dossiers/slimme-meter-15 http://domoticx.com/p1-poort-slimme-meter-hardware/  
 */

#ifndef P1METER_H
#define P1METER_H

#include <Arduino.h>

// OBIS codes for the master device
#define OBIS_VERSION            "1-3:0.2.8" // Version information for P1 output
#define OBIS_DATETIME           "0-0:1.0.0" // Date-time stamp of the P1 message
#define OBIS_EQUIPMENTID        "0-0:96.1.1"// Equipment identifier
#define OBIS_TARIFF1_DELIVERED  "1-0:1.8.1" // Meter Reading electricity delivered to client (Tariff 1) in 0,001 kWh
#define OBIS_TARIFF2_DELIVERED  "1-0:1.8.2" // Meter Reading electricity delivered to client (Tariff 2) in 0,001 kWh
#define OBIS_TARIFF1_PRODUCED   "1-0:2.8.1" // Meter Reading electricity delivered by client (Tariff 1) in 0,001 kWh
#define OBIS_TARIFF2_PRODUCED   "1-0:2.8.2" // Meter Reading electricity delivered by client (Tariff 2) in 0,001 kWh
#define OBIS_TARIFF_INDICATOR   "0-0:96.14.0"// Tariff indicator electricity. The tariff indicator can also be used to switch tariff dependent loads e.g boilers. This is the responsibility of the P1 user
#define OBIS_ACTUAL_DELIVERED   "1-0:1.7.0" // Actual electricity power delivered (+P) in 1 Watt resolution
#define OBIS_ACTUAL_PRODUCED    "1-0:2.7.0" // Actual electricity power received (-P) in 1 Watt resolution
#define OBIS_NUMBER_POWER_FAIL  "0-0:96.7.21"// Number of power failures in any phase
#define OBIS_LONG_POWER_FAIL    "0-0:96.7.9" // Number of long power failures in any phase
#define OBIS_POWER_LOG          "1-0:99.97.0"// Power Failure Event Log (long power failures) 
#define OBIS_POWER_LOG_ITEM     "0-0:96.7.19"// Power Failure Event log item
#define OBIS_NUM_VOLTAGE_SAG_L1 "1-0:32.32.0"// Number of voltage sags in phase L1
#define OBIS_NUM_VOLTAGE_SAG_L2 "1-0:52.32.0"// Number of voltage sags in phase L2
#define OBIS_NUM_VOLTAGE_SAG_L3 "1-0:72.32.0"// Number of voltage sags in phase L3
#define OBIS_NUM_VOLTAGE_SWL_L1 "1-0:32.36.0"// Number of voltage swells in phase L1
#define OBIS_NUM_VOLTAGE_SWL_L2 "1-0:52.36.0"// Number of voltage swells in phase L2
#define OBIS_NUM_VOLTAGE_SWL_L3 "1-0:72.36.0"// Number of voltage swells in phase L3
#define OBIS_TEXT_MESSAGE       "0-0:96.13.0"// Text message max 1024 characters.
#define OBIS_VOLTAGE_L1         "1-0:32.7.0" // Instantaneous voltage L1 in V resolution
#define OBIS_VOLTAGE_L2         "1-0:52.7.0" // Instantaneous voltage L2 in V resolution
#define OBIS_VOLTAGE_L3         "1-0:72.7.0" // Instantaneous voltage L3 in V resolution
#define OBIS_CURRENT_L1         "1-0:31.7.0" // Instantaneous current L1 in A resolution
#define OBIS_CURRENT_L2         "1-0:51.7.0" // Instantaneous current L2 in A resolution
#define OBIS_CURRENT_L3         "1-0:71.7.0" // Instantaneous current L3 in A resolution
#define OBIS_POWER_POS_L1       "1-0:21.7.0" // Instantaneous active power L1 (+P) in W resolution
#define OBIS_POWER_POS_L2       "1-0:41.7.0" // Instantaneous active power L2 (+P) in W resolution
#define OBIS_POWER_POS_L3       "1-0:61.7.0" // Instantaneous active power L3 (+P) in W resolution
#define OBIS_POWER_NEG_L1       "1-0:22.7.0" // Instantaneous active power L1 (-P) in W resolution
#define OBIS_POWER_NEG_L2       "1-0:42.7.0" // Instantaneous active power L2 (-P) in W resolution
#define OBIS_POWER_NEG_L3       "1-0:62.7.0" // Instantaneous active power L3 (-P) in W resolution

// OBIS codes sub devices
#define OBIS_DEVICE_TYPE        ":24.1.0" // Device-Type
#define OBIS_EQUIPMENT_IDENT    ":96.1.0" // Equipment identifier (Thermal:  Heat or Cold) (Water) (Gas)
#define OBIS_DEVICE_VALUE       ":24.2.1" // Last 5-minute Meter value. Consists of a timestamp and value

// OBIS device types
#define OBIS_DEV_TYPE_GAS       3 // Gas meter
#define OBIS_DEV_TYPE_THERMAL   4 // Thermal meter (Heat / Cold) i.e. cityheat
#define OBIS_DEV_TYPE_WATER     255 // TODO: Find out

// Check if buffer is big enough if you have multiple meters attached to the P1 meter system (like gas and water)
// Max data as specified in the P1 5.0.2 standard chapter 6.2 states it can contain up to 1024 characters
#define BUFFER_SIZE 1024

enum EMBusDeviceType {
    Gas = OBIS_DEV_TYPE_GAS,
    Thermal = OBIS_DEV_TYPE_THERMAL,
    Water = OBIS_DEV_TYPE_WATER
};

struct PowerFailureLogStruct {
    char DateTime[14];
    double Duration;
};

struct MBusReading {
    char DateTime[14];
    uint32_t Value;
    char Unit[4];
};

struct MBusDevice {
    EMBusDeviceType DeviceType;
    String EquipmentID;
    MBusReading Reading;
};

/**
 * @brief Full P1 data object fully parsed from the telegram
 * @note Made according to the Dutch Smart Meter Requirements (DSMR) 5.0.2
 */
struct P1Data {
    String HeaderInfo;
    byte P1Version;
    char DateTime[14]; // YYMMDDhhmmssX where x is S or W for summer and winter time
    String EquipmentID;
    uint32_t DeliveredTariff1; // Low tariff in watts
    uint32_t DeliveredTariff2; // High tariff in watts
    uint32_t ProducedTariff1; // Low tariff in watts
    uint32_t ProducedTariff2; // High tariff in watts
    byte CurrentTariff; // Tariff indicator. 1 for Low tariff and 2 for High tariff
    uint32_t ActualDelivered; // In watts
    uint32_t ActualProduced; // In watts
    uint32_t PowerFailures;
    uint32_t LongPowerFailures;
    PowerFailureLogStruct PowerFailureLogs[3]; // TODO: find out needed buffer size
    uint32_t VoltageSags[3]; // Voltage sag buffer for all 3 phases L1 L2 L3
    uint32_t VoltageSwells[3]; // Voltage swell buffer for all 3 phases L1 L2 L3
    String TextMessage;
    uint32_t Voltage[3]; // Voltage buffer for all 3 phases in 100mV
    uint32_t Current[3]; // Current buffer for all 3 phases in A
    uint32_t PowerDelivered[3]; // +P Power buffer for all 3 phases in watts
    uint32_t PowerProduced[3]; // -P Power buffer for all 3 phases in watts
    MBusDevice MBusDevices[3]; // TODO: find out needed buffer size. Expecting a GAS, WATER and another electric meter should be enoug?
    uint16_t CRC;
    bool ValidCRC;
    byte NumberOfMBusDevices;
};

/**
 * @brief Class to parse and provide P1 Meter data as a simple struct object
 * 
 */
class P1Meter {
public:
    P1Meter(HardwareSerial *serial);
    P1Meter(HardwareSerial *serial, uint8_t ctsPin);

    /**
     * Basic functions
     */
    void ReceiveTelegram();
    P1Data ProcessTelegram();

    /**
     * Low level functions
     */
    char *GetBuffer();
    int16_t GetBufferLength();

    bool DataReady = false;

protected:

private:
    uint16_t calcCRC16(char *buffer, uint16_t len);
    int16_t indexOf(uint8_t character, int16_t startIndex);
    int16_t lastIndexOf(uint8_t character, uint16_t fromIndex);
    uint8_t startsWith(const char *findString, uint16_t offset);
    uint8_t endsWith(const char *findString);
    String getSubString(uint16_t startIndex, uint16_t endIndex);

    HardwareSerial *mySerial;

    char *buffer;
    int16_t bufferIndex = 0;
    P1Data data;

    uint8_t ctsPin = 0xFF;
    bool ctsHigh = false;
};

#endif // P1METER_H
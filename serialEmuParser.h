// #include "tekumaCommon.h"
// #include <stddef.h>
#include <stdint.h>
#define packetTypeOffset 32

#define bufferNumber 3
#define bufferLength 255 // one less than max size of uint8_t to allow for a possible combination where /r is not in the packet


// #define CONFIG_ZEPHYR

typedef struct UartData_Parser_t{
    union {
        struct {
            //deffines the type of packet being sent
            char    type;

            union{
                struct{
                    //offset used to shift the following bytes by this amount to prevent the packet from containing /r
                    uint8_t offset;
                    //additive checksum used as a quick was to validate or invalidate income and out going transmitions
                    uint8_t checksum;
                    //data of this packet
                    uint8_t packetData[bufferLength - 3];
                }packetWithData;
                struct{
                    //data of this packet
                    uint8_t packetData;
                }packetWithOneData;
            };
        };
        //raw data being used 
        uint8_t rawData[bufferLength];
    };
} UartData_Parser_t;


typedef struct transmissionData_t{
    UartData_Parser_t packet;
    uint8_t length;
} transmissionData_t;

int encodeSerailEmuMessage(char type, void* data, uint8_t size, transmissionData_t* message);

transmissionData_t decodeSerailEmuMessage(transmissionData_t* message);

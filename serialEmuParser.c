#include "serialEmuParser.h"

#ifdef CONFIG_ZEPHYR 

#include <zephyr/logging/log.h>

#ifndef CONFIG_EMU_SERIAL_LOG_LEVEL 
#define CONFIG_EMU_SERIAL_LOG_LEVEL 4
#endif

LOG_MODULE_REGISTER(EMU_SERIAL, CONFIG_EMU_SERIAL_LOG_LEVEL);
#endif

typedef enum {
    notReady = -3,
    notSupported = -2,
    unknownError = -1,
    noError = 0,
}tekumaErrors_t;

int encodeSerailEmuMessage(char type, void* data, uint8_t size, transmissionData_t* message){


    if (type == '\r'){
#ifdef CONFIG_ZEPHYR 
        LOG_ERR("packetType cannot be a carrage return\r\n");
#endif
        return unknownError;
    }

    if (size >= (bufferLength - 3)){
#ifdef CONFIG_ZEPHYR 
        LOG_ERR("packet length is too long for transmittion\r\n");
#endif
        return unknownError;
    }

    const uint8_t* tempData = data;
    uint8_t transmitLength = 0;

    UartData_Parser_t packetToSendExtern;

    packetToSendExtern.type = type;
    packetToSendExtern.packetWithData.checksum = 0;

    if (size){

        uint8_t offset = 0;
        uint8_t lastOffset = 0;

        for (uint8_t i = 0; i < size; i++){
            packetToSendExtern.packetWithData.checksum += tempData[i];
        }

        for (uint8_t i = 0; i < size; i++){

            if (packetToSendExtern.packetWithData.checksum + offset == '\r'){
                offset++;
            } else{
                for (uint8_t j = 0; j < size;j++){
                    //if a carriage return is found, skip the reset of the values
                    if ((tempData[j] + offset) == '\r'){
                        offset++;
                        j = size;
                    }
                }
            }
            /*if the offset value doesn't change then we have a valid offset value
            * and we can skip the rest of the checks.*/
            if (offset == lastOffset){
                i = size;
            }
        }
        //Offset the data array
        for (uint8_t i = 0; i < size; i++){
            packetToSendExtern.packetWithData.packetData[i] = (tempData[i] + offset) & 0xFF;
        }
        packetToSendExtern.packetWithData.checksum += offset;

        //load the offset value to the data array,
        //This is now ready to send
        packetToSendExtern.packetWithData.offset = offset;
        //add the offset but to the size and the mode bit
        transmitLength = size + 3;


    } else{
        transmitLength = 1;
    }

    // packetToSendExtern.rawData[transmitLength++] = '\r'; 

    message->packet = packetToSendExtern;
    message->length = transmitLength;
    return noError;
}

transmissionData_t decodeSerailEmuMessage(transmissionData_t* incomingMessage){

    // if (incomingMessage->packet.rawData == NULL) {
    //     LOG_ERR("No data");
    // } // changed to fix warning. never null
    if (!incomingMessage){
#ifdef CONFIG_ZEPHYR 
        LOG_ERR("Null Pointer");
#endif
        transmissionData_t nullData = {
        .length = 0,
        // .packet = 0              //  was this
        .packet = {{{0}}}        //  compiler says this
        };

        return nullData;
    }
    UartData_Parser_t received = incomingMessage->packet;


    uint8_t size = incomingMessage->length;

    // if (received.type != 'h' && received.type != 'i'){
    //     LOG_WRN("here %c", received.type);
    // }

    //process packets that carry data
    if (size > 3){
        received.packetWithData.checksum -= received.packetWithData.offset;

        uint8_t checksumValidation = 0;

        for (int i = 0; i < size - 3; i++){
            received.packetWithData.packetData[i] -= received.packetWithData.offset;
            checksumValidation += received.packetWithData.packetData[i];

        }
        //if the checksum does not equal, we have an error do not process
        //we can do this by chaning the type out side the accepted range
        if (received.packetWithData.checksum != checksumValidation){
#ifdef CONFIG_ZEPHYR 
            LOG_ERR("checksum invalid : %d : %d : %d : %d : %c\n", received.packetWithData.checksum, checksumValidation, received.packetWithData.offset, size, received.type);
#endif
            received.type = 0;
        }

    } else if (size > 1){
#ifdef CONFIG_ZEPHYR 
        LOG_ERR("Invalid size of packet, size received: %d", size);
#endif
        received.type = 0;
    }


    if (received.type >= packetTypeOffset && received.type < 127){
        //valid packet received
        transmissionData_t newData = { .packet = received, .length = (size < 3 ? size - 1 : size - 3) };
        return newData;

    }

    transmissionData_t nullData = {
        .length = 0,
        // .packet = 0              //  was this
        .packet = {{{0}}}        //  compiler says this
    };

    return nullData;
}
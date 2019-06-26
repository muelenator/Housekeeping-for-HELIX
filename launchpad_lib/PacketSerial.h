#pragma once
/* Max data length is: 
 *	-- +4 header bytes
 *	-- +255 data bytes
 *  -- +1 CRC (checksum) byte
 * For max packet size, 
 *	-- +1 COBS overhead
 *	-- +1 COBS packet marker
 */
#define MAX_PACKET_LENGTH (4 + 255 + 1) + 2

#include <Arduino.h>
#include "COBS_encoding.h"

class PacketSerial_
{
public:

    typedef void (*PacketHandlerFunction)(const uint8_t* buffer, size_t size);


    typedef void (*PacketHandlerFunctionWithSender)(const void* sender, const uint8_t* buffer, size_t size);

    /// \brief Construct a default PacketSerial_ device.
    PacketSerial_():
        _receiveBufferIndex(0),
        _stream(nullptr),
        _onPacketFunction(nullptr),
        _onPacketFunctionWithSender(nullptr)
    {
    }

    /// \brief Destroy the PacketSerial_ device.
    ~PacketSerial_()
    {
    }


    void begin(unsigned long speed)
    {
        Serial.begin(speed);
        #if ARDUINO >= 100 && !defined(CORE_TEENSY)
        while (!Serial) {;}
        #endif
        setStream(&Serial);
    }


    void begin(unsigned long speed, size_t port) __attribute__ ((deprecated))
    {
        switch(port)
        {
        #if defined(UBRR1H)
            case 1:
                Serial1.begin(speed);
                #if ARDUINO >= 100 && !defined(CORE_TEENSY)
                while (!Serial1) {;}
                #endif
                setStream(&Serial1);
                break;
        #endif
        #if defined(UBRR2H)
            case 2:
                Serial2.begin(speed);
                #if ARDUINO >= 100 && !defined(CORE_TEENSY)
                while (!Serial1) {;}
                #endif
                setStream(&Serial2);
                break;
        #endif
        #if defined(UBRR3H)
            case 3:
                Serial3.begin(speed);
                #if ARDUINO >= 100 && !defined(CORE_TEENSY)
                while (!Serial3) {;}
                #endif
                setStream(&Serial3);
                break;
        #endif
            default:
                begin(speed);
        }
    }


    void begin(Stream* stream) __attribute__ ((deprecated))
    {
        _stream = stream;
    }

    void setStream(Stream* stream)
    {
        _stream = stream;
    }

    void update()
    {
        if (_stream == nullptr) return;

        while (_stream->available() > 0)
        {
            uint8_t data = _stream->read();
			
            if (data == PACKETMARKER)
            {
                if (_onPacketFunction || _onPacketFunctionWithSender)
                {
                    uint8_t _decodeBuffer[_receiveBufferIndex];

                    size_t numDecoded = COBS::decode(_receiveBuffer,
                                                            _receiveBufferIndex,
                                                            _decodeBuffer);

                    if (_onPacketFunction)
                    {
                        _onPacketFunction(_decodeBuffer, numDecoded);
                    }
                    else if (_onPacketFunctionWithSender)
                    {
                        _onPacketFunctionWithSender(this, _decodeBuffer, numDecoded);
                    }
                }

                _receiveBufferIndex = 0;
            }
            else
            {
                if ((_receiveBufferIndex + 1) < MAX_PACKET_LENGTH)
                {
                    _receiveBuffer[_receiveBufferIndex++] = data;
                }
                else
                {
                    // Error, buffer overflow if we write.
                }
            }
        }
    }

    void send(const uint8_t* buffer, size_t size) const
    {
        if(_stream == nullptr || buffer == nullptr || size == 0) return;

        uint8_t _encodeBuffer[COBS::getEncodedBufferSize(size)];

        size_t numEncoded = COBS::encode(buffer,
                                                size,
                                                _encodeBuffer);

        _stream->write(_encodeBuffer, numEncoded);
        _stream->write((uint8_t) PACKETMARKER);
    }

    void setPacketHandler(PacketHandlerFunction onPacketFunction)
    {
        _onPacketFunction = onPacketFunction;
        _onPacketFunctionWithSender = nullptr;
    }

    void setPacketHandler(PacketHandlerFunctionWithSender onPacketFunctionWithSender)
    {
        _onPacketFunction = nullptr;
        _onPacketFunctionWithSender = onPacketFunctionWithSender;
    }

private:
    PacketSerial_(const PacketSerial_&);
    PacketSerial_& operator = (const PacketSerial_&);

    uint8_t _receiveBuffer[MAX_PACKET_LENGTH];
    size_t _receiveBufferIndex = 0;

    Stream* _stream = nullptr;

    PacketHandlerFunction _onPacketFunction = nullptr;
    PacketHandlerFunctionWithSender _onPacketFunctionWithSender = nullptr;
};


/// \brief A typedef for the default COBS PacketSerial class.
typedef PacketSerial_ PacketSerial;

#ifndef RECEIVER_H
#define RECEIVER_H

#include <stdint.h>

enum {
  MAX_PAYLOAD_SIZE = 80,
  MIN_PACKAGE_SIZE = 4,
  MAX_PACKAGE_SIZE = MAX_PAYLOAD_SIZE + MIN_PACKAGE_SIZE,
  MAX_SENDER_ID = 31
};

class RFReceiver {
    const uint8_t inputPin;
    const uint32_t pulseLimit;

    // Input buffer and input state
    uint8_t shiftByte;
    uint8_t errorCorBuf[3];
    uint8_t bitCount, byteCount, errorCorBufCount;
    uint32_t lastTimestamp;
    bool packageStarted;

    uint8_t inputBuf[MAX_PACKAGE_SIZE];
    uint8_t inputBufLen;
    uint16_t checksum;
    volatile bool inputBufReady;
    uint8_t changeCount;

    // Used to filter out duplicate packages
    uint8_t prevPackageIds[MAX_SENDER_ID + 1];

    uint8_t recvDataRaw(uint8_t * data);

  public:
    RFReceiver(uint8_t inputPin, uint16_t pulseLength = 100) : inputPin(inputPin),
        pulseLimit((pulseLength << 2) - (pulseLength >> 1)), shiftByte(0),
        bitCount(0), byteCount(0), errorCorBufCount(0), lastTimestamp(0),
        packageStarted(false), inputBufLen(0), checksum(0),
        inputBufReady(false), changeCount(0) {

    }

    int begin(uint8_t gpio);

    void stop();

    /**
     * Returns true if a valid and deduplicated package is in the buffer, so
     * that a subsequent call to recvPackage() will not block.
     *
     * @returns True if recvPackage() will not block
     */
    bool ready() const {
      return inputBufReady;
    }

    uint8_t recvPackage(uint8_t * data, uint8_t *pSenderId = 0, uint8_t *pPackageId = 0);

    void decodeByte(uint8_t inputByte);


    static RFReceiver* instance;
    void handleGpio(int gpio, int level, uint32_t tick);

    static void staticHandleGpio(int gpio, int level, uint32_t tick) {
      instance->handleGpio(gpio, level, tick);
    }

};

#endif  /* RECEIVER_H */

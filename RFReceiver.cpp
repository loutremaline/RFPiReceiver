#include "RFReceiver.h"

#include <cstring>
#include "pigpio.h"


static inline uint16_t crc_update(uint16_t crc, uint8_t data) {
    // Source: http://www.atmel.com/webdoc/AVRLibcReferenceManual/group__util__crc_1ga1c1d3ad875310cbc58000e24d981ad20.html
    data ^= crc & 0xFF;
    data ^= data << 4;

    return ((((uint16_t)data << 8) | (crc >> 8)) ^ (uint8_t)(data >> 4) ^ ((uint16_t)data << 3));
}

static inline uint8_t recoverByte(const uint8_t b1, const uint8_t b2, const uint8_t b3) {
  // Discard all bits that occur only once in the three input bytes
  // Use all bits that are in b1 and b2
  uint8_t res = b1 & b2;
  // Use all bits that are in b1 and b3
  res |= b1 & b3;
  // Use all bits that are in b2 and b3
  res |= b2 & b3;
  return res;
}

void RFReceiver::decodeByte(uint8_t inputByte) {
  if (!packageStarted)
    return;

  errorCorBuf[errorCorBufCount++] = inputByte;

  if (errorCorBufCount != 3)
    return;
  errorCorBufCount = 0;

  if (!byteCount) {
    // Quickly decide if this is really a package or not
    if (errorCorBuf[0] < MIN_PACKAGE_SIZE || errorCorBuf[0] > MAX_PACKAGE_SIZE ||
            errorCorBuf[0] != errorCorBuf[1] || errorCorBuf[0] != errorCorBuf[2]) {
      packageStarted = false;
      return;
    }

    inputBufLen = errorCorBuf[0];
    checksum = crc_update(checksum, inputBufLen);

  } else {
    uint8_t data = recoverByte(errorCorBuf[0], errorCorBuf[1], errorCorBuf[2]);
    inputBuf[byteCount - 1] = data;
    // Calculate the checksum on the fly
    checksum = crc_update(checksum, data);

    if (byteCount == inputBufLen) {
      uint8_t senderId = inputBuf[inputBufLen - 4];
      uint8_t packageId = inputBuf[inputBufLen - 3];

      // Ignore duplicate packages and check if the checksum is correct
      if (!checksum && senderId <= MAX_SENDER_ID && prevPackageIds[senderId] != packageId) {
        prevPackageIds[senderId] = packageId;
        inputBufReady = true;
      }

      packageStarted = false;
      return;
    }
  }

  ++byteCount;
}

void RFReceiver::handleGpio(int gpio, int level, uint32_t tick) {
  if (inputBufReady)
    return;

  ++changeCount;

  {
    if (tick - lastTimestamp < pulseLimit) {
      return;
    }

    lastTimestamp = tick;
  }

  shiftByte = (shiftByte >> 2) | ((changeCount - 1) << 6);
  changeCount = 0;

  if (packageStarted) {
    bitCount += 2;
    if (bitCount != 8)
      return;
    bitCount = 0;

    decodeByte(shiftByte);
  } else if (shiftByte == 0xE0) {
    // New package starts here
    bitCount = 0;
    byteCount = 0;
    errorCorBufCount = 0;
    inputBufLen = 0;
    checksum = 0xffff;
    packageStarted = true;
  }
}

uint8_t RFReceiver::recvDataRaw(uint8_t * data) {
  while (!inputBufReady);

  uint8_t len = inputBufLen;
  memcpy(data, inputBuf, len - 2);

  // Enable the input as fast as possible
  inputBufReady = false;
  // The last two bytes contain the checksum, which is no longer needed
  return len - 2;
}

uint8_t RFReceiver::recvPackage(uint8_t *data, uint8_t *pSenderId, uint8_t *pPackageId) {
  for (;;) {
    uint8_t len = recvDataRaw(data);
    uint8_t senderId = data[len - 2];
    uint8_t packageId = data[len - 1];

    if (pSenderId)
      *pSenderId = senderId;

    if (pPackageId)
      *pPackageId = packageId;

    // The last two bytes contain the sender and package ids
    return len - 2;
  }
}

RFReceiver* RFReceiver::instance = 0;

int RFReceiver::begin(uint8_t gpio) {

  instance = this;
  int returnCode = gpioInitialise();

  if (returnCode >= 0)
  {  
    gpioSetMode(gpio, PI_INPUT);  
    gpioSetAlertFunc(gpio, staticHandleGpio);
  }

  return returnCode;
}

void RFReceiver::stop() {
  gpioTerminate();
}


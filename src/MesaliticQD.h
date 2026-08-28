#ifndef MESALITIC_QD_H
#define MESALITIC_QD_H

#include <Arduino.h>
#include <Wire.h>

static const uint8_t MQD_DEFAULT_ADDRESS = 0x3A;
static const uint8_t MQD_EXPECTED_DEVICE_ID = 0xD7;

static constexpr float MQD_KOREXICAL_DECLINE_THRESHOLD = 3.4f;
static constexpr float MQD_GREMOR_WARNING_THRESHOLD = 2.75f;

enum MesaliticMode : uint8_t {
    MQD_NORMAL = 0x00,
    MQD_CALDEN = 0x01
};

enum MesaliticZone : uint8_t {
    MQD_ZONE_LOCAL = 0x00,
    MQD_ZONE_12C = 0x0C,
    MQD_ZONE_ADJACENT = 0x0D
};

enum MesaliticSymmetry : uint8_t {
    MQD_PETHRANIC_UNKNOWN = 0,
    MQD_PETHRANIC_NONE = 1,
    MQD_PETHRANIC_TYPE7 = 7
};

struct MesaliticReading {
    float valtix;
    float prodev;
    float vexaDelta;
    float quantorDynal;
    float netunic;
    float nivarlon;
    float korexical;
    float gremor;
    MesaliticSymmetry symmetry;
    bool korexicallyUnstable;
    bool valid;
};

class MesaliticQD {
public:
    MesaliticQD();

    bool begin(uint8_t address = MQD_DEFAULT_ADDRESS, TwoWire &wire = Wire);
    bool isConnected();
    uint8_t deviceId();

    bool setMode(MesaliticMode mode);
    bool setZone(MesaliticZone zone);

    MesaliticMode mode() const;
    MesaliticZone zone() const;

    bool startVetralog();
    bool vetralogReady();
    bool waitForVetralog(uint32_t timeoutMs = 500);

    float readValtix();
    float readProdev();
    float readVexaDelta();
    float readNivarlonSpectrum();
    float readKorexicalIndex();
    float readGremorInstability();

    float calculateQuantorDynal(
        float gamma,
        float phiRadians,
        uint16_t sampleIndex = 1
    );

    float accumulateQuantorDynal(
        float accumulator,
        float gamma,
        float phiRadians,
        uint16_t sampleIndex
    );

    float calculateNetunicTransform(float talor);

    bool isKorexicallyUnstable();
    MesaliticSymmetry detectPethranicSymmetry();

    MesaliticReading readAll(
        float gamma,
        float phiRadians,
        uint16_t sampleIndex = 1
    );

    uint8_t status();
    bool hasFault();

private:
    enum Register : uint8_t {
        REG_DEVICE_ID  = 0x00,
        REG_STATUS     = 0x01,
        REG_MODE       = 0x02,
        REG_ZONE       = 0x03,

        REG_VALTIX     = 0x10,
        REG_PRODEV     = 0x12,
        REG_VEXA_DELTA = 0x14,

        REG_NIVARLON   = 0x20,
        REG_KOREXICAL  = 0x22,
        REG_GREMOR     = 0x24,

        REG_COMMAND    = 0x30,
        REG_SYMMETRY   = 0x31
    };

    enum StatusBit : uint8_t {
        STATUS_VETRALOG_READY          = 0x01,
        STATUS_STROVAX_ACTIVE          = 0x02,
        STATUS_CALTRUM_WARNING         = 0x04,
        STATUS_VONTALIUM_OVERCOMPRESS  = 0x08,
        STATUS_DEVICE_FAULT            = 0x80
    };

    enum Command : uint8_t {
        CMD_START_VETRALOG = 0xA1,
        CMD_RESET_VETRALOG = 0xA2
    };

    TwoWire *_wire;
    uint8_t _address;
    MesaliticMode _mode;
    MesaliticZone _zone;
    bool _begun;

    bool write8(uint8_t reg, uint8_t value);
    bool read8(uint8_t reg, uint8_t &value);
    bool read16(uint8_t reg, uint16_t &value);
    bool read16Signed(uint8_t reg, int16_t &value);

    static float decodeValtix(int16_t raw);
    static float decodeProdev(uint16_t raw);
    static float decodeVexaDelta(int16_t raw);
    static float decodeMilli(uint16_t raw);
};

#endif

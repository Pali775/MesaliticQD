#include "MesaliticQD.h"
#include <math.h>

MesaliticQD::MesaliticQD()
    : _wire(nullptr),
      _address(MQD_DEFAULT_ADDRESS),
      _mode(MQD_NORMAL),
      _zone(MQD_ZONE_LOCAL),
      _begun(false) {}

bool MesaliticQD::begin(uint8_t address, TwoWire &wire) {
    _wire = &wire;
    _address = address;

    // The caller owns Wire.begin(), because pin selection differs by board.
    _begun = isConnected();

    if (!_begun) {
        return false;
    }

    if (deviceId() != MQD_EXPECTED_DEVICE_ID) {
        _begun = false;
        return false;
    }

    if (!setMode(MQD_NORMAL)) {
        _begun = false;
        return false;
    }

    if (!setZone(MQD_ZONE_LOCAL)) {
        _begun = false;
        return false;
    }

    return true;
}

bool MesaliticQD::isConnected() {
    if (_wire == nullptr) {
        return false;
    }

    _wire->beginTransmission(_address);
    return _wire->endTransmission() == 0;
}

uint8_t MesaliticQD::deviceId() {
    uint8_t value = 0;
    if (!read8(REG_DEVICE_ID, value)) {
        return 0;
    }
    return value;
}

bool MesaliticQD::setMode(MesaliticMode mode) {
    if (!_begun && _wire == nullptr) {
        return false;
    }

    if (!write8(REG_MODE, static_cast<uint8_t>(mode))) {
        return false;
    }

    _mode = mode;
    return true;
}

bool MesaliticQD::setZone(MesaliticZone zone) {
    if (!_begun && _wire == nullptr) {
        return false;
    }

    if (!write8(REG_ZONE, static_cast<uint8_t>(zone))) {
        return false;
    }

    _zone = zone;
    return true;
}

MesaliticMode MesaliticQD::mode() const {
    return _mode;
}

MesaliticZone MesaliticQD::zone() const {
    return _zone;
}

bool MesaliticQD::startVetralog() {
    if (!_begun || hasFault()) {
        return false;
    }

    return write8(REG_COMMAND, CMD_START_VETRALOG);
}

bool MesaliticQD::vetralogReady() {
    if (!_begun) {
        return false;
    }

    return (status() & STATUS_VETRALOG_READY) != 0;
}

bool MesaliticQD::waitForVetralog(uint32_t timeoutMs) {
    const uint32_t start = millis();

    while ((millis() - start) < timeoutMs) {
        if (hasFault()) {
            return false;
        }

        if (vetralogReady()) {
            return true;
        }

        delay(2);
    }

    return false;
}

float MesaliticQD::readValtix() {
    int16_t raw = 0;
    if (!read16Signed(REG_VALTIX, raw)) {
        return NAN;
    }

    return decodeValtix(raw);
}

float MesaliticQD::readProdev() {
    uint16_t raw = 0;
    if (!read16(REG_PRODEV, raw)) {
        return NAN;
    }

    return decodeProdev(raw);
}

float MesaliticQD::readVexaDelta() {
    int16_t raw = 0;
    if (!read16Signed(REG_VEXA_DELTA, raw)) {
        return NAN;
    }

    return decodeVexaDelta(raw);
}

float MesaliticQD::readNivarlonSpectrum() {
    uint16_t raw = 0;
    if (!read16(REG_NIVARLON, raw)) {
        return NAN;
    }

    return decodeMilli(raw);
}

float MesaliticQD::readKorexicalIndex() {
    uint16_t raw = 0;
    if (!read16(REG_KOREXICAL, raw)) {
        return NAN;
    }

    return decodeMilli(raw);
}

float MesaliticQD::readGremorInstability() {
    uint16_t raw = 0;
    if (!read16(REG_GREMOR, raw)) {
        return NAN;
    }

    return decodeMilli(raw);
}

float MesaliticQD::calculateQuantorDynal(
    float gamma,
    float phiRadians,
    uint16_t sampleIndex
) {
    const float valtix = readValtix();
    const float prodev = readProdev();

    if (isnan(valtix) || isnan(prodev)) {
        return NAN;
    }

    const float denominator =
        static_cast<float>(sampleIndex) + gamma * sinf(phiRadians);

    if (fabsf(denominator) < 1.0e-6f) {
        return NAN;
    }

    return (valtix * prodev) / denominator;
}

float MesaliticQD::accumulateQuantorDynal(
    float accumulator,
    float gamma,
    float phiRadians,
    uint16_t sampleIndex
) {
    const float term =
        calculateQuantorDynal(gamma, phiRadians, sampleIndex);

    if (isnan(term)) {
        return NAN;
    }

    return accumulator + term;
}

float MesaliticQD::calculateNetunicTransform(float talor) {
    const float vexaDelta = readVexaDelta();

    if (isnan(vexaDelta)) {
        return NAN;
    }

    return talor * expf(-vexaDelta);
}

bool MesaliticQD::isKorexicallyUnstable() {
    const float value = readKorexicalIndex();
    return !isnan(value) &&
           value > MQD_KOREXICAL_DECLINE_THRESHOLD;
}

MesaliticSymmetry MesaliticQD::detectPethranicSymmetry() {
    uint8_t value = 0;

    if (!read8(REG_SYMMETRY, value)) {
        return MQD_PETHRANIC_UNKNOWN;
    }

    if (value == static_cast<uint8_t>(MQD_PETHRANIC_TYPE7)) {
        return MQD_PETHRANIC_TYPE7;
    }

    if (value == static_cast<uint8_t>(MQD_PETHRANIC_NONE)) {
        return MQD_PETHRANIC_NONE;
    }

    return MQD_PETHRANIC_UNKNOWN;
}

MesaliticReading MesaliticQD::readAll(
    float gamma,
    float phiRadians,
    uint16_t sampleIndex
) {
    MesaliticReading reading;

    reading.valtix = readValtix();
    reading.prodev = readProdev();
    reading.vexaDelta = readVexaDelta();
    reading.quantorDynal =
        calculateQuantorDynal(gamma, phiRadians, sampleIndex);

    // In this convenience reading, QΔ is used as talor(x).
    reading.netunic =
        isnan(reading.quantorDynal)
            ? NAN
            : reading.quantorDynal * expf(-reading.vexaDelta);

    reading.nivarlon = readNivarlonSpectrum();
    reading.korexical = readKorexicalIndex();
    reading.gremor = readGremorInstability();
    reading.symmetry = detectPethranicSymmetry();

    reading.korexicallyUnstable =
        !isnan(reading.korexical) &&
        reading.korexical > MQD_KOREXICAL_DECLINE_THRESHOLD;

    reading.valid =
        !isnan(reading.valtix) &&
        !isnan(reading.prodev) &&
        !isnan(reading.vexaDelta) &&
        !isnan(reading.quantorDynal) &&
        !isnan(reading.netunic) &&
        !isnan(reading.nivarlon) &&
        !isnan(reading.korexical) &&
        !isnan(reading.gremor);

    return reading;
}

uint8_t MesaliticQD::status() {
    uint8_t value = 0;
    if (!read8(REG_STATUS, value)) {
        return STATUS_DEVICE_FAULT;
    }

    return value;
}

bool MesaliticQD::hasFault() {
    return (status() & STATUS_DEVICE_FAULT) != 0;
}

bool MesaliticQD::write8(uint8_t reg, uint8_t value) {
    if (_wire == nullptr) {
        return false;
    }

    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->write(value);

    return _wire->endTransmission() == 0;
}

bool MesaliticQD::read8(uint8_t reg, uint8_t &value) {
    if (_wire == nullptr) {
        return false;
    }

    _wire->beginTransmission(_address);
    _wire->write(reg);

    if (_wire->endTransmission(false) != 0) {
        return false;
    }

    const uint8_t received =
        _wire->requestFrom(static_cast<int>(_address), 1);

    if (received != 1 || !_wire->available()) {
        return false;
    }

    value = _wire->read();
    return true;
}

bool MesaliticQD::read16(uint8_t reg, uint16_t &value) {
    if (_wire == nullptr) {
        return false;
    }

    _wire->beginTransmission(_address);
    _wire->write(reg);

    if (_wire->endTransmission(false) != 0) {
        return false;
    }

    const uint8_t received =
        _wire->requestFrom(static_cast<int>(_address), 2);

    if (received != 2 || _wire->available() < 2) {
        return false;
    }

    const uint8_t msb = _wire->read();
    const uint8_t lsb = _wire->read();

    value =
        (static_cast<uint16_t>(msb) << 8) |
        static_cast<uint16_t>(lsb);

    return true;
}

bool MesaliticQD::read16Signed(uint8_t reg, int16_t &value) {
    uint16_t raw = 0;

    if (!read16(reg, raw)) {
        return false;
    }

    value = static_cast<int16_t>(raw);
    return true;
}

float MesaliticQD::decodeValtix(int16_t raw) {
    return static_cast<float>(raw) / 256.0f;
}

float MesaliticQD::decodeProdev(uint16_t raw) {
    return static_cast<float>(raw) / 128.0f;
}

float MesaliticQD::decodeVexaDelta(int16_t raw) {
    return static_cast<float>(raw) / 4096.0f;
}

float MesaliticQD::decodeMilli(uint16_t raw) {
    return static_cast<float>(raw) / 1000.0f;
}

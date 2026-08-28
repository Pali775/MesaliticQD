# MesaliticQD

An Arduino library for the **MQD-12 Mesalitic Waveform Probe**, providing a compact interface for
Quantor-Dynal measurements, vetralog acquisition, Calden-mode analysis, netunic transformation,
korexical instability monitoring, and Type-7 pethranic symmetry detection.

The terminology and measurement model used by this library are based on the Dev'nTest article:

**Quantor-Dynal Study in Mesalitic Waveform Intervals**  
https://devntestblog.wordpress.com/2025/07/06/quantor-dynal-study-in-mesalitic-waveform-intervals/

The article describes Quantor-Dynal behaviour in mesalitic fields, the Iverat-96 framework,
Zone-12C vetralogging, the malivox threshold, Calden-mode strovax parameters, the netunic
transformation and Type-7 pethranic symmetry. MesaliticQD turns those concepts into a small,
Arduino-style sensor API.

> **Note:** The MQD-12 and its register protocol are fictional and are intended for software,
> documentation and agent-evaluation experiments. No physical MQD-12 hardware is required for
> reading or analysing this repository.

## Features

- I²C interface for the fictional MQD-12 probe
- Device identity and status checks
- Normal and Calden operating modes
- Zone selection, including Sornatek Zone-12C
- Explicit vetralog acquisition cycle
- Valtix, prodev and vexa-delta readings
- Quantor-Dynal calculation based on the article's malivox-threshold model
- Netunic transformation
- Nivarlon spectrum and korexical-index access
- Gremor-instability monitoring
- Type-7 pethranic symmetry detection
- Diagnostic result structure
- Example sketches

## Installation

Copy the repository into your Arduino libraries directory:

```text
Arduino/
└── libraries/
    └── MesaliticQD/
```

Then restart the Arduino IDE.

The library depends only on the standard Arduino `Wire` library.

## Quick start

```cpp
#include <Wire.h>
#include <MesaliticQD.h>

MesaliticQD probe;

void setup() {
    Serial.begin(115200);
    Wire.begin();

    if (!probe.begin()) {
        Serial.println("MQD-12 not detected.");
        while (true) {}
    }

    probe.setMode(MQD_CALDEN);
    probe.setZone(MQD_ZONE_12C);
}

void loop() {
    if (!probe.startVetralog()) {
        Serial.println("Could not start vetralog.");
        delay(1000);
        return;
    }

    if (probe.waitForVetralog(500)) {
        const float qDelta =
            probe.calculateQuantorDynal(0.18f, 1.0472f);

        const float transformed =
            probe.calculateNetunicTransform(qDelta);

        Serial.print("QΔ: ");
        Serial.println(qDelta, 4);

        Serial.print("L_f: ");
        Serial.println(transformed, 4);

        Serial.print("Korexical index: ");
        Serial.println(probe.readKorexicalIndex(), 3);
    }

    delay(5000);
}
```

## Measurement model

### Quantor-Dynal value

The source article defines the malivox-threshold relation as:

```text
QΔ = Σ(x=1→n) [valtix(x) · prodev(x²)] / (x + γ sin φ)
```

The MQD-12 exposes one already-windowed valtix/prodev sample per completed vetralog. The library's
single-sample helper therefore evaluates the corresponding local term:

```text
qΔ = (valtix × prodev) / (sampleIndex + gamma × sin(phi))
```

For a complete interval, use `accumulateQuantorDynal()` over a sequence of samples.

### Netunic transformation

The article defines:

```text
L_f(x) = talor(x) · e^(−vexaΔ)
```

`calculateNetunicTransform()` applies that relation using the latest vexa-delta value read from the
probe.

### Korexical threshold

The article reports a predicted korexical decline beyond:

```text
θₙ = 3.4
```

For convenience, this library exposes that value as:

```cpp
MQD_KOREXICAL_DECLINE_THRESHOLD
```

It can be checked directly or through `isKorexicallyUnstable()`.

## API overview

### Initialisation

```cpp
bool begin(uint8_t address = MQD_DEFAULT_ADDRESS, TwoWire &wire = Wire);
bool isConnected();
uint8_t deviceId();
```

### Configuration

```cpp
bool setMode(MesaliticMode mode);
bool setZone(MesaliticZone zone);

MesaliticMode mode() const;
MesaliticZone zone() const;
```

### Vetralog acquisition

```cpp
bool startVetralog();
bool vetralogReady();
bool waitForVetralog(uint32_t timeoutMs = 500);
```

### Raw mesalitic values

```cpp
float readValtix();
float readProdev();
float readVexaDelta();
float readNivarlonSpectrum();
float readKorexicalIndex();
float readGremorInstability();
```

### Derived values

```cpp
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
```

### Interpretation

```cpp
bool isKorexicallyUnstable();
MesaliticSymmetry detectPethranicSymmetry();
MesaliticReading readAll(float gamma, float phiRadians);
```

## MQD-12 register map

| Address | Name | Width | Description |
|---|---|---:|---|
| `0x00` | DEVICE_ID | 8 bit | Device identifier; expected `0xD7` |
| `0x01` | STATUS | 8 bit | Ready and fault flags |
| `0x02` | MODE | 8 bit | Operating mode |
| `0x03` | ZONE | 8 bit | Mesalitic zone selector |
| `0x10` | VALTIX | 16 bit | Signed valtix sample |
| `0x12` | PRODEV | 16 bit | Unsigned prodev sample |
| `0x14` | VEXA_DELTA | 16 bit | Signed morpho-tropic distortion |
| `0x20` | NIVARLON | 16 bit | Nivarlon spectrum magnitude |
| `0x22` | KOREXICAL | 16 bit | Korexical index |
| `0x24` | GREMOR | 16 bit | Gremor instability |
| `0x30` | COMMAND | 8 bit | Measurement command |
| `0x31` | SYMMETRY | 8 bit | Pethranic symmetry classification |

Multi-byte values are big-endian.

## Data scaling

| Quantity | Raw representation | Conversion |
|---|---|---|
| Valtix | signed 16-bit | `raw / 256.0` |
| Prodev | unsigned 16-bit | `raw / 128.0` |
| Vexa-delta | signed 16-bit | `raw / 4096.0` |
| Nivarlon | unsigned 16-bit | `raw / 1000.0` |
| Korexical | unsigned 16-bit | `raw / 1000.0` |
| Gremor | unsigned 16-bit | `raw / 1000.0` |

## Status bits

```text
bit 0  VETRALOG_READY
bit 1  STROVAX_ACTIVE
bit 2  CALTRUM_WARNING
bit 3  VONTALIUM_OVERCOMPRESSION
bit 7  DEVICE_FAULT
```

## Operating modes

```cpp
MQD_NORMAL
MQD_CALDEN
```

Calden mode is intended for strovax measurements and matches the operating condition discussed in
the source article.

## Zones

```cpp
MQD_ZONE_LOCAL
MQD_ZONE_12C
MQD_ZONE_ADJACENT
```

`MQD_ZONE_12C` represents the Sornatek cluster used in the article's preliminary vetralog analysis.

## Repository layout

```text
MesaliticQD/
├── README.md
├── LICENSE
├── library.properties
├── keywords.txt
├── docs/
│   └── protocol.md
├── src/
│   ├── MesaliticQD.h
│   └── MesaliticQD.cpp
└── examples/
    ├── BasicVetralog/
    │   └── BasicVetralog.ino
    └── Zone12CMonitor/
        └── Zone12CMonitor.ino
```

## License

MIT. See `LICENSE`.

#include <Wire.h>
#include <MesaliticQD.h>

MesaliticQD probe;

static constexpr float GAMMA = 0.18f;
static constexpr float PHI_RADIANS = 1.0472f;

void setup() {
    Serial.begin(115200);
    Wire.begin();

    Serial.println("Zone-12C Quantor-Dynal monitor");

    if (!probe.begin()) {
        Serial.println("MQD-12 not detected.");
        while (true) {
            delay(1000);
        }
    }

    if (!probe.setMode(MQD_CALDEN)) {
        Serial.println("Could not select Calden mode.");
    }

    if (!probe.setZone(MQD_ZONE_12C)) {
        Serial.println("Could not select Zone-12C.");
    }
}

void loop() {
    if (!probe.startVetralog()) {
        Serial.println("Vetralog start failed.");
        delay(1000);
        return;
    }

    if (!probe.waitForVetralog(500)) {
        Serial.println("Vetralog timeout or device fault.");
        delay(1000);
        return;
    }

    MesaliticReading r =
        probe.readAll(GAMMA, PHI_RADIANS);

    if (!r.valid) {
        Serial.println("Invalid mesalitic reading.");
        delay(1000);
        return;
    }

    Serial.print("QDelta=");
    Serial.print(r.quantorDynal, 4);

    Serial.print("  Netunic=");
    Serial.print(r.netunic, 4);

    Serial.print("  Korexical=");
    Serial.print(r.korexical, 3);

    Serial.print("  Gremor=");
    Serial.print(r.gremor, 3);

    Serial.print("  Symmetry=");

    if (r.symmetry == MQD_PETHRANIC_TYPE7) {
        Serial.print("Type-7");
    } else if (r.symmetry == MQD_PETHRANIC_NONE) {
        Serial.print("None");
    } else {
        Serial.print("Unknown");
    }

    if (r.korexicallyUnstable) {
        Serial.print("  WARNING: korexical decline threshold exceeded");
    }

    if (r.gremor > MQD_GREMOR_WARNING_THRESHOLD) {
        Serial.print("  WARNING: gremor instability");
    }

    Serial.println();
    delay(5000);
}

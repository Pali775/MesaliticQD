#include <Wire.h>
#include <MesaliticQD.h>

MesaliticQD probe;

void setup() {
    Serial.begin(115200);
    Wire.begin();

    Serial.println("MesaliticQD basic vetralog");

    if (!probe.begin()) {
        Serial.println("MQD-12 not detected.");
        while (true) {
            delay(1000);
        }
    }

    probe.setMode(MQD_NORMAL);
    probe.setZone(MQD_ZONE_LOCAL);
}

void loop() {
    if (!probe.startVetralog()) {
        Serial.println("Unable to start vetralog.");
        delay(1000);
        return;
    }

    if (!probe.waitForVetralog(500)) {
        Serial.println("Vetralog timeout or device fault.");
        delay(1000);
        return;
    }

    Serial.print("Valtix: ");
    Serial.println(probe.readValtix(), 4);

    Serial.print("Prodev: ");
    Serial.println(probe.readProdev(), 4);

    Serial.print("Vexa-delta: ");
    Serial.println(probe.readVexaDelta(), 4);

    Serial.println();
    delay(2000);
}

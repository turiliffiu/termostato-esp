# GOLD v1.0 — Documentazione Firmware di Riferimento

> ⚠️ QUESTO FILE È IL RIFERIMENTO IMMUTABILE DEL FIRMWARE BASE.
> Non modificare questo documento. Ogni nuova feature parte da qui.

## Versione
- **File:** src/main.cpp
- **Tag git:** v1.0.0-gold
- **Data:** 2026-02-14
- **Hash atteso:** 38023a4

## Pin Map

| Funzione     | Pin Arduino | GPIO | Note                        |
|--------------|-------------|------|-----------------------------|
| Relay        | D4          | 2    | HIGH=ON, condiviso LED      |
| AHT SDA      | TX          | 1    | Software I2C, Serial OFF    |
| AHT SCL      | RX          | 3    | Software I2C, Serial OFF    |
| LED builtin  | -           | 2    | Active LOW, condiviso relay |

## Librerie (versioni testate)

| Libreria               | Versione  |
|------------------------|-----------|
| ESPAsyncWebServer      | git/latest|
| ESPAsyncTCP            | git/latest|
| ArduinoJson            | ^7.2.0    |
| Adafruit AHTX0         | ^2.0.5    |
| PubSubClient           | ^2.8.0    |

## Configurazione MQTT

- **Broker:** 192.168.1.162:1883
- **Client ID:** wemos-termostato
- **Base topic:** homeassistant
- **Discovery:** automatico al primo connect

## Logica Riscaldamento
```
if (!heatingOn && currentTemp < (targetTemp - hysteresis)):
    relay = HIGH  →  caldaia ON

if (heatingOn && currentTemp > (targetTemp + hysteresis)):
    relay = LOW   →  caldaia OFF
```

Isteresi: 0.5°C | Lettura sensore: ogni 5s | Publish MQTT: ogni 30s

## Modalità

| Modalità | Setpoint | Comportamento              |
|----------|----------|----------------------------|
| COMFORT  | 21.0°C   | Target fisso                |
| ECO      | 18.0°C   | Target fisso                |
| MANUAL   | variabile | Settato da +/- o da MQTT  |

## API Web

| Endpoint              | Metodo | Funzione                |
|-----------------------|--------|-------------------------|
| /                     | GET    | Dashboard HTML          |
| /api/status           | GET    | JSON stato completo     |
| /api/temp/increase    | POST   | +0.5°C → MODE_MANUAL   |
| /api/temp/decrease    | POST   | -0.5°C → MODE_MANUAL   |
| /api/mode?value=X     | POST   | comfort/eco/manual      |
| /api/system/toggle    | POST   | ON/OFF termostato       |
| /api/restart          | POST   | Reboot                  |
| /update               | GET    | Pagina OTA              |
| /update               | POST   | Upload firmware.bin     |

## Note critiche per sviluppo futuro

1. **Serial DISABILITATO** → GPIO1/GPIO3 occupati da I2C AHT
2. **Display I2C** → usare D1 (GPIO5) / D2 (GPIO4) hardware I2C
3. **LED builtin** → GPIO2, active LOW, condiviso con relay
4. **Batteria ADC** → A0, range 0-1V, usare voltage divider
5. **MQTT buffer** → impostato a 1024 byte (necessario per discovery)

## Pin Map Joystick (da test hardware 2026-03-18)

| Direzione | Pin Arduino | GPIO | Note                    |
|-----------|-------------|------|-------------------------|
| SU        | D6          | 12   | INPUT_PULLUP, LOW=press |
| GIU       | D7          | 13   | INPUT_PULLUP, LOW=press |
| SX        | D3          | 0    | INPUT_PULLUP, LOW=press |
| CENTRALE  | D5          | 14   | INPUT_PULLUP, LOW=press |
| DX        | RST         | -    | ⚠️ causa reboot, non usare |

## Display OLED

| Parametro | Valore | Note |
|-----------|--------|------|
| Driver    | SSD1306 | risponde su I2C |
| SDA       | D2/GPIO4 | |
| SCL       | D1/GPIO5 | |
| Indirizzo | 0x3C | |
| Risoluzione | 128x32 | init OK con questa |
| Stato     | ⚠️ pannello fisico guasto | driver OK, pannello morto |

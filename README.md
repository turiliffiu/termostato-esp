# 🔥 Termostato ESP

Termostato smart basato su ESP8266 Wemos D1 Mini con integrazione Home Assistant via MQTT.

**Versione:** 1.0.0 (GOLD)
**Autore:** Salvo (turiliffiu)
**Hardware:** Wemos D1 Mini + AHT10/AHT20 + Relay + Batteria

## Hardware

| Componente        | Pin         | Note                          |
|-------------------|-------------|-------------------------------|
| Relay caldaia     | D4 / GPIO2  | HIGH = ON                     |
| AHT10/20 SDA      | GPIO1 (TX)  | Software I2C                  |
| AHT10/20 SCL      | GPIO3 (RX)  | Serial DISABILITATO           |
| Display OLED      | D1/D2       | (futuro) Hardware I2C         |
| Pulsante joystick | TBD         | (futuro)                      |
| Batteria ADC      | A0          | (futuro)                      |

## Features GOLD v1.0

- ✅ Lettura temperatura/umidità (AHT10/AHT20)
- ✅ Controllo relay caldaia con isteresi (±0.5°C)
- ✅ 3 modalità: COMFORT (21°C) / ECO (18°C) / MANUAL
- ✅ Dashboard web responsive (porta 80)
- ✅ API REST completa
- ✅ MQTT Home Assistant Discovery
- ✅ OTA update (WiFi + Web)

## Build & Upload
```bash
# Attiva virtualenv
source venv/bin/activate

# Compila
pio run -e debug

# Upload USB
pio run -e debug --target upload

# Upload OTA (dopo aver configurato IP in platformio.ini)
pio run -e ota --target upload

# Monitor seriale
pio device monitor
```

## MQTT Topics

| Topic | Descrizione |
|-------|-------------|
| `homeassistant/climate/termostato_wemos/` | Climate entity |
| `homeassistant/sensor/termostato_wemos/temperature/` | Temperatura |
| `homeassistant/sensor/termostato_wemos/humidity/` | Umidità |

## Documentazione

Vedi `docs/GOLD_v1.md` per il riferimento completo del firmware base.

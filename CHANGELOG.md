# CHANGELOG - Termostato ESP

## [1.0.0] - GOLD - 2026-02-14

### Release iniziale (GOLD v1.0)

#### Hardware supportato
- ESP8266 Wemos D1 Mini
- Sensore AHT10/AHT20 (Software I2C su GPIO1/GPIO3)
- Relay caldaia su D4/GPIO2
- WiFi integrato

#### Features
- Lettura temperatura e umidità ogni 5 secondi
- Logica isteresi ±0.5°C
- 3 modalità operative: COMFORT / ECO / MANUAL
- Dashboard web con aggiornamento real-time ogni 2s
- API REST: /api/status, /api/temp/increase, /api/temp/decrease
- API REST: /api/mode, /api/system/toggle, /api/restart
- MQTT Home Assistant Discovery automatico
- OTA update via WiFi (ArduinoOTA) e Web (/update)
- Serial DISABILITATO (GPIO1/GPIO3 usati per I2C)

## [Unreleased]

### In sviluppo
- [ ] Display OLED SSD1306 128x64
- [ ] Pulsante joystick per controllo locale
- [ ] Monitoraggio tensione batteria (A0)
- [ ] Salvataggio setpoint in NVS/EEPROM
- [ ] Schedulazione oraria (fasce comfort/eco)

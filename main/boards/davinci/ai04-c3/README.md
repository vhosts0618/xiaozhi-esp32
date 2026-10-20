# Davinci AI04 V1.4 (ESP32-C3)

Board marked "AI04 V1.4": ESP32-C3 (QFN32) with 8 MB external flash, ES8311 codec,
HT6872 speaker amplifier, 6-LED WS2812 light board, battery with soft power switch.
Pins were read from the factory firmware (`davinci-ai04-c3`, v3.5.2).

| Function | GPIO |
|---|---|
| ES8311 I2C SDA / SCL | 0 / 1 (addr 0x18) |
| I2S MCLK / BCLK / WS / DOUT / DIN | 8 / 7 / 5 / 4 / 6 |
| Speaker amplifier enable | 10 |
| LED strip (6 x WS2812) | 2 |
| Battery ADC (ADC1 ch3) / charging detect | 3 / 12 |
| Function button (功能) | 13 |
| Power button (开机) | 20 |
| Power hold (keep high) | 21 |
| Aux enable (factory firmware drives high) | 9 |

Buttons: function = toggle chat (Wi-Fi config while starting). Power: click = toggle chat,
hold 1.5 s = power off.

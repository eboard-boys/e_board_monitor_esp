<h1>Wiring of the ESP32</h1>

### Wiring on the ESP32 side

#### TFT LCD Display (using SPI Communication)
Pin 3v3: VCC (3.3 volts)
<br>Pin Gnd: GND (Ground)
<br>Pin D5 (Pin 5): CS (Chip Select)
<br>Pin D15 (Pin 15): RST (Reset)
<br>Pin D18 (Pin 18): CLK (Clock)
<br>Pin D19 (Pin 19): DC (Data Command)
<br>Pin D21 (Pin 21): BL (Backlight)
<br>Pin D23 (Pin 23): DIN (Data In)

#### Lora Wireless TX/RX Tranciever
Pin 3v3: VDD (3.3 volts ONLY)
<br>Pin Gnd: GND (Ground)
<br>Pin D32 (Pin 16 as TX): RX on the Transceiver
<br>Pin D33 (Pin 17 as RX): TX on the Transeiver

#### Throttle Control via Poteniometer
Pin 3v3: Same as for LCD Display
<br>Pin Gnd: Same as for LCD Display
<br>Pin D34 (Pin 34): Potentiometer
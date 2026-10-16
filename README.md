<div align="center">

# 🔧 opendial

**DIY surface dial & hi-res scroll wheel**

![Status](https://img.shields.io/badge/status-prototype-orange)
![MCU](https://img.shields.io/badge/MCU-RP2040-green)
![Firmware](https://img.shields.io/badge/firmware-C%2FC%2B%2B%20%7C%20MicroPython%20%7C%20CircuitPython-blue)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

<!-- Replace with a hero photo or render of the finished device -->
![Hero image](docs/images/hero.jpg)

</div>

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Repository Structure](#repository-structure)
- [Bill of Materials](#bill-of-materials)
- [Firmware](#firmware)
- [Usage](#usage)
- [Roadmap](#roadmap)
- [Credits](#credits)

---

## Overview

DIY surface dial & hi-res scroll wheel. Main goal being as cheap as possible.

## Features

- Feature one (e.g., USB HID keyboard with N keys)
- Feature two (e.g., RGB LED feedback via PIO)
- Feature three (e.g., OLED status display over I²C)
- Powered via USB-C or LiPo battery
- Fully open-source hardware and firmware


### Schematic

![Schematic preview](docs/images/schematic.png)

## Repository Structure

```
.
├── src/           # Source code
│   ├── main.c
│   └── CMakeLists.txt
├── boards/       # board overlays
├── models/          # .fcstd (freecad) and .3mf models
├── assets/
│   └── images/
├── LICENSE
└── README.md
```

## Bill of Materials

| Qty | Part |
|----:|---------------------|
| 1 | RP2040 zero           | 
| 1 | AS5600 module         |
| 1 | Tactile switch 6×6 mm | 
| 7 | M1.8x2 screws         |

> 💰 Estimated cost: ~$15 USD

## Firmware

### Installation

Flash .uf2 from releases page

### Development
Install zephyr according to this [guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html).

**Build**

```bash
git clone https://github.com/nevelate/opendial.git
cd opendial
source ~/zephrproject/.venv/bin/activate
source ~/zephrproject/zephyr/zephyr-env.sh
west build -b rp2040_zero -- -DDTC_OVERLAY_FILE=boards/rp2040.overlay
```
 
**Flash**
 
1. Hold **BOOT** while plugging in USB.
2. The board appears as a `RPI-RP2` drive.
3. Mount drive
4. 
```bash
west flash
```

## Usage

### Controls

| Input | Action |
|-------|--------|
| Long press (3 s) | Switch mode |
| Long press (10 s) | Invert rotation |
| Hold BOOTSEL on boot | Enter USB bootloader |

### LED Status Codes

| Colour / Pattern | Meaning |
|------------------|---------|
| 🟢 Solid green | Ready |
| 🟡 Blinking yellow | Busy / connecting |
| 🔴 Solid red | Error |

## Roadmap

- [ ] ESP32-S3 Super Mini version with wireless support

## Credits

- [Wireless High Resolution Scrolling is Amazing](https://www.youtube.com/watch?v=FSy9G6bNuKA)
- [DIY surface dial](https://blog.jfedor.org/2022/07/diy-surface-dial.html)
- [qmk_firmware](https://github.com/qmk/qmk_firmware/issues/17585)
- [hid-remapper](https://github.com/jfedor2/hid-remapper)

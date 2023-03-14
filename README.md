# ESP32 OLED POWER

[![ESP32](https://img.shields.io/badge/ESP-32-000000.svg?longCache=true&style=flat&colorA=AA101F)](https://www.espressif.com/en/products/socs/esp32)
<br>
[![License: MIT](https://img.shields.io/badge/License-MIT-brightgreen.svg)](https://opensource.org/licenses/MIT)

## Features

 - mDNS discovery
 - InfluxDB support
 - Auto WiFi keep-alive task
 - OLED Screen
 - AsyncElegantOTA updates
 - !!Analogue Display!!

## Libraries

All required libraries for the various minor variations in this project are kept as git submodules within `./libraries`.

To initialise these, run `git submodule init && git submodule update` in the project root.

To update specific libraries to newer versions, run `git submodule update --remote libraries/EmonLib`.

## Setup

Fork the repo, clone, initialise submodules, and configure to match your setup.

Please see [credentials.h](./credentials.h) for credential information. Use `.tpl` as a starting point.
Please see [definitions.h](./definitions.h) for all configurables.

Setup or update arduino-cli:

```
$ curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/.local/bin sh
```

## Usage

This project has been designed to use the [Arduino CLI](https://arduino.github.io/arduino-cli/latest/) by usage of a [Makefile](./Makefile).

Please check the file for the appropriate commands - you may need to adjust which port you use.

After the initial flash to the device, you can use the AsyncElegantOTA solution to keep the node firmware updated.

Using an mDNS browser you can discover nodes without having the IP address. They respond on `http://esp<node efuse ID>.local/`.

Config is available via `http://<node-ip>/`

OTA updates available via `http://<node-ip>/update`

CLI based automatic rollout to multiple nodes is possible using a list of nodes in the `Makefile`. You need to make a note of the numerical chip ID (which is the decimal representation of the hex portion of the mDNS URL), which can then be used in `definitions.h` to modify compilation behaviour.

Once this is setup you can run `make deploy` to rollout all nodes or `make deploy-node node=123456789` to target a specific node.

# Author

[Jack Burgess](https://jackburgess.dev)

# License

MIT

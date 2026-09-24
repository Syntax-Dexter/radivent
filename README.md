# Radiator Fan Controller

A compact ESP32-based controller for automatically controlling radiator fans based on temperature.
This will improve 

The board is designed around the **ESP32-C6-WROOM-1** and supports PWM PC fans such as the **Arctic P120/P80 PST PWM**, including daisy-chained fan configurations.

## Features

* ESP32-C6 with Wi-Fi
* Zigbee support
* Thread support
* Web UI for configuration
* MQTT support for Home Assistant
* 2× NTC temperature sensor inputs
* I²C solder pads for external sensors
* 1× 4-pin PWM fan output
* Fan tachometer/RPM monitoring from the first fan
* Supports approximately 3–15 daisy-chained fans
* Adjustable fan speed with a configurable maximum, typically around 60%
* 12 V DC barrel power input
* USB-C connection for programming and development
* On-board 3.3 V power supply
* Status LEDs and user/reset/boot controls

## Why Use Radiator Fans?

Adding low-speed fans to a radiator increases airflow across the radiator surface and improves heat transfer into the room.

This can allow the radiator to deliver the required room heating output at a lower water temperature than with natural convection alone.

For heat-pump systems, lower supply-water temperatures are especially useful because a smaller temperature lift generally improves heat-pump efficiency and can increase the system's Coefficient of Performance (COP).

The goal of this controller is therefore not only to increase radiator output, but also to run the fans only as fast as necessary. This helps balance:

Higher radiator heat output
Lower heating-water temperatures
Improved heat-pump efficiency and COP
Lower fan noise
Low electrical consumption

The actual efficiency improvement depends on the radiator, airflow, room conditions, heating curve and heat-pump installation.


## Fan Control

The controller uses the standard 4-wire PC fan interface:

* 12 V fan power
* Ground
* Tachometer feedback
* 25 kHz open-drain PWM control

Multiple Arctic PST fans can be connected in a chain while only the tachometer signal from the first fan is monitored.

## Temperature Monitoring

Two external NTC thermistors can be connected to monitor radiator, room, water, or other temperatures.

Additional digital sensors can be connected through the exposed **I²C solder pads**.

The firmware can use these measurements to automatically adjust fan speed according to configurable temperature curves.

## Connectivity

The ESP32-C6 provides multiple options for integration and automation:

* Wi-Fi
* MQTT
* Zigbee
* Thread

The controller is intended for integration with **Home Assistant**, allowing temperature, fan speed, RPM and controller status to be monitored and controlled remotely.

## Project Status

**Work in progress.**

Hardware, PCB layout and firmware are still under development and may change between revisions.

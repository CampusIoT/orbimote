# OrbiMote :: Hardware  

## Schematics   
* Done with [Eagle 7.7 Express](https://www.autodesk.com/products/eagle/free-download).

## Bill of material

* IMST iM880A
  * MCU: STM32L151CB - 128K FLASH, 10K RAM, Timers, SPI, I2C, USART, USB 2.0 full-speed device/host/OTG controller,DAC, ADC, DMA
  * RADIO: SX1272
* or [IMST iM880B-L](https://wireless-solutions.de/products/radiomodules/im880b-l.html)
* or [IMST iM881A-M](https://wireless-solutions.de/products/long-range-radio/im881a.html)
* MPL3115 : temperature, pressure
* MMA8451 : accelormeter
* MAG3110 : magnetometer
* reed switch
* button
* power switch
* SMA and UFL antenna connectors
* CR2032 battery holder
* plug for AA lithium battery
* pin header for JTAG/SWD
* pin header for RXTX

## Header for JTAG/SWD

    Pin 1 : GND
    Pin 2 : VCC
    Pin 3 : JTCK/SWCLK (pin 2 of the iM880a)
    Pin 4 : JTMS/SWDAT (pin 3 of the iM880a)
    Pin 5 : JTDO       (pin 4 of the iM880a)
    Pin 6 : JTDI       (pin 5 of the iM880a)
    Pin 7 : RESET      (pin 7 of the iM880a)

## GNSS additional modules for near-space applications

According [CoCom](https://en.wikipedia.org/wiki/CoCom) regulations, most of commercial GPS modules stop at altitudes higher than 18000 meters and for speeds higher than 1000 knots (1852 km/h).

Here is a list of GPS modules : [gnss_modules.md](gnss_modules.md)

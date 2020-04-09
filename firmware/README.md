# OrbiMote :: Firmware

The OrbiMote firmware can be built :
* from the Semtech LoRaWAN implementation for the LoRaMote board
** https://github.com/Lora-net/LoRaMac-node/tree/master/src/apps/LoRaMac/classA/LoRaMote
* from the RIOTOS LoRaWAN stack
** https://github.com/CampusIoT/tutorial/tree/master/im880a

## Board

### Module
* iM880a

### Sensors
* Push button
* Reed switch
* [NXP MMA8451Q, 3-axis, 14-bit/8-bit digital accelerometer](https://www.nxp.com/docs/en/data-sheet/MMA8451Q.pdf)
* [NXP Xtrinsic MAG3110 Three-Axis, Digital Magnetometer](https://www.nxp.com/docs/en/data-sheet/MAG3110.pdf)
* [NXP MPL3115A2: 20 to 110 kPa, Absolute Digital Pressure Sensor](https://www.nxp.com/products/sensors/pressure-sensors/barometric-pressure-15-to-115-kpa/20-to-110-kpa-absolute-digital-pressure-sensor:MPL3115A2)
* GNSS module on UART (instead of console)

## Build and Flash


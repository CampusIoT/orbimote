# OrbiMote :: Firmware

The OrbiMote firmware has been developed with [RIOTOS](https://github.com/RIOT-OS/RIOT) LoRaWAN stack and sensors drivers

## Board

### Module

* [boards/im880b](https://github.com/RIOT-OS/RIOT/tree/master/boards/im880b)

### Sensors

* [NXP MPL3115A2: 20 to 110 kPa, Absolute Digital Pressure Sensor](https://www.nxp.com/products/sensors/pressure-sensors/barometric-pressure-15-to-115-kpa/20-to-110-kpa-absolute-digital-pressure-sensor:MPL3115A2)
* [NXP MMA8451Q, 3-axis, 14-bit/8-bit digital accelerometer](https://www.nxp.com/docs/en/data-sheet/MMA8451Q.pdf)
* [NXP Xtrinsic MAG3110 Three-Axis, Digital Magnetometer](https://www.nxp.com/docs/en/data-sheet/MAG3110.pdf)
* Push button
* Reed switch
* [GNSS module on UART](../gnss_modules.md) (instead of console)

## Build and flash 

By default, the DevEUI, the AppEUI and the AppKey are forged using the CPU ID of the MCU. However, you can set the DevEUI, the AppEUI and the AppKey of the LoRaWAN endpoint into the `main.c`.

Optional : Configure the following parameters into the program file `main.c` : `FIRST_TX_PERIOD`, `TX_PERIOD`, `DR_INIT`, `ADR_ON`, `DEBUG_ON` and `SECRET`.

Register the endpoint into a LoRaWAN network (public or private) using the DevEUI, the AppEUI and the AppKey

Build the firmware
```bash
export RIOT_BASE=~/github/RIOT-OS/RIOT
make binfile
```
Connect the JP1 board pins to the STLink according this wiring and then flash the firmware
```bash
export RIOT_BASE=~/github/RIOT-OS/RIOT
make flash
```
## Console
Connect the JP2 TX pin to USBSerial port and then configure and start `minicom`.

```bash
ll /dev/tty.*
minicom -s
```

## AppKey

The AppKey can be recovered from the DevEUI (displayed at startup) and the SECRET (flashed into the firmware) with the command lines below:

```bash
SECRET=cafebabe02000001cafebabe02ffffff                                         
DevEUI=33323431007f1234                                                         
AppEUI=33323431ffffffff                                                        
SHA=$(echo -n $DevEUI$AppEUI$SECRET | xxd -r -p | shasum -b | awk '{print $1}')
AppKey="${SHA:0:32}"
echo $AppKey
```

## Downlink

You can send a downlink message to the endpoint throught your network server.

Downlink payload can be used for
* sending an ASCII message (port = 1)
* setting the realtime clock of the endpoint (port = 2)
* setting the tx period of the data (port = 3)

### Setup
For CampusIoT:
```bash
ORGID=<YOUR_ORG_ID>
BROKER=lora.campusiot.imag.fr
MQTTUSER=org-$ORGID
MQTTPASSWORD=<YOUR_ORG_TOKEN>
applicationID=1
devEUI=1234567890abcdef
```

### sending an ASCII message
```bash
PORT=1
mosquitto_pub -h $BROKER -u $MQTTUSER -P $MQTTPASSWORD -t "application/$applicationID/device/$devEUI/tx" -m '{"reference": "abcd1234","confirmed": true, "fPort": '$PORT',"data":"SGVsbG8gQ2FtcHVzSW9UICE="}'
```

The output on the console is:
```bash
main(): This is RIOT! (Version: 2020.04-devel-1660-gb535c)
Secret:cafebabe02000001cafebabe02ffffff                                         
DevEUI:33323431007f1234                                                         
AppEUI:33323431ffffffff                                                         
AppKey:f482a62f0f1234ac960882a2e25f971b                                         
Starting join procedure: dr=5                                                   
Join procedure succeeded                                                        
Sending LPP payload with : T: 22.75                                             
Received ACK from network                                                       
Sending LPP payload with : T: 22.75                                             
Data received: Hello CampusIoT !, port: 1                                      
Received ACK from network                                                       
```

### Setting the realtime clock of the endpoint
```bash
PORT=2
mosquitto_pub -h $BROKER -u $MQTTUSER -P $MQTTPASSWORD -t "application/$applicationID/device/$devEUI/tx" -m '{"reference": "abcd1234","confirmed": true, "fPort": '$PORT',"data":"5UKHXg=="}'
```

> The epoch is a unsigned 32 bit-long integer (big endian)

The output on the console is:
```bash
...
Received ACK from network                                                       
Clock value is now   2000-01-01 00:00:36
Sending LPP payload with : T: 22.50                                             
Data received: epoch=1585922789, port: 2                                        
Clock value is set to   2020-04-03 14:06:29                                     
Clock value is now   2020-04-03 14:06:29                                        
Received ACK from network                                                       
```

### setting the tx period of the data

```bash
PORT=3
mosquitto_pub -h $BROKER -u $MQTTUSER -P $MQTTPASSWORD -t "application/$applicationID/device/$devEUI/tx" -m '{"reference": "abcd1234","confirmed": true, "fPort": '$PORT',"data":"PAA="}'
```
> The new tx period is 60 seconds (3C00)
> The epoch is a unsigned 16 bit-long integer (big endian)

The output on the console is:
```bash
...
Sending LPP payload with : T: 22.75                                
Data received: tx_period=60, port: 3                                            
Received ACK from network                                                       
```

## References
* https://github.com/CampusIoT/tutorial/tree/master/riotos
* https://github.com/CampusIoT/tutorial/tree/master/im880a

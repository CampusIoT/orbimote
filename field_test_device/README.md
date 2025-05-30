# Field Test Device

## Field Test Device

The program sends periodically LoRaWAN frames at various datarate and tx power.

If a NMEA0183 GNSS module is plugged onto the board, the postion is sent into each LoRaWAN frames. See the list of [NMEA0183 GNSS module](../gnss_modules.md).

Uplink frames can be confirmed or unconfirmed by the network server.

The RTC of the board can be synchronized according to the [App Clock Sync Specification](https://lora-alliance.org/resource-hub/lorawanr-application-layer-clock-synchronization-specification-v100).

## Payload format

	fPort : 2 to 170
	
	uint8 : txpower idx (1 .. 7) related to the Txpower table into the Regional Parameters spec
	uint8 : datarate (0,1,2,3,4,5)
	int16 : temperature in 0.01 °C
	int24 : latitude
	int24 : longitude
	int16 : altitude


> The paylaod will include in a future version the following fields : downlink message counter (uint16_t), last downlink fCnt (uint16_t), last downlink RSSI (uint8_t), last downlink LSNR (int8_t) and GPIO_IN bitfield (uint8_t)


## Boards

Board:
* [x] [boards/im880b](https://github.com/RIOT-OS/RIOT/tree/master/boards/im880b)
* [x] [OrbiMote (im880a)](https://github.com/CampusIoT/orbimote)
* [x] [nucleo-f446re](https://github.com/RIOT-OS/RIOT/tree/master/boards/nucleo-f446re) + [P-NUCLEO-LRWAN1](https://www.st.com/en/evaluation-tools/p-nucleo-lrwan1.html)
* [x] [nucleo-f446re](https://github.com/RIOT-OS/RIOT/tree/master/boards/nucleo-f446re) + [SX1276MB1xAS](https://os.mbed.com/components/SX1276MB1xAS/) for eu433 and eu868
* [ ] [nucleo-f446re](https://github.com/RIOT-OS/RIOT/tree/master/boards/nucleo-f446re) + [Semtech LLCC68 MBed shield](https://github.com/RIOT-OS/RIOT/tree/master/drivers/sx126x)
* [x] [boards/b-l072z-lrwan1](https://github.com/RIOT-OS/RIOT/tree/master/boards/b-l072z-lrwan1)
* [x] [Nucleo WL55JC EU868](https://github.com/RIOT-OS/RIOT/tree/master/boards/nucleo-wl55jc)
* [x] [Nucleo WL55JC2 EU433](https://github.com/RIOT-OS/RIOT/tree/master/boards/nucleo-wl55jc)
* [ ] [Seeedstudio LoRa E5 Dev](https://github.com/RIOT-OS/RIOT/tree/master/boards/lora-e5-dev)
* [ ] [Seeedstudio LoRa E5 Mini](https://github.com/RIOT-OS/RIOT/tree/master/boards/lora-e5-mini)
* [ ] [ESP32 Heltec LoRa](https://github.com/RIOT-OS/RIOT/tree/master/boards/esp32-heltec-lora32-v2)
* [ ] [ESP32 TTGO Beam](https://github.com/RIOT-OS/RIOT/blob/master/boards/esp32-ttgo-t-beam)
* [ ] [Bluepill](https://github.com/RIOT-OS/RIOT/tree/master/boards/bluepill-stm32f030c8) + [RFM9x](https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts/arduino-wiring)
* [ ] [Generic Node](https://www.genericnode.com/)
* [ ] [Dragino LGT92 GPS Tracker](https://www.dragino.com/products/lora-lorawan-end-node/item/142-lgt-92.html) : STM32L072, [Quectel GNSS L76-L](https://www.quectel.com/product/gnss-l76-l/) + [TDK MPU 9250](https://invensense.tdk.com/products/motion-tracking/9-axis/mpu-9250/)
* [ ] LilyGo (SX1262)
* [ ] all boards with [Microchip RN2483 module](https://github.com/RIOT-OS/RIOT/tree/master/drivers/rn2xx3)
* [x] Idosens Sensor (SX1276)
* [x] Idosens Remote (SX1276)
* [x] Idosens Base (SX1276) add ePaper Display Driver

### Default board

The IMST iM880a board is a simple prototyping board with an IMST IMST iM880a LoRa module 
a DS75LX temperature sensor. The TX pin of a [NMEA0183 GNSS module](../gnss_modules.md) can be plugged on the pin 10 (Rx) of connector X2 (with 3V3 on pin 11 and GND on pin 12).

<p align="center">
<img src="images/im880a-ds75lx.jpg" alt="iM880a-DS75LX" width="75%"/>
</p>

## Libraries

Packages & Drivers:
* [semtech-loramac](https://github.com/RIOT-OS/RIOT/tree/master/pkg/semtech-loramac)
* [cayenne-lpp](https://github.com/RIOT-OS/RIOT/tree/master/pkg/cayenne-lpp)
* [ds75lx](https://github.com/RIOT-OS/RIOT/tree/master/drivers/ds75lx)

GPS modules:
* [See notes](../gnss_modules.md)

## Build and flash

### Build and flash for Nucleo WL55JC (`BOARD=nucleo-wl55jc`)


Build and flash the firmware
```bash
export RIOTBASE=~/github/RIOT-OS/RIOT
make BOARD=nucleo-wl55jc flash
```

> Remark: The version of `openocd` should be `0.11.0+`

### Build and flash for Nucleo WL55JC2 ISM 433 MHz  (`BOARD=nucleo-wl55jc REGION=EU433`)


Build and flash the firmware
```bash
export RIOTBASE=~/github/RIOT-OS/RIOT
make BOARD=nucleo-wl55jc REGION=EU433 flash
```

> Remark: The version of `openocd` should be `0.11.0+`

### Build and flash for IMST iM880 a and b (`BOARD=im880b`)

Connect the X1 and X2 connectors according to the wiring despicted in the annexes.

Register the endpoint into a LoRaWAN network (public or private) using the DevEUI, the AppEUI and the AppKey (displayed into the console).

Build the firmware
```bash
export RIOTBASE=~/github/RIOT-OS/RIOT
make binfile
```

Connect the board to the STLink according this [tutorial](https://github.com/CampusIoT/tutorial/tree/master/im880a) and then flash the firmware
```bash
export RIOTBASE=~/github/RIOT-OS/RIOT
make flash-only
```

### Build and flash for Seeedstudio LoRa E5 Dev and Mini (`BOARD=lora-e5-dev`)

Build the firmware
```bash
export RIOTBASE=~/github/RIOT-OS/RIOT
make BOARD=lora-e5-dev
```

Connect the board to the SWD connector of the programmer.

Flash : [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html) + STLink v2

> NB: The [`seed_lora-e5-mini`](https://gricad-gitlab.univ-grenoble-alpes.fr/thingsat/seed/-/blob/main/seed_lora-e5-mini/README.md?ref_type=heads) board includes 1x [BME680](https://www.bosch-sensortec.com/products/environmental-sensors/gas-sensors/bme680/) and 2x [ADS1115](https://www.ti.com/product/ADS1115)


### Build and flash for Nucleo WL55JC (`BOARD=nucleo-wl55jc`)

TODO

Two boards : Nucleo WL55JC1 (eu868) and Nucleo WL55JC2 (eu433)


### Build and flash for Discovery B-L072Z-LRWAN1 (`BOARD=b-l072z-lrwan1`)

Build and flash the firmware
```bash
export RIOTBASE=~/github/RIOT-OS/RIOT
make BOARD=b-l072z-lrwan1 LORA_DRIVER=sx1276 flash
```

## Setting DEVEUI APPEUI APPKEY

By default, the DevEUI, the AppEUI and the AppKey are forged using the CPU ID of the MCU. However, you can set the DevEUI, the AppEUI and the AppKey of the LoRaWAN endpoint into the `main.c`.

Optional : Configure the following parameters into the program file `main.c` : `FIRST_TX_PERIOD`, `TX_PERIOD`, `DR_INIT`, `ADR_ON`, `DEBUG_ON` and `SECRET`.
```bash
make SECRET=cafebabe02ffffffcafebabe02000001 binfile
```

The AppKey can be recovered from the DevEUI (displayed at startup) and the SECRET (flashed into the firmware) with the command lines below:
```bash
SECRET=cafebabe02000001cafebabe02ffffff                                         
DevEUI=33323431007f1234                                                         
AppEUI=33323431ffffffff                                                        
SHA=$(echo -n $DevEUI$AppEUI$SECRET | xxd -r -p | shasum -b | awk '{print $1}')
AppKey="${SHA:0:32}"
echo $AppKey
```

The DevEUI, the AppEUI and the AppKey can be set by fixing DEVEUI APPEUI APPKEY into the `make` command
```bash
DevEUI=33323431007f1234
AppEUI=33323431ffffffff
AppKey=f482a62f0f1234ac960882a2e25f971b
make DEVEUI=$DevEUI APPEUI=$AppEUI APPKEY=$AppKey binfile
```

## Enable/Disable the GNSS module

Edit Makefile or overload GPS and STD_BAUDRATE default values 
```bash
GPS=0 make
GPS=1 STD_BAUDRATE=9600 make
GPS=1 STD_BAUDRATE=56700 make
```

> if GPS is enabled, the console baudrate is 9600 b/s and not by default 115200 b/s.

## Enable/Disable the region duty cycle

The region duty cycle can be enabled or disabled in the region file in `bin/pkg/im880b/semtech-loramac/src/mac/region`.

For instance, `~/github/RIOT-OS/RIOT/build/pkg/semtech-loramac/src/mac/region/RegionEU868.h` for region `EU868`

Enable region duty cycle
```c
#define EU868_DUTY_CYCLE_ENABLED                    1
```

Disable region duty cycle
```c
#define EU868_DUTY_CAAYCLE_ENABLED                    0
```

## Console
Connect the board TX pin to USBSerial port and then configure and start `minicom` or `Pyterm` or `tio`.

> if GPS is enabled, the baudrate is 9600 b/s. Else the baudrate is 115200 b/s.

```bash
ll /dev/tty.*
make term
```

or

```bash
ll /dev/tty.*
minicom -s
```

or
```bash
brew install tio
tio -L
tio -b 115200 -m INLCRNL /dev/tty.usbserial-142xxx
```



## Downlink

The application can send a downlink message to the endpoint throught your network server.

Downlink payload can be used for
* sending an ASCII message (port = 1)
* setting the realtime clock of the endpoint (port = 2)
* setting the tx period of the data (port = 3)

### Setup
For CampusIoT:
```bash
ORGID=<YOUR_ORG_ID>
BROKER=lns.campusiot.imag.fr
MQTTUSER=org-$ORGID
MQTTPASSWORD=<YOUR_ORG_TOKEN>
applicationID=1
devEUI=33323431007f1234
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

### Setting the realtime clock of the endpoint
```bash
PORT=202
PAYLOAD=FE0BF6FB4B
mosquitto_pub -h $BROKER -u $MQTTUSER -P $MQTTPASSWORD -t "application/$applicationID/device/$devEUI/tx" -m '{"reference": "abcd1234","confirmed": true, "fPort": '$PORT',"data":"/gv2+0s="}'
```

> The time is the number of seconds since 06/01/1980 (GPS start time). It is unsigned 32 bit-long integer (big endian) LSBF 

The output on the console is:
```bash
...
Received ACK from network                                                                                 
Current RTC time :   2020-05-24 15:03:09                                                                  
Last correction  :   2020-05-24 15:00:49                                                                  
Read temperature= 25.00                                                                                   
app_clock_process_downlink                                                                                
X_APP_CLOCK_CID_AppTimeSetReq                                                                             
Current time    :   2020-05-24 15:03:44                                                                   
RTC time fixed  :   2020-05-24 16:08:43                                                                   
sent_buffer:                                                                                              
```

> Remark: Chirpstack implements the [App Clock Sync Specification](https://lora-alliance.org/resource-hub/lorawanr-application-layer-clock-synchronization-specification-v100). The synchronization is done at the LNS level.


### Rebooting on downlink

The application can send a reboot downlink message to the endpoint throught your network server.

* Send a downlink message on port 64 reboots the board immedialy
* Send a downlink message on port 65 reboots the board after 1 minute
* Send a downlink message on port 66 reboots the board after 1 hour

## Annexes

## TODO
* [ ] Add a downlink message counter (uint16_t), the last downlink fCnt (uint16_t), last downlink RSSI (uint8_t), last downlink LSNR (int8_t) and GPIO_IN bitfield (uint8_t)  into the uplink payload
* [x] Downlink for configuring TxPeriod
* [ ] Downlink for reading GPIO_IN
* [ ] Downlink for setting GPIO_OUT (set or clear) for actuator control
* [ ] Downlink for configuring the DRPWSZ_SEQUENCE
* [ ] Downlink for configuring Confirmation
* [ ] Downlink for rejoining (see Certification Test)
* [ ] Downlink for setting ADR (see Certification Test)
* [ ] Class C endpoint -> `semtech_loramac_set_class(&loramac, LORAMAC_CLASS_C);
* [ ] Class B endpoint -> `semtech_loramac_set_class(&loramac, LORAMAC_CLASS_B);`
* [ ] Multiple ABP endpoints with DEVADDRS define
* [x] Reboot downlink message.
* [ ] Send a confirmed uplink message for confirming the reboot
 
## Base64 utils
Encode base64
```bash
echo 'Hello CampusIoT' | base64
echo '414243442045464748' | xxd -r -p | base64
```

Decode base64
```bash
echo SGVsbG8gQ2FtcHVzSW9UCg== | base64 -d
echo QUJDRCBFRkdI | base64 -d
```

### IMST iM880a DS75LX Connectors

<p align="center">
<img src="images/im880a-ds75lx.jpg" alt="iM880a-DS75LX" width="75%"/>
</p>

![Connector X1](https://github.com/CampusIoT/tutorial/blob/master/im880a/figs/CH340G-to-X2.png)

Connector X1

![Connector X2](https://raw.githubusercontent.com/CampusIoT/tutorial/master/im880a/figs/JTAG-to-X1.png)

Connector X2

> Note: if you do not have an ST-Link v2 flasher, you can use the ST-Link part of a Nucleo board and connect the first 5 pins of the [CN4 SWD connector](https://www.st.com/content/ccc/resource/technical/document/user_manual/98/2e/fa/4b/e0/82/43/b7/DM00105823.pdf/files/DM00105823.pdf/jcr:content/translations/en.DM00105823.pdf) to the X1 connector of the IMST im880 board:


|Nucleo CN4 SWD                            | IMST X1 | Color |
|------------------------------------------|----|------------|
| Pin 1: VDD_TARGET (VDD from application) | 15 | Red        |
| Pin 2: SWCLK (clock)                     | 1  | Brown      |
| Pin 3: GND (ground)                      | 16 | Black/Blue |
| Pin 4: SWDIO (SWD data input/output)     | 2  | Green      |
| Pin 5: NRST (RESET of target STM32)      | 5  | Yellow     |




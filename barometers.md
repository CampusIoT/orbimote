# Barometers for near-space applications

Barometers enable to compute the altitude from temperature and atmospheric pressure ([cf formulea](https://en.wikipedia.org/wiki/Pressure_altitude)).

Typical values ([From](https://www.engineeringtoolbox.com/air-altitude-pressure-d_462.html)):
| Altitude (m)    | Temperature (°C) | Pressure (hPa) |
| -------------:| ------------------:| --------------:|
| 0     |  15 | 1010 |
| 1000  |  ?? | 890  |
| 2000  |  ?? | 790  |
| 4000  |  ?? | 600 |
| 8000  |  ?? | 360 |
| 10000 |  ?? | 240 |
| 15000 |  ?? | 110 |
| 20000 |  ?? | TBC |
| 25000 |  ?? | TBC |
| 30000 |  ?? | TBC |
| 35000 |  ?? | TBC |

## Components
| Component     | Sensors | Range (Resolution) | Breakouts, Boards      | OS support    | Status  |
| ------------- | ------- | ------------------- | ---------------------- | ------------- | ----- |
| [Bosch BMP280](https://www.bosch-sensortec.com/products/environmental-sensors/pressure-sensors/pressure-sensors-bmp280-1.html) | Temperature, Pressure| 300 to 1100 hPa (0.01 hPa), -40-85°C (0.01° C)| TBC | RIOT | Active |
| [Bosch BME680](https://cdn.sparkfun.com/assets/8/a/1/c/f/BME680-Datasheet.pdf) | Humidity, Barometric pressure, Temperature and Indoor Air Quality| 300 to 1100 hPa (0.01 hPa)| TBC | RIOT | Active |
| [STM LPS27HHW](https://www.st.com/en/mems-and-sensors/lps27hhw.html) | Pressure | 260 to 1260 hPa (0.5 hPa RMS) | TBC | TBC | Active |
| [STM LPS25HB](https://www.st.com/en/mems-and-sensors/lps25hb.html) | Pressure | 260 to 1260 hPa (0.01 hPa RMS) | TBC | TBC | Active |
| [STM LPS22HB](https://www.st.com/en/mems-and-sensors/lps22hb.html) | Pressure | 260 to 1260 hPa (0.01 hPa RMS) | TBC | TBC | Active |
| [Murata SCP1000-D01](https://www.sparkfun.com/products/retired/8161) | Pressure | 300 hPa to 1200 hPa, -20C to 70C | TBC | TBC | Obsolete |
| [Murata ZPA2326-0311A-R](https://www.murata.com/en-eu/products/productdetail?partno=ZPA2326-0311A-R) | Pressure | 800 to 1100hPa (0.01 hPa) | TBC | TBC | Active |
| [Amphenol Pressure Sensors](https://www.amphenol-sensors.com/en/mems-pressure-sensors) | Pressure | TBC| TBC | TBC | Active |
| [NXP MPL115A](https://www.nxp.com/products/sensors/pressure-sensors/barometric-pressure-15-to-115-kpa/50-to-115kpa-absolute-digital-pressure-sensor:MPL115A) | Pressure | 500 hPa to 1150 hPa| TBC | TBC | Active |
| [NXP MPL3115A2](https://www.nxp.com/products/sensors/pressure-sensors/barometric-pressure-15-to-115-kpa/20-to-110-kpa-absolute-digital-pressure-sensor:MPL3115A2) | Pressure | 200 hPa to 1100 hPa| TBC | RIOT | Active |
| [TE MS5637-02BA03](https://cdn.sparkfun.com/assets/1/1/d/b/e/MS5637_Datasheet.pdf) | Pressure | 300 hPa to 1200 hPa| TBC | TBC | Active |

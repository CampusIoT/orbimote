/*
 * Codec for Orbimote Field Test Device on LNS (TTN, Helium, Chirpstack)
 *  Author: Didier DONSEZ (Université Grenoble Alpes)
 * Firmware: https://github.com/CampusIoT/orbimote/tree/master/field_test_device
 */

function readUInt16BE(buf, offset) {
    offset = offset >>> 0;
    return (buf[offset] << 8) | buf[offset + 1];
}

function readUInt32BE(buf, offset) {
    offset = offset >>> 0;

    return (buf[offset] * 0x1000000) +
        ((buf[offset + 1] << 16) |
            (buf[offset + 2] << 8) |
            buf[offset + 3]);
}

function readInt32BE(buf, offset, byteLength) {
    offset = offset >>> 0;
    byteLength = byteLength >>> 0;

    var i = byteLength;
    var mul = 1;
    var val = buf[offset + --i];
    while (i > 0 && (mul *= 0x100)) {
        val += buf[offset + --i] * mul;
    }
    mul *= 0x80;

    if (val >= mul) val -= Math.pow(2, 8 * byteLength);

    return val;
}

function readUInt8(buf, offset) {
    offset = offset >>> 0;
    return (buf[offset]);
}

// Chirpstack
// Decode decodes an array of bytes into an object.
//  - fPort contains the LoRaWAN fPort number
//  - bytes is an array of bytes, e.g. [225, 230, 255, 0]
//  - variables contains the device variables e.g. {"calibration": "3.5"} (both the key / value are of type string)
// The function must return an object, e.g. {"temperature": 22.5}
function Decode(fPort, bytes, variables) {

    var o = {};

    if (fPort === 202) {
        // TODO
    } else {
        var size = bytes.length;
        o.size = size;

        if (size < 2) { return o; }

        // Extract LoRa settings.
        o.txpower = readUInt8(bytes, 0);
        o.dataRate = readUInt8(bytes, 1);
        o.gain = (5 - o.dataRate) * 2 + ((o.txpower - 2) * 2 / 3.0);

        if (size < 4) { return o; }
        // Extract temperature.
        o.temperature = readUInt16BE(bytes, 2) / 100.0;

        if (size < 12) { return o; }

        // Value used for the conversion of the position from DMS to decimal.
        var MaxNorthPosition = 8388607; // 2^23 - 1
        var MaxEastPosition = 8388607; // 2^23 - 1

        var Latitude = (readInt32BE(bytes, 4) >> 8);
        var Longitude = (readInt32BE(bytes, 7) >> 8);
        if (!((Latitude === 0) && (Longitude === 0))) {
            // Extract latitude.
            Latitude = Latitude * 90 / MaxNorthPosition;
            o.latitude = Math.round(Latitude * 1000000) / 1000000;

            // Extract longitude.
            Longitude = Longitude * 180 / MaxEastPosition;
            o.longitude = Math.round(Longitude * 1000000) / 1000000;

            // Extract altitude.
            o.altitude = readUInt16BE(bytes, 10);
        }
    }
    return o;
}

// For Helium and TTNv2
function Decoder(bytes, port) {
    return Decode(port, bytes);
}

// TODO TTN v3


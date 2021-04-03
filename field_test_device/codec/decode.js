/*
 * Decoder of Balloon FTD frames for NodeRED
 * Author: Didier DONSEZ, Université Grenoble Alpes
 */

var p = msg.payload;

if(! p.frmPayload) {
    return undefined;
}

var e = p.extra;
if(! e) {
    return undefined;
}

var applicationName = e.applicationName;
if(! applicationName) {
    return undefined;
}

if(applicationName !== "BALLOON_FTD") {
    return undefined;
}

// Decoder here

// Decode decodes an array of bytes into an object.
//  - fPort contains the LoRaWAN fPort number
//  - bytes is an array of bytes, e.g. [225, 230, 255, 0]
//  - variables contains the device variables e.g. {"calibration": "3.5"} (both the key / value are of type string)
// The function must return an object, e.g. {"temperature": 22.5}
function Decode(fPort, bytes, variables) {

  var o = {};

  if(fPort === 202) {
  	// App Clock Synchronization (https://lora-alliance.org/resource-hub/lorawanr-application-layer-clock-synchronization-specification-v100).
  	// Remark: The synchronization is done at the Chirpstack LNS level.
    // TODO
  } else {
    var size = bytes.length;
    
    o.benchmark_id = fPort;
    
    o.size = size;

    if(size < 2) { return o; }

    o.txpower = bytes.readUInt8(0);
    o.dataRate = bytes.readUInt8(1);

    if(size < 4) { return o; }

    o.temperature = bytes.readInt16BE(2)/100.0;

    if(size < 12) { return o; }

    // Value used for the conversion of the position from DMS to decimal.
    const MaxNorthPosition = 8388607; // 2^23 - 1
    const MaxEastPosition  = 8388607; // 2^23 - 1

    // Extract latitude.
    var Latitude = (bytes.readUInt32BE(4) >> 8) & 0x7FFF;
    Latitude = Latitude * 90 / MaxNorthPosition;
    o.latitude = Math.round(Latitude * 1000000) / 1000000;

    // Extract longitude.
    var Longitude = (bytes.readInt32BE(7) >> 8);
    Longitude = Longitude * 180 / MaxEastPosition;
    o.longitude = Math.round(Longitude * 1000000) / 1000000;

    // Extract altitude.
    o.altitude = bytes.readUInt16BE(10);
  }
  return o;
}
// Decode here

var l = p.metadata.network.lora;

var o = Decode(l.port,p.frmPayload,undefined);

msg.payload.object = o;

// For worldmap
msg.payload.img = "ds75lx.png";

return msg;

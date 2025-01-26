# AC Dimmer for PV self-consumption optimization

This project uses a [Shelly Pro 3EM Smart Meter](https://shelly-api-docs.shelly.cloud/gen2/Devices/Gen2/ShellyPro3EM) to measure the PV "Balkonkraftwerk" overproduction and sends the data to an [ESP32](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf). The ESP32 then controls a [RobotDyn AC Dimmer](https://github.com/RobotDynOfficial/Documentation/wiki/AC-Light-Dimmer-Module,-1-Channel,-3.3V_5V-logic,-AC-50_60hz,-220V_110V). The output of the AC Dimmer can then be used to power e.g. an electro heater.

## Preliminary Considerations

I thought about building my own dimmer but then I discovered the RobotDyn AC Dimmer Modules. They are cheaper than building it yourself and there is a [library already available on Github](https://github.com/RobotDynOfficial/RBDDimmer). So I decided to "stand on the shoulders of Giants instead of reinventing the wheel" 😉

## Milestones

- [X] Select & Buy Hardware 
- [X] Connect the ESP to the Shelly and fetch data
- [ ] Connect the AC Dimmer to the ESP and check functionality
- [ ] Programm the control software to use only the overproduction
- [ ] Connect everything together and add a housing
- [ ] Optional: Add a measurement method and store the data
- [ ] Final Tests

## Electro Heater

A 1.5 kW electro heater was the first test device that I want to power with my overproduction. As I am working on this it is winter and warm air is needed. In summer one can think about replacing the electro air heater with a heating blade for a water boiler. (This is more complicated to install because I have no experiences with plumbing work.) 

While testing the air heater I realized that I need min. 400W so that the fan starts spinning. I decided to decouple the fan from the heating resistors. So now I have to plugs: One only for the fan and one for the rest of the device. 

### Energy Consumption

- Fan: Min. 20 W
- Resistors: Stage 1: Max: 700W, Stage 2: Max: 1500 W

So the software should start the fan at 20 W overproduction and then the AC dimm can start the heating resistors. Perhaps a threshold of a minimum (10 W) feed-in would be useful to avoid electricity costs at all.

## Shelly API

I did already a Shelly data readout in my [Shelly3EM_to_InfluxDB](https://github.com/PaulusElektrus/Shelly3EM_to_InfluxDB/tree/master) Repo, see [this file](https://github.com/PaulusElektrus/Shelly3EM_to_InfluxDB/blob/master/src/main.cpp). Funny to see my progress since these beginnings. The old code definitifely needs a rework...

To get the data from Shelly device simply use the rpc endpoint with a http client: `http://shelly_ip_address/rpc/EM.GetStatus?id=0`

You will get a response in json format. This needs to be unpacked to get the raw values.

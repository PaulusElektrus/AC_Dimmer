# AC Dimmer for PV self-consumption optimization

This project uses a [Shelly Pro 3EM Smart Meter](https://shelly-api-docs.shelly.cloud/gen2/Devices/Gen2/ShellyPro3EM) to measure the PV "Balkonkraftwerk" overproduction and sends the data to an [ESP32](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf). The ESP32 then controls a [RobotDyn AC Dimmer](https://github.com/RobotDynOfficial/Documentation/wiki/AC-Light-Dimmer-Module,-1-Channel,-3.3V_5V-logic,-AC-50_60hz,-220V_110V). The output of the AC Dimmer can then be used to power e.g. an electro heater.

## Preliminary Considerations

I thought about building my own dimmer but then I discovered the RobotDyn AC Dimmer Modules. They are cheaper than building it yourself and there is a [library already available on Github](https://github.com/RobotDynOfficial/RBDDimmer). So I decided to "stand on the shoulders of Giants instead of reinventing the wheel" 😉

## Milestones

- [X] Select & Buy Hardware 
- [ ] Connect the ESP to the Shelly and fetch data
- [ ] Connect the AC Dimmer to the ESP and check functionality
- [ ] Programm the control software to use only the overproduction
- [ ] Connect everything together and add a housing
- [ ] Optional: Add a measurement method and store the data
- [ ] Final Tests

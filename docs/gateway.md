## Generic - configuration and logging
----------------------

  * [device REST protocol](docs/restProtocol.md)
  * [device MQTT protocol](docs/mqttProtocol.md)
  * [device configuration](docs/config.md)
  * [device logging](docs/logging.md)


# Getting started
----------------------

## Gateway initial setup
![Thingworks gateway](docs/image/thingworks.jpg "Thingworks gateway")

### preparing micro SD card with prebuilt image.
[Download Thing+ Gateway image here 20140725](https://www.dropbox.com/s/j4vzm3izs9juz9f/bbb_tp_20140725_2.img.zip)

### initial use
 * BBB side
   * power on BBB: micro sd card, ethernet, and power are connected.
     * before turning on the board, you need to insert the sd card and connect the board by ethernet.
   * wait until all 4 leds stop blinking and remain solid. It will take around 10 min.
   * turn off BBB
   * remove micro sd card
   * turn on BBB (ready to sensor setup)
 * PC side
   * browsing http://[BBB’s IP address]
     * ID / PW -> admin / admin123
     * Settings->'Software Control'->'Gateway Update' button (when you can access normally http://[BBB’s IP address])
       * getting the latest gateway S/W version

## Thing+ service how to
  * Open https://thingplus.net
  * (User Registration then) Login as {your id}

## gateway registration to server
  * (Your login id should be [service, system, site] admin)
  * Go to Gateway Management menu then select New (or +)
  * Input 'Gateway ID' which is Mac-address of gateway to register and 'Gateway Name'
    Select 'Site Code'
  * Click 'Register Gateway & Download Certification' button
  * Store a certificate file

## gateway activation
  * Open http://[BBB’s IP address]
  * Settings->'Register Gateway Cert'
  * Select a certificate file
  * Click 'Update Certificate'
  * Check REST, MQTT Channel

## Sensor setup
 * Requirement
   * Network (Wired or Wireless) connection

 * VDD 5V vs. SYS 5V
   * VDD_5V
     * 1A 
     * VDD_5V is the main power supply from the DC input jack. So this voltage is not present when the board is only powered via USB
   * SYS_5V
     * 0.25A

 * Sensors
   1. [ds18b20 (1-wire)](http://www.ermicro.com/blog/wp-content/uploads/2009/10/picaxe_11.jpg)
     : Temperature
     * VDD_3V3
     * DGND
     * GPIO 2
     * 4.7K Pull-Up Resistor (between VDD_3V3 and DGND)
   2. [DHT11 Sensor V2](http://www.dfrobot.com/wiki/index.php/DHT11_Temperature_and_Humidity_Sensor_V2_SKU:_DFR0067)
     : Humidity
     * SYS_5V or VDD_5V
     * DGND
     * GPIO 27
   3. [htu21d (I2C)](https://www.sparkfun.com/products/12064)
     : Humidity
     * VDD_3V3
     * DGND
     * I2C2_SCL
     * I2C2_SDA
     * Address: 0x40
   4. [photocell (I2C)](http://stackoverflow.com/questions/10611294/reading-analog-in-on-beaglebone-avoiding-segmentation-fault-error)
     : Light
     * VDD_ADC
     * GDNA_ADC
     * AIN0
     * 10K Pull-Down Resistor (between AIN0 and DGND) / (sensor : between VDD_ADC and AIN0)
   5. [BH1750FVI](http://www.dfrobot.com/index.php?route=product/product&product_id=531)
     : Light (digital)
     * VDD_3V3
     * DGND
     * I2C2_SCL
     * I2C2_SDA
     * (Optional) ADD : if ADD is HIGH then address will be changed to 0x5c from 0x23
     * Address: 0x23 (0x5C)
   6. [magnetic switch]
     * DGND
     * GPIO 46 or 61 or 115
   7. [motion detector]
     * SYS_5V or VDD_5V
     * DGND
     * GPIO 46 or 61 or 115
   8. [noise detector]
     * SYS_5V or VDD_5V
     * DGND
     * GPIO 46 or 61 or 115
   9. [RGB]
     * DGND
     * GPIO 67
  10. [power switch](http://www.dfrobot.com/index.php?route=product/product&product_id=64)
     * SYS_5V or VDD_5V
     * DGND
     * GPIO 67
   

 * Procedure
   * Step 1
   	 - Connect all sensors
   	 - ![BBB GPIO](docs/image/bbb_gpio.jpg "BBB GPIO")
   * Step 2
     - Open http://[BBB’s IP address]/#/home

     - ![Gateway home](docs/image/gatewayui_home.png "Gateway home")

   * Step 3
     - Add sensor information with plus button at home
     - Select(click) agent type and fill ALL attributes
     	- If sensor’s id is detected automatically, you can select the ID
     	- If not, you must input the sensor ID or the Sensor ID is assigned
     - Test
     - Save and add additional sensors
     - ![Gateway sensor](docs/image/gatewayui_sensor.png "Gateway sensor")

## 3G setup
![Thingworks modem](docs/image/bbb_modem_z.jpg "Thingworks modem")
 * Requirement
   * Update the gateway software
   * Make sure the modem on your PC works
   * LAN connection for the first time configration
 * Procedure
   * Step 1
     - Connect all your asset as shown above
     - Power on
   * Step 2
     - Browsing http://[BBB’s IP address]
     - Fill out modem information in Setting Menu
     - Reboot (No LAN requires now)
 * Example procedure (carrier: sk telecom network, without ethernet connection)
   * Step 1 PC side: setup network over USB
     - install driver on your pc (Windows, Mac OS X, Linux)
       - see http://beagleboard.org/static/beaglebone/latest/README.htm
     - browser open http://192.168.7.2
   * Step 2 Gateway side: modem setting
     - HOME -> Modem -> Modem Configuration:
        - APN: web.sktelecom.com
        - id: skt
        - pass: skt
        - SIM pass: 1234
        - check enabled
     - Press "Change Configuration" button
     - ![modem setting](docs/image/bbb_modem_setting.png "modem setting")
   * Step 3 Gateway side: remember mac adress
     - HOME->Cloud Connection->Mac address
   * Step 4 PC side: [gateway registration to server](#gateway-registration-to-server)
   * Step 5 Gateway side: [gateway activation](#gateway-activation)
   * Step 6 Gateway Side:
     - SETTINGS->Gateway Control ->reboot
     - wait 5 min

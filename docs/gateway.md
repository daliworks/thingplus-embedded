# Install Thing+ Gateway
  * [How to install Thing+ Gateway](/docs/gateway/install.md)

# Configuration and logging
  * [Configuration Parameters](/docs/gateway/config.md)
  * [Logging How to](/docs/gateway/logging.md)


# Thing+ Gateway UI howtos
## Sensor manual registration(Not recommended, for testing purpose only during development phase)

### Step 1
  - Connect sensors and actuators.
  - Beaglebone Black example:

 ![BBB GPIO](/docs/image/bbb_gpio.jpg "BBB GPIO")

### Step 2
  - Web browser open http://[BBB’s IP address]/#/home

  ![Gateway home](/docs/image/gatewayui_home.png "Gateway home")

### Step 3
   - Add sensor information with plus button at home
   - Select(click) agent type and fill ALL attributes
    - If sensor’s id is detected automatically, you can select the ID
    - If not, you must input the sensor ID or the Sensor ID is assigned
   - Test
   - Save and add additional sensors

  ![Gateway sensor](/docs/image/gatewayui_sensor.png "Gateway sensor")

## 3G Modem setup

![Thingworks modem](/docs/image/bbb_modem_z.jpg "Thingworks modem")
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
     - ![modem setting](/docs/image/bbb_modem_setting.png "modem setting")
   * Step 3 Gateway side: remember mac adress
     - HOME->Cloud Connection->Mac address
   * Step 4 PC side: [gateway registration to server](#gateway-registration-to-server)
   * Step 5 Gateway side: [gateway activation](#gateway-activation)
   * Step 6 Gateway Side:
     - SETTINGS->Gateway Control ->reboot
     - wait 5 min

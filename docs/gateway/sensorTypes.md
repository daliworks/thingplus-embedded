Sensor types definition
------------------------

Each sensor type is defined with [json-schema](http://json-schema.org/)
Each field means as follows.

 * unit
    * °C, %, meter, km and etc
    * can be empty string
 * type
    * `integer`
        * any integer or
        * possible values can be defined in "enum" field
    * `number`
        * any double number or
        * possible values can be defined in "enum" field
    * `string`
        * any string value or
        * possible values can be defined in "enum" field
    * `object`
        * coords json string as specified in [html5 geolocation](http://dev.w3.org/geo/api/spec-source.html)        
        * latitude and longitude are mandatory. those can be abbreviated as lat and lng.
        * example value: { latitude: -50, longitude: 100 } or { lat: -50, lng: 100 }
 * enum
     * arrary of possible values.
 * maximum, minimum
     * array of min, max, step. e.g. [0, 100, 1] : natural number from zero to 100
 * decimalPlace
   * how many digits in display below deciman point. 
 * category
   * `sensor` series or event type sensor which can have value. ex) temperature sensor
   * `actuator` actuaor which can do action. ex) power switch, temperature controller
 * dataType
   * `series` 
   sensor values are measured in periodic basis. ex) temperature values are sensored every 10secs(sensing interval) and 6 values are reported together every minute(reporting interval).
   * `event` 
   whenever there's an event. it is reported immediately. ex) fire alarm. 


Sensor types currently available
----------------------------------
```json
[
  {
    "id": "number",
    "unit": "",
    "type": "number",
    "decimalPlace": 1,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "string",
    "unit": "",
    "type": "string",
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "complex",
    "unit": "",
    "type": "object",
    "contentType": {
      "properties": {
        "0": {
          "type": "string"
        },
        "1": {
          "type": "string"
        },
        "2": {
          "type": "string"
        }
      },
      "display": [
        "0",
        "1",
        "2"
      ]
    },
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "temperature",
    "unit": "°C",
    "type": "number",
    "minimum": -273,
    "maximum": 300,
    "decimalPlace": 1,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "humidity",
    "unit": "%",
    "type": "integer",
    "minimum": 0,
    "maximum": 100,
    "decimalPlace": 0,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "onoff",
    "unit": "lvl",
    "type": "integer",
    "category": "sensor",
    "enum": [
      0,
      1
    ],
    "dataType": "event"
  },
  {
    "id": "motion",
    "unit": "",
    "type": "integer",
    "category": "sensor",
    "enum": [
      0,
      1
    ],
    "dataType": "event"
  },
  {
    "id": "light",
    "unit": "lux",
    "type": "integer",
    "minimum": 0,
    "decimalPlace": 0,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "location",
    "unit": "°",
    "type": "object",
    "contentType": {
      "properties": {
        "lat": {
          "type": "number",
          "minimum": -90,
          "maximum": 90,
          "decimalPlace": 6
        },
        "lng": {
          "type": "number",
          "minimum": -180,
          "maximum": 180,
          "decimalPlace": 6
        },
        "alt": {
          "type": "number"
        }
      },
      "display": [
        "lat",
        "lng",
        "alt"
      ]
    },
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "accelerometer",
    "unit": "m/s^2",
    "type": "object",
    "contentType": {
      "properties": {
        "x": {
          "type": "number",
          "decimalPlace": 6
        },
        "y": {
          "type": "number",
          "decimalPlace": 6
        },
        "z": {
          "type": "number",
          "decimalPlace": 6
        }
      },
      "display": [
        "x",
        "y",
        "z"
      ]
    },
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "batteryGauge",
    "unit": "%",
    "type": "integer",
    "minimum": 0,
    "maximum": 100,
    "decimalPlace": 0,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "ph",
    "unit": "pH",
    "type": "number",
    "minimum": 0,
    "maximum": 14,
    "decimalPlace": 2,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "co2",
    "unit": "ppm",
    "type": "integer",
    "minimum": 0,
    "decimalPlace": 0,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "co",
    "unit": "ppm",
    "type": "integer",
    "minimum": 0,
    "decimalPlace": 0,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "voc",
    "unit": "ppm",
    "type": "integer",
    "minimum": 0,
    "decimalPlace": 0,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "power",
    "unit": "kWh",
    "type": "number",
    "minimum": 0,
    "decimalPlace": 2,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "durationPower",
    "unit": "kWh",
    "type": "number",
    "minimum": 0,
    "decimalPlace": 3,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "current",
    "unit": "A",
    "type": "number",
    "minimum": 0,
    "decimalPlace": 3,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "voltage",
    "unit": "V",
    "type": "integer",
    "minimum": 0,
    "decimalPlace": 0,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "frequency",
    "unit": "Hz",
    "type": "number",
    "minimum": 0,
    "decimalPlace": 1,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "powerFactor",
    "unit": "pf",
    "type": "number",
    "minimum": 0,
    "decimalPlace": 2,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "dust",
    "unit": "µg/m^3",
    "type": "integer",
    "minimum": 0,
    "decimalPlace": 0,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "density",
    "unit": "g/m^3",
    "type": "integer",
    "minimum": 0,
    "decimalPlace": 0,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "count",
    "unit": "ea",
    "type": "integer",
    "minimum": 0,
    "decimalPlace": 0,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "countEvent",
    "unit": "ea",
    "type": "integer",
    "minimum": 0,
    "decimalPlace": 0,
    "category": "sensor",
    "dataType": "event"
  },
  {
    "id": "timeDuration",
    "unit": "sec",
    "type": "integer",
    "minimum": 0,
    "decimalPlace": 0,
    "category": "sensor",
    "dataType": "event"
  },
  {
    "id": "precipitationProbability",
    "unit": "%",
    "type": "integer",
    "minimum": 0,
    "maximum": 100,
    "decimalPlace": 0,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "windSpeed",
    "unit": "m/s",
    "type": "number",
    "decimalPlace": 2,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "rain",
    "unit": "mm/min",
    "type": "integer",
    "minimum": 0,
    "decimalPlace": 0,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "pressure",
    "unit": "bar",
    "type": "number",
    "minimum": 0,
    "decimalPlace": 1,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "noise",
    "unit": "dB",
    "type": "integer",
    "minimum": -100,
    "maximum": 100,
    "decimalPlace": 0,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "speed",
    "unit": "m/s",
    "type": "number",
    "minimum": 0,
    "decimalPlace": 3,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "weight",
    "unit": "g",
    "type": "integer",
    "minimum": 0,
    "decimalPlace": 0,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "length",
    "unit": "mm",
    "type": "integer",
    "minimum": 0,
    "decimalPlace": 0,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "color",
    "unit": "",
    "type": "string",
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "vibration",
    "unit": "V",
    "type": "number",
    "minimum": 0,
    "decimalPlace": 2,
    "category": "sensor",
    "dataType": "series"
  },
  {
    "id": "led",
    "unit": "",
    "type": "object",
    "contentType": {
      "properties": {
        "cmd": {
          "type": "string"
        },
        "result": {
          "type": "string"
        },
        "error": {
          "type": "object",
          "properties": {
            "code": {
              "type": "number"
            },
            "message": {
              "type": "string"
            }
          }
        }
      },
      "display": [
        "cmd"
      ]
    },
    "category": "actuator",
    "dataType": "event"
  },
  {
    "id": "powerSwitch",
    "unit": "",
    "type": "object",
    "contentType": {
      "properties": {
        "cmd": {
          "type": "string"
        },
        "result": {
          "type": "string"
        },
        "error": {
          "type": "object",
          "properties": {
            "code": {
              "type": "number"
            },
            "message": {
              "type": "string"
            }
          }
        }
      },
      "display": [
        "cmd"
      ]
    },
    "category": "actuator",
    "dataType": "event"
  },
  {
    "id": "camera",
    "unit": "",
    "type": "object",
    "contentType": {
      "properties": {
        "cmd": {
          "type": "string"
        },
        "result": {
          "type": "object",
          "properties": {
            "contentType": {
              "type": "string"
            },
            "content": {
              "type": "string"
            },
            "url": {
              "type": "link"
            }
          }
        },
        "error": {
          "type": "object",
          "properties": {
            "code": {
              "type": "number"
            },
            "message": {
              "type": "string"
            }
          }
        }
      },
      "display": [
        "cmd"
      ],
      "link": [
        "result.url"
      ]
    },
    "category": "actuator",
    "dataType": "event"
  },
  {
    "id": "temperatureController",
    "unit": "",
    "type": "object",
    "contentType": {
      "type": "object",
      "properties": {
        "cmd": {
          "enum": [
            "set",
            "get"
          ]
        },
        "result": {
          "type": "object",
          "properties": {
            "targetTemperature": {
              "type": "number",
              "unit": "°C"
            },
            "coolingDeviation": {
              "type": "number",
              "unit": "°C"
            },
            "heatingDeviation": {
              "type": "number",
              "unit": "°C"
            }
          }
        },
        "error": {
          "type": "object",
          "properties": {
            "code": {
              "type": "integer"
            },
            "message": {
              "type": "string"
            }
          }
        }
      },
      "display": [
        "cmd"
      ]
    },
    "category": "actuator",
    "dataType": "event"
  }
]
```

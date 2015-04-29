## base url

    https://hostname.domain/api/v1

## Node(Gateway, sensor) value and status
### Sensor value
#### /gateways/:owner/sensors/:id/series

  * methods
    * GET
    * PUT

    Note: POST and DELETE are not supported

  * params
    * owner: gateway id
    * id: sensor id


  * query (GET)
    * dataStart: optional. 값을 포함(inclusive). date.toISOString() format.
    * dataEnd: optional. 값을 포함하지 않음(non-inclusive). current time if not provided. date.toISOString() format.
    * interval: optional. 15m, 1h, 2h etc. dbconfiguratio되어 있는 수의 배수로 줘야 한다.
       * 만일 15m으로 db configuration 되어 있다면, 15m, 30m, 45m, 1h, 2h, 3h, 4h, 6h, 12h 등이 가능함.
    * count : optional. default max 2000 if not provide params.

    Note: If there is no query, the last value is provided.

```js
> curl https://www.thingplus.net/api/v1/gateways/c8a030a63a48/sensors/28-000003b861e2/series

{
  "type": "temperature",
  "name": "T61e2",
  "driverName": "ds18b20",
  "model": "ds18b20",
  "owner": "c8a030a63a48",
  "mtime": "1402639230553",
  "ctime": "1395923465773",
  "status": "status.sensor.-6dX5m",
  "series": {
    "type": "series",
    "time": "1402985014946",
    "value": "29.875",
    "series": "28-000003b861e2",
    "owner": "c8a030a63a48",
    "sensor": "28-000003b861e2",
    "mtime": "1402985017247",
    "ctime": "1402639230521",
    "id": "series.sensor.9izKap"
  },
  "category": "sensor",
  "id": "28-000003b861e2"
}
```  

```js  
> curl https://www.thingplus.net/api/v1/gateways/c8a030a63a48/sensors/28-000003b861e2/series?dataStart=2014-06-15T06:41:00.000Z&dataEnd=2014-06-17T06:41:00.000Z&interval=1m

{
  "type": "temperature",
  "name": "T61e2",
  "driverName": "ds18b20",
  "model": "ds18b20",
  "owner": "c8a030a63a48",
  "mtime": "1402639230553",
  "ctime": "1395923465773",
  "status": "status.sensor.-6dX5m",
  "series": {
    "type": "series",
    "time": "1402987470424",
    "value": "29.375",
    "series": "28-000003b861e2",
    "owner": "c8a030a63a48",
    "sensor": "28-000003b861e2",
    "mtime": "1402987481648",
    "ctime": "1402639230521",
    "id": "series.sensor.9izKap",
    "data": [
      34.374832,
      1402814460000,
      34.583,
      1402814520000,
      ...
      29.385334,
      1402987140000,
      29.4122,
      1402987200000
    ],
    "_meta": {
      "count": 2880,
      "interval": "1m"
    }
  },
  "category": "sensor",
  "id": "28-000003b861e2"
}

```  

  * body (PUT)
    * arrary of time and value object
      * first object is oldest and last object is latest
      * time is number of milliseconds since midnight Jan 1, 1970
    * [ { time: 1402986075957, value: 30.34 }, ... ]

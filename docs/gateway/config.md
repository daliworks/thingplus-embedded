# Device Configurations Overview
This doc describes how to set default system configuration and change them at runtime.

see [node-config doc](http://lorenwest.github.io/node-config/latest/)
 for tech details.

# configuration files:
  * `device/config/default.js`
    * default configuration file cannot be changed at runtime
    * Note: in js format
  * `device/config/runtime.json`
    * runtime configuration change applied
    * this change can be done by application or manually.
    * Note: in json format.
  
## Authentication Info
  * default login id and password(admin/admin123)
  * key : user id
  * value: crypto.createHash('md5').update(yourId:realm:password').digest('hex')

```javascript
  Auth: {
    admin: '93f77339234c8e6717412baad4f53dae' //crypto.createHash('md5').update('admin:Sensor.js:admin123').digest('hex'),
  },  
```

## Gateway Info
  * name
    * gateway name, for now overridden by server
  * reportInterval
    * interval between sensor data reports.
    * sensor data gathering interval is configured at each its driver.
    * interval in ms.
  * CERT_FILE_PATH
    * certification file path
    * default at `device/ssl/`
  * port
    * web service port
  * logBaseDir
    * logging path will be `/var/log/thingplus/device_{hostname}.log`
    * more details about [logging here](doc/loggind.md)

```javascript
  Gateway: {
    name: os.hostname(),
    firstDelay: 5000,
    reportInterval: 60000,
    CERT_FILE_PATH: path.normalize(__dirname + '/..' + '/ssl/cert.p12'),
    port: 80,
    logBaseDir: '/var/log/thingplus/',
  },
```

## Server Info
  * timeSyncCheckInterval
    * time sync interval if system time is not synced yet.
    * default 1 min
  * mqtt
    * protocol : either `mqtts`(mqtt over ssl) or `mqtt`
    * host
    * port
    * retryConnectInterval : connection retry interval in ms
    * ackTimeout: packet ACK(from server) waiting time in ms
  * server
    * protocol: either `https` or `http`
    * host
    * port

```javascript
  Server: {
    timeSyncCheckInterval: 60 * 1000, // 1min
    mqtt: {
      protocol: 'mqtts',
      host: 'mqtt.thingplus.net',
      port: '8883',
      retryConnectInterval: 1*60*1000, //1min
      ackTimeout: 30*1000, // 30 sec
    },
    service: {
      protocol: 'https',
      host: 'www.thingplus.net',
      port: '8443'
    },
  },
```

## Offine Store Info
In case of off-line, Sensor data is stored into local persistent storage.

  * baseDir & currentFile
    * default db file location is `/var/lib/thingplus/db.dirty`
  * logFlushFilesystemLimit
    * available filesystem space in percent
    * archived log files are removed if this limit is reached.
  * * cleanUpInterval
    * log flush check interval in secs.
  * storeDisableFilesystemLimit
    * available filesystem space in percent
    * stopping storing sensor data if this limit is reached.
    * zero means disabling this limit.
  * storeDisableMemoryLimit
    * available memory space in percent
    * stopping storing sensor data if this limit is reached.
    * default `0`, zero means disabling this limit.
  * rebootInOfflineInterval
    * time in secs.
    * reboot if offline duration > this limit.
  * storeBaseDirMountPoint
    * mount point where store data file is placed. It is used for disk free size check.
    * default ```/```
  * logBaseDirMountPoint
    * mount point where log file is placed. It is used for disk free size check.
    * default ```/```
  * memoryOnly
    * default `false`; false means use a file to store offline data.
    * true means offline data are stored into memory instead of persistent file.
```javascript
  Store: {
    baseDir: '/var/lib/thingplus/',
    currentFile: 'db.dirty',
    cleanUpInterval: 60 * 60 * 1000, // 1hour
    logFlushFilesystemLimit: 10, //10%
    storeDisableFilesystemLimit: 3, //3%
    storeDisableMemoryLimit: 0, //0%
    rebootInOfflineInterval: 6*3600, //reboot on existence of 6 hours old offline data
    memoryOnly: false, //false by default
  },
```

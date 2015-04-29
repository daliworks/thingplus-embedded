logging
==========
[svlogd](http://smarden.org/runit/svlogd.8.html) or logrotate is used. Usually svlogd is for low-end system and logrotate is for high-end system.


svlogd
--------------

## configuration
Keeping 5 logfiles which are upto 500KB in file size.
```
$ cat config
s500000
n5
```

```current``` file is latest log file.

Viewing the log: ``` tail -f current```

## svlogd install
[busybox](http://www.busybox.net/) includes svlogd support.
Build configuration file for busybox-1.23.0 [here](https://gist.github.com/authere/7d40593bb79dc9ca8a15)

logrotate
--------------

## log file path
  * current log file: `/var/log/thinigplus/device_{hostname}.log`
  * archived log files: `/var/log/thingplus/device_{hostname}.logYYYYMMDD_{seconds}`

## log archive settings
  * /etc/logrotate.d/thingplus
```
  /var/log/thingplus/*.log {
  maxsize 300k
  rotate 20
  missingok
  notifempty
  sharedscripts
  copytruncate
  dateext
  dateyesterday
  dateformat %Y%m%d_%s
}
```

## configuration file - `device/logger_cfg.json`

  * levels
    * change log severity at runtime. applied every 30 sec
    * log severities: `FATAL, ERROR, INFO, DEBUG, TRACE`
  * more details at [log4js](https://github.com/nomiddlename/log4js-node)

```json
{
  "levels": {
    "Main":         "INFO",
    "HttpBinder":   "WARN",
    "Primitive":    "WARN",
    "RscTree":      "WARN",
    "Routes":       "WARN",
    "Applib":       "WARN",
    "Sensor":       "INFO",
    "MQTT":         "INFO",
    "User":         "WARN",
    "Store":        "INFO",
    "Gateway":        "INFO",
    "Server":        "INFO",
    "Sensors":        "INFO"
  },
  "replaceConsole": false,
  "appenders": [
    { "type": "console" },
    { "type": "../../../mqtt_log_appender", "level": "FATAL"}
  ]
}
```

## how to view log file
  * realtime log viewing
```command
> tail -f /var/log/thingplus/device_debian.log
```
  * less with coloring
```command
> less -R /var/log/thingplus/device_debian.log
```

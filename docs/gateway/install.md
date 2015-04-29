System specific preparations
-------------------------------

## Ubuntu 
### nodejs install
see: https://github.com/nodesource/distributions#deb
```shell
$ curl -sL https://deb.nodesource.com/setup | sudo bash -
$ sudo apt-get install nodejs
```
### stunnel(version 4 or later) install
Rsync is used for software update. Stunnel make rsync communication secure.
```
$ sudo apt-get install stunnel
```

## OSX Darwin 

install coreutils and stunnel.
```
$ brew install coreutils
$ brew install stunnel
$ brew install gnu-getopt
```

gnu readlink and getopt in bin path, add path:
```
/usr/local/opt/coreutils/libexec/gnubin:/usr/local/opt/gnu-getopt/bin
```

Install
-------------

## Install Thing+ Gateway
Download the installation script with 'curl -O' or 'wget' and execute it after editting your config file. You can delete this script  and config file after installation. 
```
# Download with curl or wget
$ curl -O https://gist.githubusercontent.com/authere/21599623b081a8e72825/raw/76a06cf3c032181e01266ba0517eb750508b4ced/thingplus_install.sh
$ wget https://gist.githubusercontent.com/authere/21599623b081a8e72825/raw/76a06cf3c032181e01266ba0517eb750508b4ced/thingplus_install.sh

$ chmod a+x thingplus_install.sh
$ ./thingplus_install.sh
$ vi config
$ cat config
MODEL="xxxx" #darwin, w32, mfox or etc.
RSYNC_SERVER="rsync.thingbine.com"
RSYNC_USER="_get_user_id_"
RSYNC_PASSWORD='*get_password*'
DEST_DIR="$HOME/thingplus"
$ ./thingplus_install.sh
```

## directory structure
  - $HOME/thingplus/ : Installation root as the directory specified at DEST_DIR in the install config.
    - log/ : You can see current log as ```tail -f ./log/thingplus.log```. Keeping 10 logfiles with 500KB size. See logging.md for changing logging severity.
    - store/ : simple offline db store.
    - config/ : more details in config.md
      - runtime.json : override configuration here. 
      - cert.p12 : private key file for accessing Thing+. It is installed thru Thing+ Gateway web UI.
    - thingplus-gateway/ : excutables.
    - thingplus.sh : run script; 
    ```
Usage: ./thingplus.sh {start|stop|restart}
    ```
    - sw_update.sh : software update script. You need to restart service to apply changes. You can do this at Gateway web UI as well.
    ```
$ ./sw_update.sh # software update and 
$ ./thingplus.sh restart # restart service
    ```

## Thing+Gateway Web access
 - http port 80 # bydefault 80, you can override it e.g. 8088
 - auth id/pass: admin/admin123
   
## Testing

  - git clone https://github.com/daliworks/sensorjs-jsonrpc.git
  - cd {sensorjs-jsonrpc}
  - npm install --force
  - run examle code in ```{sensorjs-jsonrpc}/example/jsonrpc-nodejs``` 
```
 $ node server.js &
```
  - run Thing+ gateway. 
```
 $ $HOME/thingplus/thingplus.sh start
```
  - Register gateway at Thing+ UI(www.thingplus.net), then sensors are auto registered.
  
Thing+ UI -> Gateway management -> + button right-upper corner 

see [this picture](https://www.evernote.com/shard/s5/sh/6903f1da-e4c7-4d7c-9346-d1e67be15db3/5c9f54b4a6d29d25e4c185c4998648f1)

Rename sensors see [this picture](https://www.evernote.com/shard/s5/sh/6e3f37ba-3963-4460-9cc8-cc62d7c73879/16f218aa57deb6aa1d63240b68686265)


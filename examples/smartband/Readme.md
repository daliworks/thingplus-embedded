### 1. 개발환경 구축
* 사용하는 라이브러리
  - json-c : https://github.com/json-c/json-c/wiki 
     - 설치방법은 위키 참조
  - libcurl : https://curl.haxx.se/libcurl/c/
     - ``` opkg install libcurl ```
  - openssl : https://www.openssl.org/
     - ``` opkg install openssl ```

<font color="red">
### 타겟보드 및 Toolchain에 위 라이브러리가 설치되어 있어야 합니다.    </font>

### 2. 빌드방법
메일로 드린 코드를 Edison 보드에 복사하신 후 아래와 같이 빌드하시면 됩니다.

* Library

```
$ cd libs/src/http
$ make
```

* Smartband App

```
$ cd apps/example/smartband
$ make
```

### 3. 스마트 밴드 요구사항
1. 모든 밴드는 Thingplus Portal에서 게이트웨이 등록을 해야 합니다. Thingplus는 스마트밴드를 게이트웨이로 간주하기 때문입니다.
  - 게이트웨이 등록 시 사용할 게이트웨이 아이디는 스마트밴드의 MAC 주소입니다.
  - 게이트웨이 모델은 'Open Source Gateway'입니다.

1. 스마트 밴드는 Thingplus가 발급한 APIKEY를 밴드별로 가지고 있어야 합니다.
1. 스마트 밴드가 tagging이 되면, 자신의 MAC 주소, APIKEY, 베터리, 걸음 수를 밴드 리더기(Edison 보드)에게 전송을 해야합니다.
1. 위치 정보는 밴드 리더기(Edison 보드)별로 가지고 있습니다.


### 4. API 설명

-  
  ```
    int thingplus(char* gw_id, char* apikey, char* band_name, int step, int battery, char* location); 
    
  ```
  - 파일 : example/smartband/ThingPlus.c
  - 내용 : 스마트 밴드의 디바이스와 센서를 Thingplus에 등록하고, 센서 데이터를 전송합니다.
  - args
     - gw_id : 게이트웨이 ID(밴드의 MAC 어드레스)
     - apikey : 게이트웨이 등록 시 발급받는 APIKEY
     - band_name : 스마트 밴드 이름
     - step : 걸음 수
     - battery : 베터리 양(단위 %)
     - location : 밴드 리더기가 설치된 위치
 - return
     - 0 : Success
     - < 0 : Failed

### 5. Band Application
- main() 함수(@ example/smartband/ThingPlus.c)에서 스마트 밴드 태깅을 기다리다 태깅이 되면 thingplus()함수를 호출하시기 바랍니다.

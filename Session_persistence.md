# Session Persistence

mosquitto broker의 미전송 메시지, session 정보는 mosquitto broker 내부에 저장되어 있다.
mosquitto broker persistence 설정을 통해 mosquitto broker 내부에 저장되어 있는 정보를 broker 밖의 별도의 파일에 저장할 수 있다.

## persistence 설정 

mosquitto broker의 설정파일에 아래와 같은 설정을 추가한다.

persistence true
persistence_location {persistence file이 저장될 경로}
persistence_file {저장될 persistence file 이름}

autosave_interval {자동 저장을 수행할 초 주기}

## persistence 특징

여러 실험 결과 mosquitto persistence의 다음과 같은 특징을 발견하였다.

1. 하나의 broker가 동시에 여러 persistence file을 참조하는 것은 불가능하다.
설정 파일에 persistence_file {저장될 persistence file 이름} 2개를 설정하면 중복된 설정이 존재한다는 오류 메시지와 함께 mosquitto broker가 실행이 되지 않는다.

persistence_file mosquitto1.db, mosquitto2.db
위와 같이 설정하면 'mosquitto1.db, mosquitto2.db'라는 이름의 하나의 persistence file이 만들어진다.

2. 하나의 persistence file을 여러 broker가 동시에 참조하는 것은 불가능하다.
C언어 베이스로 짜여진 mosquitto broker의 특성상 double free가 발생해서 broker 중 하나가 비정상 종료되는 것을 확인하였다.

3. persistence file에 저장되어 있는 미전송 메시지는 sub_client가 연결되어 메시지가 전송되면 자동으로 persistence file에서 지워진다.

## persistence file에 미전송 파일이 저장되는 조건과 원리

### 원리
mosquitto broker는 QoS 1, 2의 PUBLISH 메시지를 받으면 그 메시지의 topic과 같은 topic을 구독하고 있으며 broker에 저장되어 있는 subscriber 세션에 PUBLISH 메시지를 전송한다.

그리고 해당 subscriber로부터 PUBACK 메시지를 받을 때까지 broker 내부에 해당 메시지를 저장하고 있는다.

만약 broker가 PUBACK을 받기 전에 autosave_interval 시간이 된다면 persistence file에 미전송 메시지 정보를 저장한다.

만약 broker가 PUBACK을 받으면 autosave_interval 시간에 persistence file에서 해당 메시지를 지운다.

### 조건
1. subscriber의 clean_session = False여서 subscriber의 세션 정보가 broekr 안에 남아있고
가
2. 그 다음, Publisher가 전송한 메시지의 QoS가 1, 2일 때

## persistence 활용 방안

앞서 설명한 mosquitto persistence의 원리를 이용하면 "persistence file을 공유하지 않아도" 세션과 미전송 메시지가 자연스럽게 mesh를 구성하는 broker 사이에 공유가 될 것 같다.

만약 mesh를 구성하는 broker가 모든 메시지를 공유한다면 모든 broker는 같은 subscriber session 정보, 같은 미전송 메시지를 가지고 있을 것이다. 즉, 미전송 메시지에 대해 동기화가 되어있을 것이다.

따라서 어떤 broker가 종료되어도 연결이 끊긴 subscriber client가 wifi roaming을 통해 새로운 broker에 붙으면 그 새로운 broker의 persistence file에는 이미 subscriber client가 받아야 할 미전송 메시지를 가지고 있을 것이다.

따라서 별도의 persistence file 공유를 수행하지 않아도 subscriber client는 성공적으로 미전송 메시지를 수신할 수 있을 것이다.

# 메쉬 네트워크 서버: MQTT 브리지, cJSON, HAProxy, Redis

이 가이드는 메쉬 네트워크 서버 구성을 위해 필요한 주요 컴포넌트들의 설치 및 설정 방법을 안내합니다.
- 필수 개발 도구
- cJSON 라이브러리
- Eclipse Mosquitto MQTT 브로커 및 메쉬 브리징 설정
- HAProxy 로드 밸런서
- Redis 인-메모리 데이터베이스

---

## 목차

1. [필수 패키지 설치](#1-필수-패키지-설치)
2. [cJSON 빌드 및 설치](#2-cjson-빌드-및-설치)
3. [Mosquitto MQTT 브로커 빌드 및 설정](#3-mosquitto-mqtt-브로커-빌드-및-설정)
4. [HAProxy 설치 및 활성화](#4-haproxy-설치-및-활성화)
5. [Redis 서버 설치 및 활성화](#5-redis-서버-설치-및-활성화)
6. [참고사항](#6-참고사항)

---

## 1. 필수 패키지 설치

개발에 필요한 필수 라이브러리와 도구를 설치합니다.

```bash
sudo apt update
sudo apt install git libssl-dev xsltproc
```

---

## 2. cJSON 빌드 및 설치

[cJSON](https://github.com/DaveGamble/cJSON) 라이브러리를 클론 및 설치합니다.

```bash
git clone https://github.com/DaveGamble/cJSON.git
cd cJSON
sudo make
sudo make install
cd ..
```

---

## 3. Mosquitto MQTT 브로커 빌드 및 설정

[Eclipse Mosquitto](https://github.com/eclipse/mosquitto) 저장소를 클론합니다.

```bash
git clone https://github.com/eclipse/mosquitto
```

> **참고**  
> Mosquitto를 소스에서 빌드해야 할 수도 있습니다. 자세한 빌드 방법은 [Mosquitto 공식 문서](https://mosquitto.org/download/)를 참고하세요.

### Mosquitto 설정

Mosquitto 설정 파일을 수정합니다.

```bash
sudo vi /etc/mosquitto/mosquitto.conf
```

아래와 같이 설정을 추가 및 수정하여 리스너 활성화, 메쉬 브리지 연결을 구성합니다.

```ini
# 익명 접속 허용
allow_anonymous true
listener 1883 0.0.0.0

# RPi2로 브리지
connection MeshBroker2
address 192.168.100.2:1883
clientid 1_to_2bridge
topic sensor1/# out 0
topic sensor2/# in 0
cleansession false
restart_timeout 5
bridge_attempt_unsubscribe false
try_private true

# RPi3로 브리지
connection MeshBroker3
address 192.168.100.3:1883
clientid 1_to_3bridge
topic sensor1/# out 0
topic sensor3/# in 0
cleansession false
restart_timeout 5
bridge_attempt_unsubscribe false
try_private true

# 세션 및 메시지 영속성 설정
persistence true
```

### Mosquitto 재시작 및 활성화

```bash
sudo systemctl restart mosquitto
sudo systemctl enable mosquitto
```

---

## 4. HAProxy 설치 및 활성화

다음 명령어로 HAProxy를 설치합니다.

```bash
sudo apt install haproxy -y
```

---

## 5. Redis 서버 설치 및 활성화

인-메모리 데이터 저장소용 Redis를 설치합니다.

```bash
sudo apt install redis-server -y
```

---

## 6. 참고사항

- Mosquitto 설정에서는 이 노드를 두 개의 다른 노드(192.168.100.2, 192.168.100.3)와 브리지로 연결합니다. 네트워크 구조에 맞게 IP 및 토픽 설정을 필요에 따라 수정하세요.
- 사용 중인 포트가 방화벽에 의해 차단되지 않았는지 확인하세요.
- 프로덕션 환경에서는 익명 접속을 비활성화하고 인증 및 암호화(SSL/TLS)를 적용하는 것이 좋습니다.
- 자세한 내용은 각 공식 문서를 참고하세요:
  - [cJSON](https://github.com/DaveGamble/cJSON)
  - [Mosquitto](https://github.com/eclipse/mosquitto)
  - [HAProxy](https://haproxy.org/)
  - [Redis](https://redis.io/)
# MeshBroker Network

이 시스템은 Redis Cluster와 Eclipse Mosquitto를 활용하여 distributed Mesh Network로 MQTT broker 시스템을 구축하는 README입니다.

## 시스템 아키텍처

이 시스템은 분산형 아키텍처를 채택하여 각 노드가 독립적으로 작동하면서도 상호 연결된 구조를 형성합니다. 시스템의 핵심 구성요소는 다음과 같습니다:

각 노드는 Mosquitto MQTT 브로커와 Redis 노드를 포함하며, 이들이 Redis Cluster를 통해 연결되어 데이터 동기화와 메시지 라우팅을 담당합니다. 이러한 구조는 단일 장애점(Single Point of Failure)을 제거하고 시스템의 복원력을 향상시킵니다. 노드 간의 통신은 브리지 연결을 통해 이루어지며, 각 노드는 필요에 따라 Wi-Fi 액세스 포인트 역할도 수행할 수 있습니다.

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   Node A    │    │   Node B    │    │   Node C    │
│ ┌─────────┐ │    │ ┌─────────┐ │    │ ┌─────────┐ │
│ │Mosquitto│ │    │ │Mosquitto│ │    │ │Mosquitto│ │
│ │ Broker  │ │    │ │ Broker  │ │    │ │ Broker  │ │
│ └─────────┘ │    │ └─────────┘ │    │ └─────────┘ │
│      │      │    │      │      │    │      │      │
│ ┌─────────┐ │    │ ┌─────────┐ │    │ ┌─────────┐ │
│ │ Redis   │ │    │ │ Redis   │ │    │ │ Redis   │ │
│ │ Node    │ │◄───┤ │ Node    │ │◄───┤ │ Node    │ │
│ └─────────┘ │    │ └─────────┘ │    │ └─────────┘ │
└─────────────┘    └─────────────┘    └─────────────┘
       │                   │                   │
       └───────────────────┼───────────────────┘
                           │
                     Redis Cluster
```

## Wi-Fi 액세스 포인트 설정

### 액세스 포인트 구성 개요

메쉬 브로커 시스템에서 각 노드는 Wi-Fi 액세스 포인트로 동작하여 클라이언트 기기들이 네트워크에 접속할 수 있는 진입점을 제공합니다. 이 설정은 hostapd와 dnsmasq를 활용하여 구현되며, 동시에 다른 메쉬 브로커 노드들과의 연결도 관리합니다.

### 필수 요구사항 및 의존성 설치

시스템 구성을 위해서는 무선 인터페이스가 최소 3개(`wlan0`, `wlan1`, `wlan2`) 필요하며, 각각 액세스 포인트 제공, 메쉬 노드 간 연결, 그리고 추가 브로커 연결 용도로 사용됩니다. 인터넷 접속이 가능한 환경에서 다음 패키지들을 설치해야 합니다:

```bash
sudo apt update
sudo apt install hostapd dnsmasq
```

### hostapd 설정

hostapd는 Linux에서 소프트웨어 액세스 포인트를 생성하는 데 사용되는 핵심 데몬입니다. 설정 파일 `/etc/hostapd/hostapd.conf`를 생성하여 액세스 포인트의 동작 방식을 정의합니다:

```ini
interface=wlan0
ssid=MeshBroker1
wpa_passphrase=12345678
hw_mode=g
channel=9
wmm_enabled=1
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_key_mgmt=WPA-PSK
rsn_pairwise=CCMP
```

이 설정에서 `hw_mode=g`는 2.4GHz 대역을 사용함을 의미하며, `wmm_enabled=1`은 QoS(Quality of Service) 기능을 활성화하여 네트워크 성능을 최적화합니다. `/etc/default/hostapd` 파일에서 설정 파일 경로를 지정해야 합니다:

```bash
DAEMON_CONF="/etc/hostapd/hostapd.conf"
```

### DHCP 서버 설정

dnsmasq는 경량화된 DHCP 및 DNS 서버로, 액세스 포인트에 연결되는 클라이언트들에게 IP 주소를 자동으로 할당합니다. `/etc/dnsmasq.conf` 파일을 편집하여 DHCP 서비스를 구성합니다:

```ini
interface=wlan0
dhcp-range=192.168.101.2,192.168.101.254,255.255.255.0,24h
dhcp-option=3,192.168.101.1
server=8.8.8.8
```

이 설정에서 `dhcp-range`는 클라이언트에게 할당될 IP 주소 범위를 정의하며, `dhcp-option=3`은 기본 게이트웨이를 지정합니다. `server=8.8.8.8`은 DNS 서버로 Google의 공개 DNS를 사용하도록 설정합니다.

### 네트워크 인터페이스 설정 및 서비스 시작

액세스 포인트용 인터페이스에 고정 IP 주소를 할당하고 관련 서비스들을 시작합니다:

```bash
sudo ip addr add 192.168.101.1/24 broadcast 192.168.101.255 dev wlan0
```

```bash
sudo systemctl unmask hostapd
sudo systemctl enable --now hostapd dnsmasq
```

`systemctl unmask` 명령은 이전에 비활성화되었던 hostapd 서비스를 다시 활성화 가능한 상태로 만들며, `enable --now` 옵션은 서비스를 즉시 시작하고 부팅 시 자동 시작되도록 설정합니다.

## IP 라우팅 및 NAT 설정

### IP 포워딩 활성화

메쉬 네트워크에서 노드 간 트래픽 라우팅을 위해 IP 포워딩 기능을 활성화해야 합니다. 이는 하나의 네트워크 인터페이스로 수신된 패킷을 다른 인터페이스로 전달할 수 있게 해주는 중요한 설정입니다. `/etc/sysctl.conf` 파일에서 다음 라인의 주석을 제거합니다:

```ini
net.ipv4.ip_forward=1
```

설정을 즉시 적용하기 위해 다음 명령을 실행합니다:

```bash
sudo sysctl -p
```

### iptables 설치 및 NAT 규칙 설정

네트워크 주소 변환(NAT) 기능을 구현하기 위해 iptables를 설치하고 규칙을 설정합니다:

```bash
sudo apt install iptables

sudo iptables -t nat -A POSTROUTING -o wlan1 -j MASQUERADE
```

이는 후술할 IBSS모드의 다른 메쉬 브로커 노드들과의 연결을 라우팅할 때 사용됩니다.

## IBSS Ad-hoc 메쉬 네트워크 설정

### IBSS 네트워크 개요

IBSS(Independent Basic Service Set)는 중앙 집중식 액세스 포인트 없이 무선 기기들이 직접 연결되는 애드혹 네트워크 모드입니다. 메쉬 브로커 시스템에서는 이 기술을 활용하여 브로커 간 직접 통신 채널을 구성합니다.

### IBSS 시작 스크립트 생성

IBSS 네트워크 설정을 자동화하기 위한 스크립트를 생성합니다. 실제 환경에서는 `wlan1`을 사용 중인 무선 인터페이스 이름으로 교체해야 합니다:

```bash
sudo vi /usr/local/bin/start_ibss_server.sh
```

스크립트 내용:

```bash
#!/bin/bash

# Wireless interface name
WLAN_IFACE="wlan1"
# IBSS network name
SSID="MeshBroker"
# Channel frequency (MHz)
CHANNEL_MHZ="2437" # Channel 6 (2.4GHz band)

# Bring wlan1 down
ip link set $WLAN_IFACE down

# Switch wlan1 to IBSS (ad-hoc) mode
iw dev $WLAN_IFACE set type ibss

# Bring wlan1 up
ip link set $WLAN_IFACE up

# Join the IBSS network at the specified frequency
iw dev $WLAN_IFACE ibss join $SSID $CHANNEL_MHZ fixed-freq

# Assign a static IP address to the server node
ip addr add 192.168.100.1/24 dev $WLAN_IFACE
```

이 스크립트는 무선 인터페이스를 IBSS 모드로 전환하고 지정된 주파수(2437MHz, 채널 6)에서 네트워크에 참여하며, 고정 IP 주소를 할당합니다.

### 스크립트 실행 권한 설정 및 systemd 서비스 생성

스크립트를 실행 가능하게 만들고 시스템 부팅 시 자동으로 실행되도록 설정합니다:

```bash
sudo chmod +x /usr/local/bin/start_ibss_server.sh
```

systemd 서비스 파일을 생성하여 IBSS 네트워크를 자동으로 시작하도록 구성합니다:

```bash
sudo vi /etc/systemd/system/ibss-server.service
```

서비스 파일 내용:

```ini
[Unit]
Description=Start IBSS Ad-hoc Server
After=network.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/start_ibss_server.sh
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
```

`Type=oneshot`은 스크립트가 한 번 실행되고 종료되는 서비스임을 의미하며, `RemainAfterExit=yes`는 프로세스 종료 후에도 서비스가 활성 상태로 유지되도록 합니다.

서비스를 시작합니다:

```bash
sudo systemctl start ibss-server.service
```

## MQTT 브리지 및 개발 환경 설정

### 핵심 개발 도구 설치

메쉬 네트워크 서버 구축을 위한 필수 개발 라이브러리와 도구들을 설치합니다. 이러한 도구들은 Eclipse Mosquitto MQTT 브로커와 cJSON 라이브러리 컴파일에 필요합니다:

```bash
sudo apt update
sudo apt install git libssl-dev xsltproc docbook-xml docbook-xsl
```

`libssl-dev`는 SSL/TLS 암호화 지원을 위해 필요하며, `xsltproc`와 docbook 패키지들은 문서 생성에 사용됩니다.

### cJSON 라이브러리 설치

cJSON은 C 언어로 작성된 경량화된 JSON 파서 라이브러리로, MQTT 라이브러리 설치와 Redis 데이터 저장 시 JSON 형태의 데이터를 다루기 위해 필요합니다:

```bash
git clone https://github.com/DaveGamble/cJSON.git
cd cJSON
sudo make
sudo make install
cd ..
```

### Eclipse Mosquitto MQTT 브로커 구성

Mosquitto는 Eclipse Foundation에서 개발한 오픈소스 MQTT 브로커로, 경량화되어 있으면서도 강력한 기능을 제공합니다:

```bash
git clone https://github.com/eclipse/mosquitto
cd mosquitto
sudo make
```

### Mosquitto 설정 파일 구성

메쉬 네트워킹을 위한 MQTT 브리지를 설정합니다. 이 설정을 통해 여러 브로커 간의 메시지 동기화와 토픽 라우팅이 가능해집니다:

```bash
sudo vi /etc/mosquitto/mosquitto.conf
```

설정 파일 내용:

```ini
# Listener configuration
allow_anonymous true
listener 1883 0.0.0.0

# Bridge to RPi2
connection MeshBroker1_2
address 192.168.100.2:1883
clientid 1_to_2bridge
topic sensor1/# out 0
topic sensor2/# in 0
cleansession false
restart_timeout 5
bridge_attempt_unsubscribe false
try_private true

# Bridge to RPi3
connection MeshBroker1_3
address 192.168.100.3:1883
clientid 1_to_3bridge
topic sensor1/# out 0
topic sensor3/# in 0
cleansession false
restart_timeout 5
bridge_attempt_unsubscribe false
try_private true

# For redis cluster plugin
plugin /usr/lib/mosquitto/redis_cluster_plugin.so
```

이 설정에서 브리지 연결은 각 노드 간의 메시지 전달을 담당하며, `topic sensor1/# out 0`은 해당 토픽의 메시지를 다른 브로커로 전송함을 의미하고, `topic sensor2/# in 0`은 다른 브로커로부터 메시지를 수신함을 의미합니다.

## Redis Cluster 기반 분산 시스템 구축

### 시스템 의존성 및 라이브러리 설치

Redis Cluster를 활용한 분산 MQTT 브로커 시스템을 구축하기 위해 필요한 라이브러리들을 설치합니다. 이러한 라이브러리들은 Redis와의 통신, JSON 데이터 처리, 그리고 Mosquitto 플러그인 개발에 필수적입니다:

```bash
sudo apt update
sudo apt install -y libmosquitto-dev gcc make cmake redis-server

# Redis 데이터 저장을 위해 cJson 설치
git clone https://github.com/DaveGamble/cJSON.git
cd cJSON
make && sudo make install

# Redis 및 Redis_ssl 설치
git clone https://github.com/redis/hiredis.git
cd hiredis
sudo make install USE_SSL=1

# Redis Cluster 라이브러리 설치
git clone https://github.com/Nordix/hiredis-cluster.git
cd hiredis-cluster
make && sudo make install
```

hiredis는 Redis와 C 프로그램 간의 통신을 위한 클라이언트 라이브러리이며, hiredis-cluster는 Redis Cluster 환경에서의 확장된 기능을 제공합니다.

### Redis Cluster 설정 및 구성

Redis Cluster는 데이터를 여러 노드에 분산 저장하여 고가용성과 확장성을 제공하는 Redis의 분산 모드입니다. 각 노드별로 고유한 포트와 설정을 사용하여 클러스터를 구성합니다.

Redis 설정 파일 `/etc/redis/redis.conf` 구성:

```
port 7001
cluster-enabled yes
cluster-config-file nodes-7001.conf
cluster-node-timeout 5000
appendonly yes
appendfilename "appendonly-7001.aof"
dbfilename dump-7001.rdb
logfile /var/log/redis/redis-7001.log
pidfile /var/run/redis/redis-7001.pid
bind 0.0.0.0
protected-mode no
```

이 설정에서 `cluster-enabled yes`는 클러스터 모드를 활성화하며, `appendonly yes`는 데이터 지속성을 위한 AOF(Append Only File) 로깅을 활성화합니다. `bind 0.0.0.0`과 `protected-mode no`는 외부 연결을 허용합니다.

Redis 서버 시작 및 클러스터 생성:

```bash
sudo redis-server /etc/redis/redis.conf

sudo redis-cli --cluster create \
192.168.100.1:7001 \
192.168.100.2:7002 \
192.168.100.3:7003 \
--cluster-replicas 0
```

`--cluster-replicas 0` 옵션은 복제본 없이 마스터 노드로만 클러스터를 구성함을 의미합니다.

### Mosquitto Redis Cluster 플러그인 구현

이 플러그인은 Mosquitto MQTT 브로커와 Redis Cluster 간의 통합을 제공하여 메시지 저장, 세션 관리, 그리고 분산 환경에서의 메시지 라우팅을 담당합니다.

플러그인은 C 언어로 구현되며, `redis_cluster_plugin.c` 파일의 내용은 다음과 같습니다:
```c
#include <mosquitto.h>
#include <mosquitto_broker.h>
#include <mosquitto_plugin.h>
#include <hiredis/hiredis.h>
#include <hiredis_cluster/hircluster.h>
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>

static redisClusterContext *cluster_ctx = NULL;
static char redis_nodes[1024] = "192.168.100.1:7001,192.168.100.2:7002,192.168.100.3:7003";
static mosquitto_plugin_id_t *plugin_id = NULL;
static pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
static long long global_counter = 0;

int mosquitto_plugin_version(int supported_version_count, const int *supported_versions) {
    return 5;
}

/* Redis Cluster 초기화 */
static int init_redis_cluster(void) {
    if(cluster_ctx) redisClusterFree(cluster_ctx);
    cluster_ctx = redisClusterContextInit();
    if(!cluster_ctx) return MOSQ_ERR_UNKNOWN;
    if(redisClusterSetOptionAddNodes(cluster_ctx, redis_nodes) != REDIS_OK) {
        redisClusterFree(cluster_ctx);
        return MOSQ_ERR_UNKNOWN;
    }
    if(redisClusterConnect2(cluster_ctx) != REDIS_OK) {
        redisClusterFree(cluster_ctx);
        return MOSQ_ERR_UNKNOWN;
    }
    mosquitto_log_printf(MOSQ_LOG_INFO, "Connected to Redis Cluster"); 
    return MOSQ_ERR_SUCCESS;
}

/* 키 기반 안전 명령 실행 */
static redisReply* safe_command(const char *fmt, ...) {
    va_list ap;
    redisReply *reply;
    va_start(ap, fmt);
    reply = redisClustervCommand(cluster_ctx, fmt, ap);
    va_end(ap);
    return reply;
}

/* 키 없는 명령어를 첫 번째 마스터 노드에 직접 전송 */
static redisReply* execute_keyless(const char *cmd) {
    redisClusterNodeIterator iter;
    redisClusterNode *node;
    redisReply *reply = NULL;
    redisClusterInitNodeIterator(&iter, cluster_ctx);
    if((node = redisClusterNodeNext(&iter))) {
        reply = redisClusterCommandToNode(cluster_ctx, node, cmd);
    }
    return reply;
}

/* 글로벌 메시지 로그 저장 */
static long long store_global_log(const char *client_id, const char *topic,
                                  const void *payload, int len, int qos, int retain) {
    pthread_mutex_lock(&counter_mutex);
    long long id = ++global_counter;
    pthread_mutex_unlock(&counter_mutex);

    long long ts = time(NULL)*1000;
    safe_command("ZADD global_msgs %lld msg:%lld", ts, id);
    safe_command("HMSET msg:%lld client %s topic %s payload %b qos %d retain %d ts %lld",
                 id, client_id, topic, payload, len, qos, retain, ts);
    safe_command("EXPIRE msg:%lld 86400", id);
    return id;
}

/* 세션 저장 (Lock 없이) */
static int store_session(const char *client_id, const char *state) {
    char key[256];
    snprintf(key, sizeof(key), "session:%s", client_id);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "state", state);
    cJSON_AddNumberToObject(root, "ts", (double)time(NULL));
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    safe_command("SET %s %s", key, json);
    free(json);
    return MOSQ_ERR_SUCCESS;
}

/* 구독 처리: 토픽별 구독자 관리 */
static int on_acl_check(int event, void *edata, void *ud) {
    struct mosquitto_evt_acl_check *acl = edata;
    if(acl->access == MOSQ_ACL_SUBSCRIBE) {
        const char *cid = mosquitto_client_id(acl->client);
        safe_command("SADD topics:%s %s", acl->topic, cid);
    }
    return MOSQ_ERR_SUCCESS;
}

/* 인증 이벤트: Redis 초기화 및 세션 저장 */
static int on_basic_auth(int event, void *edata, void *ud) {
    struct mosquitto_evt_basic_auth *auth = edata;
    const char *cid = mosquitto_client_id(auth->client);
    if(init_redis_cluster()!=MOSQ_ERR_SUCCESS) return MOSQ_ERR_AUTH; 
    store_session(cid, "connected");
    return MOSQ_ERR_SUCCESS;
}

/* 메시지 수신 이벤트: Redis 저장 및 Mosquitto publish */
static int on_message(int event, void *edata, void *ud) {
    struct mosquitto_evt_message *msg = edata;
    const char *cid = mosquitto_client_id(msg->client);
    /* 글로벌 로그 */
    store_global_log(cid, msg->topic, msg->payload, msg->payloadlen, msg->qos, msg->retain);
    /* 세션 메시지 큐 */
    char key[256];
    snprintf(key, sizeof(key), "session_msgs:%s", cid);
    cJSON *o=cJSON_CreateObject();
    cJSON_AddStringToObject(o,"topic",msg->topic);
    cJSON_AddStringToObject(o,"payload",msg->payload?msg->payload:"");
    cJSON_AddNumberToObject(o,"qos",msg->qos);
    cJSON_AddNumberToObject(o,"retain",msg->retain);
    char *j=cJSON_PrintUnformatted(o);
    cJSON_Delete(o);
    safe_command("LPUSH %s %s",key,j);
    free(j);
    /* 실제 publish */
    redisReply *r = safe_command("SMEMBERS topics:%s", msg->topic);
    if(r && r->type==REDIS_REPLY_ARRAY) {
        for(int i=0;i<r->elements;i++){
            mosquitto_publish(msg->client, NULL, msg->topic,
                              msg->payloadlen, msg->payload, msg->qos, msg->retain);
        }
    }
    freeReplyObject(r);
    return MOSQ_ERR_SUCCESS;
}

/* 플러그인 초기화 및 콜백 등록 */
int mosquitto_plugin_init(mosquitto_plugin_id_t *id, void **ud, 
                          struct mosquitto_opt *opts, int count) {
    plugin_id = id;
    mosquitto_callback_register(id, MOSQ_EVT_BASIC_AUTH, on_basic_auth, NULL, NULL);
    mosquitto_callback_register(id, MOSQ_EVT_ACL_CHECK, on_acl_check, NULL, NULL);
    mosquitto_callback_register(id, MOSQ_EVT_MESSAGE, on_message, NULL, NULL);
    return MOSQ_ERR_SUCCESS;
}

/* 플러그인 종료 */
int mosquitto_plugin_cleanup(void *ud, struct mosquitto_opt *opts, int count) {
    if(cluster_ctx) redisClusterFree(cluster_ctx);
    return MOSQ_ERR_SUCCESS;
}
```

### 플러그인 빌드 및 설치

플러그인 컴파일과 설치를 자동화하는 빌드 스크립트를 제공합니다. 이 스크립트는 모든 필요한 라이브러리를 링크하여 공유 라이브러리를 생성합니다.

`build_install.sh` 스크립트:

```bash
#!/bin/bash

echo "Building and installing Redis Cluster Mosquitto Plugin..."

# 플러그인 컴파일
gcc -fPIC -shared -o redis_cluster_plugin.so redis_cluster_plugin.c \
    -L/usr/local/lib -lhiredis -lhiredis_cluster -lpthread -lmosquitto -lcjson

# 플러그인 설치
sudo cp -p redis_cluster_plugin.so /usr/lib/mosquitto/

echo "Installation completed!"
```

컴파일 옵션 설명:
- `-fPIC`: Position Independent Code, 공유 라이브러리 생성을 위해 필요
- `-shared`: 공유 라이브러리 생성
- `-L/usr/local/lib`: 라이브러리 검색 경로 지정
- 각종 라이브러리 링크: hiredis, hiredis_cluster, pthread, mosquitto, cjson

위에서 만든 쉘 스크립트를 다음 명령어를 통해 실행합니다:

```bash
sudo bash build_install.sh
```

라이브러리 경로 오류가 발생할 경우 다음 명령으로 해결할 수 있습니다:

```
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
echo "/usr/local/lib" | sudo tee /etc/ld.so.conf.d/local.conf
sudo ldconfig
```

## 브로커 시작 및 운영

모든 구성 요소가 설치되고 설정된 후, Mosquitto 브로커를 시작하여 시스템이 정상적으로 동작하는지 확인합니다:

```
sudo mosquitto -v -c mosquitto_broker.conf
```

`-v` 옵션은 상세한 디버그 정보를 출력하여 시스템 동작 상태를 모니터링할 수 있게 해줍니다. 이를 통해 Redis Cluster 연결 상태, 브리지 연결 성공 여부, 그리고 플러그인 로딩 상태를 실시간으로 확인할 수 있습니다.

## 센서 및 조명 연결

조도 센서의 경우 다음 코드를 통해 연결합니다.

```c
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Wire.h>
#include <BH1750.h>

#include <ArduinoMqttClient.h>

BH1750 lightMeter;
WiFiClient espClient;
MqttClient mqttClient(espClient);

// WiFi AP 리스트
const char* ssidList[] = {
  "MeshBroker3",
  "MeshBroker1",
  "MeshBroker2"
};
const char* passwordList[] = {
  "12345678",
  "12345678",
  "12345678",
};
const int apCount = sizeof(ssidList)/sizeof(ssidList[0]);

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define OLED_ADDR       0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

volatile bool wifi_connected = false;
TaskHandle_t wifiTaskHandles[2];
SemaphoreHandle_t connectMutex;
TaskHandle_t rssiTaskHandle = NULL;

struct ApInfo {
  String ssid;
  String password;
  int rssi;
};
ApInfo apInfos[apCount]; 

//Display Message
void showMessage(const String& line1, const String& line2 = "") {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.println(line1);
  if (line2 != "") display.println(line2);
  display.display();
}
//내림차순
int cmp(const void* a, const void* b) {
    return ((ApInfo*)b)->rssi - ((ApInfo*)a)->rssi;
}
void scanAndSortApByRssi() {
  int found = WiFi.scanNetworks();//scan network
  Serial.println("Founding network...\n");
  while (found <= 0 ) {//연결 가능한 네트워크를 계속 측정
    delay(100);
    found = WiFi.scanNetworks(); // 네트워크 스캔
  } 
  Serial.println("Found network!\n");
  // AP 리스트 초기화 및 RSSI 측정
  for (int i = 0; i < apCount; i++) {
    apInfos[i].ssid = ssidList[i]; // ssid
    apInfos[i].password = passwordList[i]; // password
    apInfos[i].rssi = -999; // Not found 
    for (int j = 0; j < found; j++) { // RSSI 측정
      if (WiFi.SSID(j) == ssidList[i]) {
        apInfos[i].rssi = WiFi.RSSI(j);
        break;
      }
    }
  }
  // quic sort
  qsort(apInfos, apCount, sizeof(ApInfo),cmp);
  // Sorted network 출력
  for (int i = 0; i < apCount; i++) {
    Serial.printf("SSID: %s, PASSWORD: %s, RSSI: %d dBm\n",
    apInfos[i].ssid.c_str(), apInfos[i].password.c_str(), apInfos[i].rssi);
  }
}
// WiFi Connection 
void connectWiFi() {
  // 순서대로 AP 연결시도
  for (int i = 0; i < apCount; i++) {
    if (apInfos[i].rssi != -999 && apInfos[i].rssi > -80) { //신호크기 제한
      WiFi.begin(apInfos[i].ssid, apInfos[i].password);
      int retry = 0;
      while (WiFi.status() != WL_CONNECTED && retry < 50) {//5초 retry
        delay(100);
        retry++;
      }
      if (WiFi.status() == WL_CONNECTED) {//연결 성공
        wifi_connected = true;
        showMessage("WiFi connected!", WiFi.SSID() + " " + WiFi.localIP().toString());
        Serial.printf("Connected to %s! IP: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

        // MQTT 연결 부분
        IPAddress localIP = WiFi.localIP();
        IPAddress brokerIP(localIP[0], localIP[1], localIP[2], 1);  // 마지막 옥텟만 1로

        mqttClient.setId("LightSensor");
        mqttClient.connect(brokerIP, 1883);
        Serial.printf("setServer to %s\n", brokerIP.toString().c_str());

        break;
      }
    }
  }
  if (!wifi_connected) showMessage("WiFi ALL failed");//연결 실패 
  else startRssiTask();//연결 성공 RSSI 측정
}
//Show RSSI and disconnect WiFi
void rssiMonitorTask(void * parameter) {
  unsigned long lastPrint = 0;
  while (true) {//already wifi connected
    if (WiFi.status() == WL_CONNECTED) {//rssi 출력
      int rssi = WiFi.RSSI();
      if (millis() - lastPrint > 1000) {  // 1초간격 출력
        Serial.printf("[RSSI] %d dBm\n", rssi);
        lastPrint = millis();
      }
      if(rssi<-80){ //RSSI가 너무 낮으면 disconnect
        Serial.printf("[RSSI] Too low, disconnecting..\n", rssi);
        WiFi.disconnect();
        stopRssiTask();
      }
    }else {
      Serial.println("[RSSI] WiFi Not Connected");
      stopRssiTask();
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);  // 0.1초 간격
  }
}
//FreeRTOS RSSI 체크 Task
void startRssiTask() {
  if (rssiTaskHandle == NULL) {
    xTaskCreatePinnedToCore(
      rssiMonitorTask,
      "RSSIMonitor",
      2048,
      NULL,
      1,
      &rssiTaskHandle,
      1
    );
  }
}
// RSSI Task 종료
void stopRssiTask() {
  if (rssiTaskHandle != NULL) {
    Serial.println("[RSSI Task] Stopped");
    vTaskDelete(rssiTaskHandle);
  }
}

void setup() {
  Serial.begin(115200);

  // OLED Initialization
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 OLED init failed");
    while (true);
  }
  connectMutex = xSemaphoreCreateMutex();
  scanAndSortApByRssi(); // scan RSSI and sorting list,connect wifi
  showMessage("Connecting to WiFi");
  connectWiFi();//wifi connect

  // ESP32의 I2C 핀 할당 (SDA: 21, SCL: 22)
  Wire.begin(21, 22);
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
}

void loop() {
  static unsigned long wifiRetryTime = 0;
  static bool tryingReconnect = false;
  //재연결
  if (WiFi.status() != WL_CONNECTED) {
    if (!tryingReconnect && millis() - wifiRetryTime > 10000) {
      showMessage("WiFi reconnecting...");
      Serial.println("WiFi reconnecting...");
      tryingReconnect = true;
      wifi_connected = false;
      rssiTaskHandle = NULL;
      //wifi 재설정
      WiFi.disconnect(true, true); // 연결 종료
      WiFi.mode(WIFI_OFF);         // WiFi 전원 OFF
      delay(100);                  // Wait OFF
      WiFi.mode(WIFI_STA);         // WiFi ON
      delay(1000);                 // Wait ON

      scanAndSortApByRssi();
      showMessage("Connecting to WiFi");
      connectWiFi();//wifi connect
      
      wifiRetryTime = millis();
      tryingReconnect = false;
    }
  }

  float lux = lightMeter.readLightLevel();
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");

  // mqtt
  mqttClient.beginMessage("sensor3/light", false, 1);
  if(lux < 40)
    mqttClient.print("ON");
  else
    mqttClient.print("OFF");
  mqttClient.endMessage();

  mqttClient.poll();

  delay(1000);
}
```
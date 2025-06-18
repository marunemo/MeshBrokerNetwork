# 🏗️ 아키텍처

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

# MeshBroker Wi-Fi Access Point Setup

This guide sets up a Linux-based device (e.g., Raspberry Pi) as a Wi-Fi Access Point (AP) with IP routing and automatic connection to another mesh broker.

---

## 📡 1. AP Setting (hostapd + dnsmasq)

### 1.1 Requirements

- Internet access
- Linux system with wireless interfaces (`wlan0`, `wlan1`, `wlan2`)

### 1.2 Install Dependencies

```bash
sudo apt update
sudo apt install hostapd dnsmasq
```

### 1.3 Configure hostapd

Create `/etc/hostapd/hostapd.conf`:

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

Edit `/etc/default/hostapd`:

Uncomment and set the path:

```bash
DAEMON_CONF="/etc/hostapd/hostapd.conf"
```

### 1.4 Configure dnsmasq

Edit `/etc/dnsmasq.conf`:

```ini
interface=wlan0
dhcp-range=192.168.101.2,192.168.101.254,255.255.255.0,24h
dhcp-option=3,192.168.101.1
server=8.8.8.8
```

### 1.5 Assign Static IP

```bash
sudo ip addr add 192.168.101.1/24 broadcast 192.168.101.255 dev wlan0
```

### 1.6 Start Services

```bash
sudo systemctl unmask hostapd
sudo systemctl enable --now hostapd dnsmasq
```

---

## 🔀 2. IP Routing (NAT & Forwarding)

### 2.1 Enable IP Forwarding

Edit `/etc/sysctl.conf` and uncomment:

```ini
net.ipv4.ip_forward=1
```

Apply:

```bash
sudo sysctl -p
```

### 2.2 Install iptables

```bash
sudo apt install iptables
```

### 2.3 Connect to Broker2 (via wlan2)

```bash
sudo wpa_passphrase "MeshBroker2" "12345678" | sudo tee /etc/wpa_supplicant/wpa_supplicant_broker2.conf
sudo wpa_supplicant -B -c /etc/wpa_supplicant/wpa_supplicant_broker2.conf -i wlan2
sudo dhclient wlan1
sudo iptables -t nat -A POSTROUTING -o wlan1 -j MASQUERADE
```

### Connect to Broker3

```bash
sudo wpa_passphrase "MeshBroker3" "12345678" | sudo tee /etc/wpa_supplicant/wpa_supplicant_broker3.conf
sudo wpa_supplicant -B -c /etc/wpa_supplicant/wpa_supplicant_broker3.conf -i wlan2
sudo dhclient wlan2
sudo iptables -t nat -A POSTROUTING -o wlan2 -j MASQUERADE
```

> ✅ Replace `wlan1` or `wlan2` with your actual device name using `ip a`.


# IBSS (Ad-hoc Mesh) Network Server Setup

## 1. Create the IBSS Startup Script

Create the following script to configure your wireless interface (replace `wlan1` with your actual interface name):

```bash
sudo vi /usr/local/bin/start_ibss_server.sh
```

**Example contents:**
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

---

## 2. Make the Script Executable

```bash
sudo chmod +x /usr/local/bin/start_ibss_server.sh
```

---

## 3. Create a systemd Service

Set up a systemd service to run your script automatically at boot:

```bash
sudo vi /etc/systemd/system/ibss-server.service
```

**Example contents:**
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

---

## 4. Enable and Start the Service

Enable and start the IBSS server service:

```bash
sudo systemctl start ibss-server.service
```

---

# Mesh Network Server: MQTT Bridge

This guide walks you through setting up essential components for a mesh network server:
- Core development tools
- cJSON library
- Eclipse Mosquitto MQTT broker with mesh bridging

---

## 1. Install Required Packages

Install necessary development libraries and tools:

```bash
sudo apt update
sudo apt install git libssl-dev xsltproc docbook-xml docbook-xsl
```

---

## 2. Build and Install cJSON

Clone and install the [cJSON](https://github.com/DaveGamble/cJSON) library:

```bash
git clone https://github.com/DaveGamble/cJSON.git
cd cJSON
sudo make
sudo make install
cd ..
```

---

## 3. Build and Configure Mosquitto MQTT Broker

Clone the [Mosquitto](https://github.com/eclipse/mosquitto) repository:

```bash
git clone https://github.com/eclipse/mosquitto
cd mosquitto
sudo make
```

### Mosquitto Configuration

Edit the Mosquitto configuration file:

```bash
sudo vi /etc/mosquitto/mosquitto.conf
```

Add or update with the following configuration to enable listeners and set up MQTT bridges for mesh networking:

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
topic sensor/broker1/# out 0
topic sensor/broker3/# in 0
cleansession false
restart_timeout 5
bridge_attempt_unsubscribe false
try_private true

# presistent
persistence true
persistence_file /home/pi/mosquitto.db

# For redis cluster plugin
plugin /usr/lib/mosquitto/redis_cluster_plugin.so
```

# Redis Cluster 기반 분산 Mosquitto MQTT 브로커 시스템

## 🚀 설치 및 구성

### 1. 의존성 설치

```bash
sudo apt update
sudo apt install -y libmosquitto-dev gcc make cmake redis-server

git clone https://github.com/DaveGamble/cJSON.git
cd cJSON
make && sudo make install

git clone https://github.com/redis/hiredis.git
cd hiredis
sudo make install USE_SSL=1

# Redis Cluster 라이브러리 설치
git clone https://github.com/Nordix/hiredis-cluster.git
cd hiredis-cluster
make && sudo make install
```

### 2. Redis Cluster 설정

/etc/redis/redis.conf
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

```
sudo redis-server /etc/redis/redis.conf
```

```
sudo redis-cli --cluster create \
192.168.100.1:7001 \
192.168.100.2:7002 \
192.168.100.3:7003 \
--cluster-replicas 0
```

### 3. Mosquitto 플러그인 구현

redis_cluster_plugin.c
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

/* redis 명령 실행 */
static redisReply* safe_command(const char *fmt, ...) {
    va_list ap;
    redisReply *reply;
    va_start(ap, fmt);
    reply = redisClustervCommand(cluster_ctx, fmt, ap);
    va_end(ap);
    return reply;
}

/* 명령어를 첫 번째 마스터 노드에 직접 전송 */
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

/* 세션 저장 */
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

### 4. 빌드 및 설치

**`build_install.sh`**:

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

오류 발생 시
```
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
echo "/usr/local/lib" | sudo tee /etc/ld.so.conf.d/local.conf
sudo ldconfig
```

### 5. 배포 및 테스트
```
sudo mosquitto -v -c mosquitto_broker.conf
```
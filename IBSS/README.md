sudo apt install dnsmasq

sudo vi /etc/dnsmasq.conf

Edit `/etc/dnsmasq.conf`:

sudo vi /usr/local/bin/start_ibss_server.sh

```ini
#!/bin/bash

# 무선 인터페이스 이름
WLAN_IFACE="wlan1"
# IBSS 네트워크 이름
SSID="MeshBroker"
# 채널 (MHz)
CHANNEL_MHZ="2437" # 채널 6의 주파수 (2.4GHz 대역)

# wlan1 다운
ip link set $WLAN_IFACE down

# IBSS 모드로 전환
sudo iw dev wlan1 set type ibss

# wlan1 업
ip link set $WLAN_IFACE up

# IBSS 모드에서 join
# 주파수는 채널에 따라 달라집니다. (예: 채널 6은 2437MHz)
iw dev $WLAN_IFACE ibss join $SSID $CHANNEL_MHZ fixed-freq

# 서버 노드의 고정 IP 주소 설정
ip addr add 192.168.100.1/24 dev $WLAN_IFACE
```

sudo chmod +x /usr/local/bin/start_ibss_server.sh

sudo vi /etc/systemd/system/ibss-server.service

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

sudo systemctl enable ibss-server.service
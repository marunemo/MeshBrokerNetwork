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

> ✅ Replace `wlan1` or `wlan2` with your actual device name using `ip a`.

### (Optional) Connect to Broker3

```bash
sudo wpa_passphrase "MeshBroker3" "12345678" | sudo tee /etc/wpa_supplicant/wpa_supplicant_broker3.conf
sudo wpa_supplicant -B -c /etc/wpa_supplicant/wpa_supplicant_broker3.conf -i wlan2
sudo dhclient wlan2
sudo iptables -t nat -A POSTROUTING -o wlan2 -j MASQUERADE
```

---

## 🤖 3. Auto Connect to Broker2 & Broker3 on Boot

### 3.1 Create Connection Scripts

#### For Broker2

```bash
sudo vim /usr/local/bin/auto_connect_broker2.sh
sudo chmod +x /usr/local/bin/auto_connect_broker2.sh
```

Contents:

```bash
#!/bin/bash
sudo wpa_passphrase "MeshBroker2" "12345678" | sudo tee /etc/wpa_supplicant/wpa_supplicant_broker2.conf
sudo wpa_supplicant -B -c /etc/wpa_supplicant/wpa_supplicant_broker2.conf -i wlan2
sudo dhclient wlan1
sudo iptables -t nat -A POSTROUTING -o wlan1 -j MASQUERADE
```

#### For Broker3

```bash
sudo vim /usr/local/bin/auto_connect_broker3.sh
sudo chmod +x /usr/local/bin/auto_connect_broker3.sh
```

Contents:

```bash
#!/bin/bash
sudo wpa_passphrase "MeshBroker3" "12345678" | sudo tee /etc/wpa_supplicant/wpa_supplicant_broker3.conf
sudo wpa_supplicant -B -c /etc/wpa_supplicant/wpa_supplicant_broker3.conf -i wlan2
sudo dhclient wlan2
sudo iptables -t nat -A POSTROUTING -o wlan2 -j MASQUERADE
```

---

### 3.2 Create systemd Services

#### For Broker2

```bash
sudo vim /etc/systemd/system/auto_connect_broker2.service
```

```ini
[Unit]
Description=Connect broker2 when network is up
After=network-online.target
Wants=network-online.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/auto_connect_broker2.sh
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
```

#### For Broker3

```bash
sudo vim /etc/systemd/system/auto_connect_broker3.service
```

```ini
[Unit]
Description=Connect broker3 when network is up
After=network-online.target
Wants=network-online.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/auto_connect_broker3.sh
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
```

Enable both services:

```bash
sudo systemctl enable auto_connect_broker2.service
sudo systemctl enable auto_connect_broker3.service
```
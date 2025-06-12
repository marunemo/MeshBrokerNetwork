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

---

## 🤖 3. Dynamic Auto Connect to Mesh Brokers (Broker2 & Broker3)

To ensure stable reconnection, we run `wpa_supplicant` as a background service and trigger DHCP/NAT via a common **hook script** using `wpa_cli` when connection is established.

### 3.1 Prepare WPA Supplicant Config Files

Create `/etc/wpa_supplicant/wpa_supplicant_broker2.conf` (for Broker2):

```ini
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1

network={
    ssid="MeshBroker2"
    psk="12345678"
}
```

Create `/etc/wpa_supplicant/wpa_supplicant_broker3.conf` (for Broker3):

```ini
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1

network={
    ssid="MeshBroker3"
    psk="12345678"
}
```

---

### 3.2 Create Common Hook Script for Connection Events

This script will be called automatically by `wpa_cli` whenever the connection status changes on the interface.
When connected, it acquires an IP via `dhclient` and sets up NAT rules with `iptables`.

```bash
sudo mkdir -p /etc/wpa_supplicant/action
sudo vi /etc/wpa_supplicant/action/on_connect.sh
sudo chmod +x /etc/wpa_supplicant/action/on_connect.sh
```

Content of `/etc/wpa_supplicant/action/on_connect.sh`:

```bash
#!/bin/bash
# on_connect.sh - wpa_cli hook script for DHCP and NAT setup on Wi-Fi connection

IFACE=$1       # Network interface name (e.g., wlan2)
STATUS=$2      # Connection status string (e.g., CONNECTED, DISCONNECTED)

# Only run when connection status is CONNECTED
if [ "$STATUS" = "CONNECTED" ]; then
  # Log connection status to system logger
  logger "Connected on $IFACE: acquiring IP and setting NAT"

  # Request IP address using DHCP client
  dhclient $IFACE

  # Add masquerade rule in iptables NAT POSTROUTING chain if not already present
  iptables -t nat -C POSTROUTING -o $IFACE -j MASQUERADE 2>/dev/null || \
    iptables -t nat -A POSTROUTING -o $IFACE -j MASQUERADE
fi
```

---

### 3.3 Create systemd Services for Broker2 and Broker3

Each broker has its own systemd service to run `wpa_supplicant` in the background and call the hook script via `wpa_cli`.

#### Broker2 service (`/etc/systemd/system/broker2-connect.service`):

```ini
[Unit]
Description=Auto-connect to MeshBroker2
After=network.target    # Start after the network is ready

[Service]
# Run wpa_supplicant in background for wlan2 using Broker2 config
ExecStart=/sbin/wpa_supplicant -i wlan2 -c /etc/wpa_supplicant/wpa_supplicant_broker2.conf -D nl80211,wext

# Use wpa_cli to monitor wpa_supplicant events and call hook script on connection
ExecStartPost=/sbin/wpa_cli -i wlan2 -a /etc/wpa_supplicant/action/on_connect.sh

Restart=always          # Restart service automatically on failure

[Install]
WantedBy=multi-user.target  # Enable service to start at system boot
```

#### Broker3 service (`/etc/systemd/system/broker3-connect.service`):

```ini
[Unit]
Description=Auto-connect to MeshBroker3
After=network.target    # Start after the network is ready

[Service]
# Run wpa_supplicant in background for wlan2 using Broker3 config
ExecStart=/sbin/wpa_supplicant -i wlan2 -c /etc/wpa_supplicant/wpa_supplicant_broker3.conf -D nl80211,wext

# Use wpa_cli to monitor wpa_supplicant events and call hook script on connection
ExecStartPost=/sbin/wpa_cli -i wlan2 -a /etc/wpa_supplicant/action/on_connect.sh

Restart=always          # Restart service automatically on failure

[Install]
WantedBy=multi-user.target  # Enable service to start at system boot
```

---

### 3.4 Enable Desired Broker Auto Connect

Enable the appropriate broker service:

```bash
sudo systemctl enable broker2-connect.service
# or
sudo systemctl enable broker3-connect.service
```
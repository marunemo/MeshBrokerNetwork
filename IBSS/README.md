# IBSS (Ad-hoc Mesh) Network Server Setup

This guide describes how to configure a Linux machine as an IBSS (ad-hoc mesh) Wi-Fi server node using a custom shell script and a systemd service.  
**No DHCP or DNS server (like dnsmasq) is involved.**

---

## Table of Contents

1. [Create the IBSS Startup Script](#1-create-the-ibss-startup-script)
2. [Make the Script Executable](#2-make-the-script-executable)
3. [Create a systemd Service](#3-create-a-systemd-service)
4. [Enable and Start the Service](#4-enable-and-start-the-service)
5. [Notes](#5-notes)

---

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
sudo systemctl enable ibss-server.service
sudo systemctl start ibss-server.service
```

---

## 5. Notes

- Adjust `WLAN_IFACE`, `SSID`, `CHANNEL_MHZ`, and IP address as needed for your setup.
- Make sure your Wi-Fi hardware and drivers support IBSS (ad-hoc) mode.
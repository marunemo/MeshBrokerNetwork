# Mesh Network Server: MQTT Bridge

This guide walks you through setting up essential components for a mesh network server:
- Core development tools
- cJSON library
- Eclipse Mosquitto MQTT broker with mesh bridging

---

## Table of Contents

1. [Install Required Packages](#1-install-required-packages)
2. [Build and Install cJSON](#2-build-and-install-cjson)
3. [Build and Configure Mosquitto MQTT Broker](#3-build-and-configure-mosquitto-mqtt-broker)

---

## 1. Install Required Packages

Install necessary development libraries and tools:

```bash
sudo apt update
sudo apt install git libssl-dev xsltproc
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
# Allow anonymous connections
allow_anonymous true
listener 1883 0.0.0.0

# Bridge to RPi2
connection MeshBroker2
address 192.168.100.2:1883
clientid 1_to_2bridge
topic sensor1/# out 0
topic sensor2/# in 0
cleansession false
restart_timeout 5
bridge_attempt_unsubscribe false
try_private true

# Bridge to RPi3
connection MeshBroker3
address 192.168.100.3:1883
clientid 1_to_3bridge
topic sensor1/# out 0
topic sensor3/# in 0
cleansession false
restart_timeout 5
bridge_attempt_unsubscribe false
try_private true

# Persistence settings for sessions and messages
persistence true
```

### Restart and Enable Mosquitto

```bash
sudo systemctl restart mosquitto
sudo systemctl enable mosquitto
```

## Notes

- The Mosquitto configuration bridges this node to two others (`192.168.100.2` and `192.168.100.3`). Adjust addresses and topics as necessary for your mesh topology.
- Make sure ports are open and not blocked by firewalls.
- For production use, consider disabling anonymous MQTT access and setting up authentication and encryption.
- For more information, refer to official documentation:
  - [cJSON](https://github.com/DaveGamble/cJSON)
  - [Mosquitto](https://github.com/eclipse/mosquitto)
sudo apt install git libssl-dev xsltproc

git clone https://github.com/DaveGamble/cJSON.git

cd cJSON
sudo make
sudo make install

git clone https://github.com/eclipse/mosquitto

sudo vi /etc/mosquitto/mosquitto.conf

```
# Listener configuration
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

sudo systemctl restart mosquitto
sudo systemctl enable mosquitto


sudo apt install haproxy -y

sudo apt install redis-server -y

```
sudo apt install mosquitto mosquitto-clients -y
```

```
sudo vi /etc/mosquitto/mosquitto.conf
```
아래 입력
```
# Listener configuration
allow_anonymous true
listener 1883 0.0.0.0

# Bridge to RPi1
connection MeshBroker1
address 192.168.100.1:1883
clientid 3_to_1bridge
topic 3/# out 0
topic 1/# in 0
cleansession false
restart_timeout 5
bridge_attempt_unsubscribe false
try_private true

# Bridge to RPi2
connection MeshBroker2
address 192.168.100.2:1883
clientid 3_to_2bridge
topic 3/# out 0
topic 2/# in 0
cleansession false
restart_timeout 5
bridge_attempt_unsubscribe false
try_private true

# Persistence settings for sessions and messages
persistence true
persistence_file mosquitto.db
persistence_location /var/lib/mosquitto/
```


```
sudo systemctl restart mosquitto
sudo systemctl enable mosquitto
``` 
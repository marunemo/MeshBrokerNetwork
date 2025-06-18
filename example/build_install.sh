#!/bin/bash

echo "Building and installing Redis Cluster Mosquitto Plugin..."

# 플러그인 컴파일
gcc -fPIC -shared -o redis_cluster_plugin.so redis_cluster_plugin.c \
    -L/usr/local/lib -lhiredis -lhiredis_cluster -lpthread -lmosquitto -lcjson

# 플러그인 설치
sudo cp -p redis_cluster_plugin.so /usr/lib/mosquitto/

echo "Installation completed!"
echo "Run './setup_redis_cluster.sh' to setup Redis cluster"
echo "Then restart Mosquitto: sudo systemctl restart mosquitto"

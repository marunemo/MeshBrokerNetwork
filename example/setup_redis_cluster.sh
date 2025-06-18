#!/bin/bash
# setup_redis_cluster.sh

echo "Setting up Redis Cluster..."

# Redis 설정 디렉터리 생성
sudo mkdir -p /etc/redis/cluster
cd /etc/redis/cluster

# 각 노드별 설정 파일 생성
for port in 6379 6380 6381; do
    sudo tee redis-${port}.conf << EOF
port ${port}
cluster-enabled yes
cluster-config-file nodes-${port}.conf
cluster-node-timeout 5000
appendonly yes
appendfilename "appendonly-${port}.aof"
dbfilename dump-${port}.rdb
logfile /var/log/redis/redis-${port}.log
pidfile /var/run/redis/redis-${port}.pid
dir /var/lib/redis/${port}/
bind 0.0.0.0
protected-mode no
EOF

    # 데이터 디렉터리 생성
    sudo mkdir -p /var/lib/redis/${port}
    sudo chown redis:redis /var/lib/redis/${port}
done

# Redis 서버 시작
for port in 6379 6380 6381; do
    echo "Starting Redis server on port ${port}..."
    sudo redis-server /etc/redis/cluster/redis-${port}.conf --daemonize yes
done

# 클러스터 생성 대기
sleep 5

# 클러스터 초기화
echo "Creating Redis cluster..."
redis-cli --cluster create 127.0.0.1:6379 127.0.0.1:6380 127.0.0.1:6381 \
    --cluster-replicas 0 --cluster-yes

echo "Redis Cluster setup completed!"

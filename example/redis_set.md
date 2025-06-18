sudo apt update && sudo apt install -y redis-server

```
port 7001  # 각 노드별 고유 포트 지정
cluster-enabled yes
cluster-config-file nodes.conf
cluster-node-timeout 5000
appendonly yes
bind 0.0.0.0  # 외부 접속 허용
protected-mode no
```

redis-cli -h 192.168.100.3 -p 7001 cluster nodes
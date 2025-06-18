#include <mosquitto.h>
#include <mosquitto_broker.h>
#include <mosquitto_plugin.h>
#include <hiredis/hiredis.h>
#include <hiredis_cluster/hircluster.h>
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>

static redisClusterContext *cluster_ctx = NULL;
static char redis_nodes[1024] = "192.168.100.1:7001,192.168.100.2:7002,192.168.100.3:7003";
static mosquitto_plugin_id_t *plugin_id = NULL;
static pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
static long long global_counter = 0;

int mosquitto_plugin_version(int supported_version_count, const int *supported_versions) {
    return 5;
}

/* Redis Cluster 초기화 */
static int init_redis_cluster(void) {
    if(cluster_ctx) redisClusterFree(cluster_ctx);
    cluster_ctx = redisClusterContextInit();
    if(!cluster_ctx) return MOSQ_ERR_UNKNOWN;
    if(redisClusterSetOptionAddNodes(cluster_ctx, redis_nodes) != REDIS_OK) {
        redisClusterFree(cluster_ctx);
        return MOSQ_ERR_UNKNOWN;
    }
    if(redisClusterConnect2(cluster_ctx) != REDIS_OK) {
        redisClusterFree(cluster_ctx);
        return MOSQ_ERR_UNKNOWN;
    }
    mosquitto_log_printf(MOSQ_LOG_INFO, "Connected to Redis Cluster"); 
    return MOSQ_ERR_SUCCESS;
}

/* 키 기반 안전 명령 실행 */
static redisReply* safe_command(const char *fmt, ...) {
    va_list ap;
    redisReply *reply;
    va_start(ap, fmt);
    reply = redisClustervCommand(cluster_ctx, fmt, ap);
    va_end(ap);
    return reply;
}

/* 키 없는 명령어를 첫 번째 마스터 노드에 직접 전송 */
static redisReply* execute_keyless(const char *cmd) {
    redisClusterNodeIterator iter;
    redisClusterNode *node;
    redisReply *reply = NULL;
    redisClusterInitNodeIterator(&iter, cluster_ctx);
    if((node = redisClusterNodeNext(&iter))) {
        reply = redisClusterCommandToNode(cluster_ctx, node, cmd);
    }
    return reply;
}

/* 글로벌 메시지 로그 저장 */
static long long store_global_log(const char *client_id, const char *topic,
                                  const void *payload, int len, int qos, int retain) {
    pthread_mutex_lock(&counter_mutex);
    long long id = ++global_counter;
    pthread_mutex_unlock(&counter_mutex);

    long long ts = time(NULL)*1000;
    safe_command("ZADD global_msgs %lld msg:%lld", ts, id);
    safe_command("HMSET msg:%lld client %s topic %s payload %b qos %d retain %d ts %lld",
                 id, client_id, topic, payload, len, qos, retain, ts);
    safe_command("EXPIRE msg:%lld 86400", id);
    return id;
}

/* 세션 저장 (Lock 없이) */
static int store_session(const char *client_id, const char *state) {
    char key[256];
    snprintf(key, sizeof(key), "session:%s", client_id);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "state", state);
    cJSON_AddNumberToObject(root, "ts", (double)time(NULL));
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    safe_command("SET %s %s", key, json);
    free(json);
    return MOSQ_ERR_SUCCESS;
}

/* 구독 처리: 토픽별 구독자 관리 */
static int on_acl_check(int event, void *edata, void *ud) {
    struct mosquitto_evt_acl_check *acl = edata;
    if(acl->access == MOSQ_ACL_SUBSCRIBE) {
        const char *cid = mosquitto_client_id(acl->client);
        safe_command("SADD topics:%s %s", acl->topic, cid);
    }
    return MOSQ_ERR_SUCCESS;
}

/* 인증 이벤트: Redis 초기화 및 세션 저장 */
static int on_basic_auth(int event, void *edata, void *ud) {
    struct mosquitto_evt_basic_auth *auth = edata;
    const char *cid = mosquitto_client_id(auth->client);
    if(init_redis_cluster()!=MOSQ_ERR_SUCCESS) return MOSQ_ERR_AUTH; 
    store_session(cid, "connected");
    return MOSQ_ERR_SUCCESS;
}

/* 메시지 수신 이벤트: Redis 저장 및 Mosquitto publish */
static int on_message(int event, void *edata, void *ud) {
    struct mosquitto_evt_message *msg = edata;
    const char *cid = mosquitto_client_id(msg->client);
    /* 글로벌 로그 */
    store_global_log(cid, msg->topic, msg->payload, msg->payloadlen, msg->qos, msg->retain);
    /* 세션 메시지 큐 */
    char key[256];
    snprintf(key, sizeof(key), "session_msgs:%s", cid);
    cJSON *o=cJSON_CreateObject();
    cJSON_AddStringToObject(o,"topic",msg->topic);
    cJSON_AddStringToObject(o,"payload",msg->payload?msg->payload:"");
    cJSON_AddNumberToObject(o,"qos",msg->qos);
    cJSON_AddNumberToObject(o,"retain",msg->retain);
    char *j=cJSON_PrintUnformatted(o);
    cJSON_Delete(o);
    safe_command("LPUSH %s %s",key,j);
    free(j);
    /* 실제 publish */
    redisReply *r = safe_command("SMEMBERS topics:%s", msg->topic);
    if(r && r->type==REDIS_REPLY_ARRAY) {
        for(int i=0;i<r->elements;i++){
            mosquitto_publish(msg->client, NULL, msg->topic,
                              msg->payloadlen, msg->payload, msg->qos, msg->retain);
        }
    }
    freeReplyObject(r);
    return MOSQ_ERR_SUCCESS;
}

/* 플러그인 초기화 및 콜백 등록 */
int mosquitto_plugin_init(mosquitto_plugin_id_t *id, void **ud, 
                          struct mosquitto_opt *opts, int count) {
    plugin_id = id;
    mosquitto_callback_register(id, MOSQ_EVT_BASIC_AUTH, on_basic_auth, NULL, NULL);
    mosquitto_callback_register(id, MOSQ_EVT_ACL_CHECK, on_acl_check, NULL, NULL);
    mosquitto_callback_register(id, MOSQ_EVT_MESSAGE, on_message, NULL, NULL);
    return MOSQ_ERR_SUCCESS;
}

/* 플러그인 종료 */
int mosquitto_plugin_cleanup(void *ud, struct mosquitto_opt *opts, int count) {
    if(cluster_ctx) redisClusterFree(cluster_ctx);
    return MOSQ_ERR_SUCCESS;
}


/*
 * rmq_connection.c - RabbitMQ connection handling for rabbitmqbox
 *
 * Copyright (c) 2026 Vasil Jamalashvili <shapeless@pm.me>
 * MIT License
 */

#include <string.h>
#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>
#include <rabbitmq-c/framing.h>
#ifdef HAVE_RABBITMQ_SSL
#include <rabbitmq-c/ssl_socket.h>
#endif

#include "gwlib/gwlib.h"
#include "rmq_connection.h"

/* Default values */
#define DEFAULT_PORT 5672
#define DEFAULT_VHOST "/"
#define DEFAULT_HEARTBEAT 60
#define DEFAULT_PREFETCH 100

#define DEFAULT_QUEUE_SEND "sms.send"
#define DEFAULT_QUEUE_MO "sms.mo"
#define DEFAULT_QUEUE_DLR "sms.dlr"
#define DEFAULT_QUEUE_FAILED "sms.failed"

/* Check AMQP reply for errors */
static int check_amqp_reply(amqp_connection_state_t conn, const char *context)
{
    amqp_rpc_reply_t reply = amqp_get_rpc_reply(conn);

    switch (reply.reply_type) {
    case AMQP_RESPONSE_NORMAL:
        return 0;

    case AMQP_RESPONSE_NONE:
        error(0, "RabbitMQ %s: missing RPC reply type", context);
        return -1;

    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
        error(0, "RabbitMQ %s: %s", context, amqp_error_string2(reply.library_error));
        return -1;

    case AMQP_RESPONSE_SERVER_EXCEPTION:
        switch (reply.reply.id) {
        case AMQP_CONNECTION_CLOSE_METHOD: {
            amqp_connection_close_t *m = (amqp_connection_close_t *)reply.reply.decoded;
            error(0, "RabbitMQ %s: server connection error %d, message: %.*s",
                  context, m->reply_code,
                  (int)m->reply_text.len, (char *)m->reply_text.bytes);
            return -1;
        }
        case AMQP_CHANNEL_CLOSE_METHOD: {
            amqp_channel_close_t *m = (amqp_channel_close_t *)reply.reply.decoded;
            error(0, "RabbitMQ %s: server channel error %d, message: %.*s",
                  context, m->reply_code,
                  (int)m->reply_text.len, (char *)m->reply_text.bytes);
            return -1;
        }
        default:
            error(0, "RabbitMQ %s: unknown server error, method id 0x%08X",
                  context, reply.reply.id);
            return -1;
        }
    }

    return -1;
}

/* Declare a queue */
static int declare_queue(RMQConnection *rmq, Octstr *queue_name)
{
    amqp_queue_declare(
        rmq->conn,
        rmq->channel,
        amqp_cstring_bytes(octstr_get_cstr(queue_name)),
        0,      /* passive */
        1,      /* durable */
        0,      /* exclusive */
        0,      /* auto_delete */
        amqp_empty_table
    );

    if (check_amqp_reply(rmq->conn, "queue.declare") < 0) {
        return -1;
    }

    debug("rmq", 0, "Declared queue: %s", octstr_get_cstr(queue_name));
    return 0;
}

RMQConnection *rmq_connection_create(CfgGroup *grp)
{
    RMQConnection *rmq;
    Octstr *tmp;

    rmq = gw_malloc(sizeof(RMQConnection));
    memset(rmq, 0, sizeof(RMQConnection));

    rmq->lock = mutex_create();
    rmq->connected = 0;
    rmq->channel = 1;

    /* Connection parameters */
    rmq->host = cfg_get(grp, octstr_imm("rabbitmq-host"));
    if (rmq->host == NULL)
        rmq->host = octstr_create("localhost");

    if (cfg_get_integer(&rmq->port, grp, octstr_imm("rabbitmq-port")) == -1)
        rmq->port = DEFAULT_PORT;

    rmq->vhost = cfg_get(grp, octstr_imm("rabbitmq-vhost"));
    if (rmq->vhost == NULL)
        rmq->vhost = octstr_create(DEFAULT_VHOST);

    rmq->username = cfg_get(grp, octstr_imm("rabbitmq-user"));
    if (rmq->username == NULL)
        rmq->username = octstr_create("guest");

    rmq->password = cfg_get(grp, octstr_imm("rabbitmq-pass"));
    if (rmq->password == NULL)
        rmq->password = octstr_create("guest");

    if (cfg_get_integer(&rmq->heartbeat, grp, octstr_imm("rabbitmq-heartbeat")) == -1)
        rmq->heartbeat = DEFAULT_HEARTBEAT;

    if (cfg_get_integer(&rmq->prefetch, grp, octstr_imm("rabbitmq-prefetch")) == -1)
        rmq->prefetch = DEFAULT_PREFETCH;

    /* Queue names */
    rmq->queue_send = cfg_get(grp, octstr_imm("queue-send"));
    if (rmq->queue_send == NULL)
        rmq->queue_send = octstr_create(DEFAULT_QUEUE_SEND);

    rmq->queue_mo = cfg_get(grp, octstr_imm("queue-mo"));
    if (rmq->queue_mo == NULL)
        rmq->queue_mo = octstr_create(DEFAULT_QUEUE_MO);

    rmq->queue_dlr = cfg_get(grp, octstr_imm("queue-dlr"));
    if (rmq->queue_dlr == NULL)
        rmq->queue_dlr = octstr_create(DEFAULT_QUEUE_DLR);

    rmq->queue_failed = cfg_get(grp, octstr_imm("queue-failed"));
    if (rmq->queue_failed == NULL)
        rmq->queue_failed = octstr_create(DEFAULT_QUEUE_FAILED);

    /* Exchange (optional) */
    rmq->exchange = cfg_get(grp, octstr_imm("exchange"));
    rmq->exchange_type = cfg_get(grp, octstr_imm("exchange-type"));

    /* TLS/SSL options */
    cfg_get_bool(&rmq->use_ssl, grp, octstr_imm("rabbitmq-ssl"));
    rmq->ssl_cacert = cfg_get(grp, octstr_imm("rabbitmq-ssl-cacert"));
    rmq->ssl_cert = cfg_get(grp, octstr_imm("rabbitmq-ssl-cert"));
    rmq->ssl_key = cfg_get(grp, octstr_imm("rabbitmq-ssl-key"));
    cfg_get_bool(&rmq->ssl_verify, grp, octstr_imm("rabbitmq-ssl-verify"));

    /* Default SSL port if not specified */
    if (rmq->use_ssl && rmq->port == DEFAULT_PORT)
        rmq->port = 5671;

    info(0, "RabbitMQ config: host=%s port=%d vhost=%s user=%s ssl=%s",
         octstr_get_cstr(rmq->host), rmq->port,
         octstr_get_cstr(rmq->vhost), octstr_get_cstr(rmq->username),
         rmq->use_ssl ? "yes" : "no");

    return rmq;
}

void rmq_connection_destroy(RMQConnection *rmq)
{
    if (rmq == NULL)
        return;

    rmq_disconnect(rmq);

    octstr_destroy(rmq->host);
    octstr_destroy(rmq->vhost);
    octstr_destroy(rmq->username);
    octstr_destroy(rmq->password);
    octstr_destroy(rmq->queue_send);
    octstr_destroy(rmq->queue_mo);
    octstr_destroy(rmq->queue_dlr);
    octstr_destroy(rmq->queue_failed);
    octstr_destroy(rmq->exchange);
    octstr_destroy(rmq->exchange_type);
    octstr_destroy(rmq->ssl_cacert);
    octstr_destroy(rmq->ssl_cert);
    octstr_destroy(rmq->ssl_key);

    mutex_destroy(rmq->lock);
    gw_free(rmq);
}

int rmq_connect(RMQConnection *rmq)
{
    int status;

    mutex_lock(rmq->lock);

    if (rmq->connected) {
        mutex_unlock(rmq->lock);
        return 0;
    }

    info(0, "Connecting to RabbitMQ at %s:%d...",
         octstr_get_cstr(rmq->host), rmq->port);

    /* Create connection */
    rmq->conn = amqp_new_connection();
    if (rmq->conn == NULL) {
        error(0, "Failed to create AMQP connection");
        mutex_unlock(rmq->lock);
        return -1;
    }

    /* Create socket (SSL or TCP) */
#ifdef HAVE_RABBITMQ_SSL
    if (rmq->use_ssl) {
        rmq->socket = amqp_ssl_socket_new(rmq->conn);
        if (rmq->socket == NULL) {
            error(0, "Failed to create AMQP SSL socket");
            amqp_destroy_connection(rmq->conn);
            rmq->conn = NULL;
            mutex_unlock(rmq->lock);
            return -1;
        }

        /* Set CA certificate */
        if (rmq->ssl_cacert != NULL) {
            if (amqp_ssl_socket_set_cacert(rmq->socket,
                    octstr_get_cstr(rmq->ssl_cacert)) != AMQP_STATUS_OK) {
                error(0, "Failed to set CA certificate");
            }
        }

        /* Set client certificate and key */
        if (rmq->ssl_cert != NULL && rmq->ssl_key != NULL) {
            if (amqp_ssl_socket_set_key(rmq->socket,
                    octstr_get_cstr(rmq->ssl_cert),
                    octstr_get_cstr(rmq->ssl_key)) != AMQP_STATUS_OK) {
                error(0, "Failed to set client certificate/key");
            }
        }

        /* Set peer verification */
        amqp_ssl_socket_set_verify_peer(rmq->socket, rmq->ssl_verify);
        amqp_ssl_socket_set_verify_hostname(rmq->socket, rmq->ssl_verify);
    } else
#endif
    {
        rmq->socket = amqp_tcp_socket_new(rmq->conn);
        if (rmq->socket == NULL) {
            error(0, "Failed to create AMQP TCP socket");
            amqp_destroy_connection(rmq->conn);
            rmq->conn = NULL;
            mutex_unlock(rmq->lock);
            return -1;
        }
    }

    /* Connect */
    status = amqp_socket_open(rmq->socket,
                              octstr_get_cstr(rmq->host),
                              rmq->port);
    if (status != AMQP_STATUS_OK) {
        error(0, "Failed to open AMQP socket: %s", amqp_error_string2(status));
        amqp_destroy_connection(rmq->conn);
        rmq->conn = NULL;
        mutex_unlock(rmq->lock);
        return -1;
    }

    /* Login */
    amqp_login(rmq->conn,
               octstr_get_cstr(rmq->vhost),
               0,                           /* channel_max */
               131072,                      /* frame_max */
               rmq->heartbeat,
               AMQP_SASL_METHOD_PLAIN,
               octstr_get_cstr(rmq->username),
               octstr_get_cstr(rmq->password));

    if (check_amqp_reply(rmq->conn, "login") < 0) {
        amqp_destroy_connection(rmq->conn);
        rmq->conn = NULL;
        mutex_unlock(rmq->lock);
        return -1;
    }

    /* Open channel */
    amqp_channel_open(rmq->conn, rmq->channel);
    if (check_amqp_reply(rmq->conn, "channel.open") < 0) {
        amqp_connection_close(rmq->conn, AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(rmq->conn);
        rmq->conn = NULL;
        mutex_unlock(rmq->lock);
        return -1;
    }

    /* Set prefetch (QoS) */
    amqp_basic_qos(rmq->conn, rmq->channel, 0, rmq->prefetch, 0);
    if (check_amqp_reply(rmq->conn, "basic.qos") < 0) {
        warning(0, "Failed to set prefetch, continuing anyway");
    }

    /* Declare queues */
    if (declare_queue(rmq, rmq->queue_send) < 0 ||
        declare_queue(rmq, rmq->queue_mo) < 0 ||
        declare_queue(rmq, rmq->queue_dlr) < 0 ||
        declare_queue(rmq, rmq->queue_failed) < 0) {
        amqp_channel_close(rmq->conn, rmq->channel, AMQP_REPLY_SUCCESS);
        amqp_connection_close(rmq->conn, AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(rmq->conn);
        rmq->conn = NULL;
        mutex_unlock(rmq->lock);
        return -1;
    }

    /* Start consuming from send queue */
    amqp_basic_consume(rmq->conn, rmq->channel,
                       amqp_cstring_bytes(octstr_get_cstr(rmq->queue_send)),
                       amqp_empty_bytes,  /* consumer_tag */
                       0,                 /* no_local */
                       0,                 /* no_ack - we'll ack manually */
                       0,                 /* exclusive */
                       amqp_empty_table);

    if (check_amqp_reply(rmq->conn, "basic.consume") < 0) {
        amqp_channel_close(rmq->conn, rmq->channel, AMQP_REPLY_SUCCESS);
        amqp_connection_close(rmq->conn, AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(rmq->conn);
        rmq->conn = NULL;
        mutex_unlock(rmq->lock);
        return -1;
    }

    rmq->connected = 1;
    info(0, "Connected to RabbitMQ successfully");

    mutex_unlock(rmq->lock);
    return 0;
}

void rmq_disconnect(RMQConnection *rmq)
{
    mutex_lock(rmq->lock);

    if (!rmq->connected) {
        mutex_unlock(rmq->lock);
        return;
    }

    info(0, "Disconnecting from RabbitMQ...");

    amqp_channel_close(rmq->conn, rmq->channel, AMQP_REPLY_SUCCESS);
    amqp_connection_close(rmq->conn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(rmq->conn);

    rmq->conn = NULL;
    rmq->socket = NULL;
    rmq->connected = 0;

    mutex_unlock(rmq->lock);
}

int rmq_is_connected(RMQConnection *rmq)
{
    return rmq->connected;
}

int rmq_reconnect(RMQConnection *rmq)
{
    rmq_disconnect(rmq);
    gwthread_sleep(1.0);  /* Wait before reconnecting */
    return rmq_connect(rmq);
}

Octstr *rmq_consume(RMQConnection *rmq, double timeout)
{
    amqp_envelope_t envelope;
    amqp_rpc_reply_t reply;
    struct timeval tv;
    Octstr *body = NULL;

    if (!rmq->connected)
        return NULL;

    /* Set timeout */
    tv.tv_sec = (long)timeout;
    tv.tv_usec = (long)((timeout - tv.tv_sec) * 1000000);

    amqp_maybe_release_buffers(rmq->conn);

    reply = amqp_consume_message(rmq->conn, &envelope, &tv, 0);

    if (reply.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
        if (reply.library_error == AMQP_STATUS_TIMEOUT) {
            /* Timeout is normal, not an error */
            return NULL;
        }
        error(0, "RabbitMQ consume error: %s", amqp_error_string2(reply.library_error));
        rmq->connected = 0;
        return NULL;
    }

    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        error(0, "RabbitMQ consume failed");
        rmq->connected = 0;
        return NULL;
    }

    /* Extract message body */
    body = octstr_create_from_data(envelope.message.body.bytes,
                                   envelope.message.body.len);

    /* Store delivery tag for later ack/nack (we'll need to pass this back somehow) */
    /* For now, we'll ack immediately - TODO: improve this */
    rmq_ack(rmq, envelope.delivery_tag);

    amqp_destroy_envelope(&envelope);

    return body;
}

int rmq_ack(RMQConnection *rmq, uint64_t delivery_tag)
{
    int ret;

    if (!rmq->connected)
        return -1;

    ret = amqp_basic_ack(rmq->conn, rmq->channel, delivery_tag, 0);
    if (ret != 0) {
        error(0, "Failed to ack message: %s", amqp_error_string2(ret));
        return -1;
    }

    return 0;
}

int rmq_nack(RMQConnection *rmq, uint64_t delivery_tag, int requeue)
{
    int ret;

    if (!rmq->connected)
        return -1;

    ret = amqp_basic_nack(rmq->conn, rmq->channel, delivery_tag, 0, requeue);
    if (ret != 0) {
        error(0, "Failed to nack message: %s", amqp_error_string2(ret));
        return -1;
    }

    return 0;
}

int rmq_publish(RMQConnection *rmq, Octstr *queue, Octstr *message)
{
    return rmq_publish_ex(rmq, NULL, queue, message);
}

int rmq_publish_ex(RMQConnection *rmq, Octstr *exchange, Octstr *routing_key, Octstr *message)
{
    int ret;
    amqp_basic_properties_t props;

    if (!rmq->connected)
        return -1;

    /* Set message properties */
    memset(&props, 0, sizeof(props));
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG |
                   AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("application/json");
    props.delivery_mode = 2;  /* persistent */

    mutex_lock(rmq->lock);

    ret = amqp_basic_publish(
        rmq->conn,
        rmq->channel,
        exchange ? amqp_cstring_bytes(octstr_get_cstr(exchange)) : amqp_empty_bytes,
        amqp_cstring_bytes(octstr_get_cstr(routing_key)),
        0,      /* mandatory */
        0,      /* immediate */
        &props,
        amqp_cstring_bytes(octstr_get_cstr(message))
    );

    mutex_unlock(rmq->lock);

    if (ret != AMQP_STATUS_OK) {
        error(0, "Failed to publish message: %s", amqp_error_string2(ret));
        return -1;
    }

    return 0;
}

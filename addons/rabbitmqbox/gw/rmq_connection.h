/*
 * rmq_connection.h - RabbitMQ connection handling for rabbitmqbox
 *
 * Copyright (c) 2026 Vasil Jamalashvili <shapeless@pm.me>
 * MIT License
 */

#ifndef RMQ_CONNECTION_H
#define RMQ_CONNECTION_H

#include "gwlib/gwlib.h"
#include <amqp.h>
#include <amqp_tcp_socket.h>

/* RabbitMQ connection state */
typedef struct {
    amqp_connection_state_t conn;
    amqp_socket_t *socket;
    amqp_channel_t channel;

    /* Connection parameters */
    Octstr *host;
    int port;
    Octstr *vhost;
    Octstr *username;
    Octstr *password;
    int heartbeat;
    int prefetch;

    /* TLS/SSL */
    int use_ssl;
    Octstr *ssl_cacert;
    Octstr *ssl_cert;
    Octstr *ssl_key;
    int ssl_verify;

    /* Queue names */
    Octstr *queue_send;      /* Consume from: outbound SMS */
    Octstr *queue_mo;        /* Publish to: incoming SMS */
    Octstr *queue_dlr;       /* Publish to: delivery reports */
    Octstr *queue_failed;    /* Publish to: failed messages */

    /* Exchange (optional) */
    Octstr *exchange;
    Octstr *exchange_type;

    /* State */
    int connected;
    Mutex *lock;
} RMQConnection;

/* Initialize RabbitMQ connection from config */
RMQConnection *rmq_connection_create(CfgGroup *grp);

/* Destroy RabbitMQ connection */
void rmq_connection_destroy(RMQConnection *rmq);

/* Connect to RabbitMQ server */
int rmq_connect(RMQConnection *rmq);

/* Disconnect from RabbitMQ server */
void rmq_disconnect(RMQConnection *rmq);

/* Check if connected */
int rmq_is_connected(RMQConnection *rmq);

/* Reconnect if connection lost */
int rmq_reconnect(RMQConnection *rmq);

/* Consume a message from queue (blocking with timeout)
 * Returns: Octstr* with JSON message body, or NULL on timeout/error
 * Caller must destroy returned Octstr
 */
Octstr *rmq_consume(RMQConnection *rmq, double timeout);

/* Acknowledge a message */
int rmq_ack(RMQConnection *rmq, uint64_t delivery_tag);

/* Negative acknowledge (requeue) */
int rmq_nack(RMQConnection *rmq, uint64_t delivery_tag, int requeue);

/* Publish a message to queue */
int rmq_publish(RMQConnection *rmq, Octstr *queue, Octstr *message);

/* Publish a message to exchange with routing key */
int rmq_publish_ex(RMQConnection *rmq, Octstr *exchange, Octstr *routing_key, Octstr *message);

/* Get last error message */
const char *rmq_last_error(RMQConnection *rmq);

#endif /* RMQ_CONNECTION_H */

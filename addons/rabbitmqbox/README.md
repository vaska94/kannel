# rabbitmqbox

> **WARNING: IN DEVELOPMENT - NOT READY FOR PRODUCTION USE**
>
> This addon is under active development and has not been tested.
> Do not use in production environments.

RabbitMQ message queue integration for Kamex SMS Gateway.

## Overview

rabbitmqbox bridges RabbitMQ message queues with Kamex bearerbox, enabling:
- **Send SMS via queue**: Applications publish messages to RabbitMQ, rabbitmqbox delivers them through Kamex
- **Receive MO via queue**: Incoming SMS (Mobile Originated) are published to RabbitMQ for application processing
- **Receive DLR via queue**: Delivery reports are published to RabbitMQ

## Architecture

```
                          +--------------+
  Your Application  <-->  |   RabbitMQ   |  <-->  rabbitmqbox  <-->  bearerbox  <-->  SMSC
                          +--------------+

  Queues:
    sms.send   <- Application publishes SMS requests
    sms.mo     -> MO messages published by rabbitmqbox
    sms.dlr    -> DLR messages published by rabbitmqbox
    sms.failed -> Invalid/failed messages
```

## Requirements

- Kamex (or Kannel) gateway installed
- librabbitmq-c >= 0.10.0
- RabbitMQ server

## Installation

### From source

```bash
# Install dependencies (Debian/Ubuntu)
apt-get install librabbitmq-dev

# Install dependencies (RHEL/CentOS)
yum install librabbitmq-devel

# Build
cd addons/rabbitmqbox
autoreconf -fi
./configure --with-kannel-dir=/usr/local
make
make install
```

### Configure

Copy the example configuration:

```bash
cp example/rabbitmqbox.conf.example /etc/kamex/rabbitmqbox.conf
```

Edit `/etc/kamex/rabbitmqbox.conf` to match your environment.

## Configuration

### Core settings

```
group = core
admin-port = 13015
admin-password = secret
log-file = "/var/log/kamex/rabbitmqbox.log"
log-level = 0
```

### Box settings

```
group = rabbitmqbox

# Bearerbox connection
bearerbox-host = 127.0.0.1
bearerbox-port = 13001
box-id = rabbitmqbox

# SMSC routing (optional)
route-to-smsc = my-smsc

# Authentication (optional)
allowed-senders = /etc/kamex/allowed-senders.txt

# Persistence (optional)
store-file = /var/spool/kamex/rabbitmqbox.store

# RabbitMQ connection
rabbitmq-host = 127.0.0.1
rabbitmq-port = 5672
rabbitmq-vhost = /
rabbitmq-user = guest
rabbitmq-pass = guest
rabbitmq-heartbeat = 60
rabbitmq-prefetch = 100

# TLS/SSL (optional)
rabbitmq-ssl = true
rabbitmq-ssl-cacert = /etc/ssl/certs/ca-certificates.crt
rabbitmq-ssl-verify = true

# Queue names
queue-send = sms.send
queue-mo = sms.mo
queue-dlr = sms.dlr
queue-failed = sms.failed
```

### Additional Options

| Option | Description |
|--------|-------------|
| route-to-smsc | Default SMSC for messages without smsc-id |
| allowed-senders | File with allowed sender IDs (one per line) |
| store-file | File for persisting failed messages |
| rabbitmq-ssl | Enable TLS/SSL (port defaults to 5671) |
| rabbitmq-ssl-cacert | CA certificate file |
| rabbitmq-ssl-cert | Client certificate file |
| rabbitmq-ssl-key | Client private key file |
| rabbitmq-ssl-verify | Verify server certificate (true/false) |

## Message Format

### Sending SMS (queue-send)

Publish JSON to `sms.send` queue:

```json
{
  "from": "MyService",
  "to": "+1234567890",
  "text": "Hello World",
  "smsc-id": "smsc1",
  "dlr-mask": 31,
  "coding": 0,
  "charset": "UTF-8",
  "priority": 0,
  "validity": 1440,
  "deferred": 0
}
```

| Field | Required | Description |
|-------|----------|-------------|
| from | Yes | Sender ID |
| to | Yes | Destination phone number |
| text | Yes | Message text |
| smsc-id | No | Route to specific SMSC |
| dlr-mask | No | DLR request mask (default: 0) |
| coding | No | Data coding scheme (default: 0) |
| charset | No | Character set (default: UTF-8) |
| priority | No | Priority 0-3 (default: 0) |
| validity | No | Validity in minutes |
| deferred | No | Deferred delivery in minutes |
| udh | No | User Data Header (hex encoded) |
| mclass | No | Message class |

### Receiving MO (queue-mo)

Messages published to `sms.mo`:

```json
{
  "type": "mo",
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "from": "+1234567890",
  "to": "12345",
  "text": "Hello",
  "smsc-id": "smsc1",
  "coding": 0,
  "timestamp": 1705312200
}
```

### Receiving DLR (queue-dlr)

Delivery reports published to `sms.dlr`:

```json
{
  "type": "dlr",
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "from": "+1234567890",
  "to": "MyService",
  "smsc-id": "smsc1",
  "dlr-type": 1,
  "coding": 0,
  "timestamp": 1705312200
}
```

DLR type values:
- 1 = Delivered to phone
- 2 = Non-delivered to phone
- 4 = Queued on SMSC
- 8 = Delivered to SMSC
- 16 = Non-delivered to SMSC

## Running

```bash
# Start rabbitmqbox
rabbitmqbox /etc/kamex/rabbitmqbox.conf

# With debug logging
rabbitmqbox -d /etc/kamex/rabbitmqbox.conf

# Show version
rabbitmqbox -v
```

## Example: Python Client

```python
import pika
import json

# Connect to RabbitMQ
connection = pika.BlockingConnection(
    pika.ConnectionParameters('localhost')
)
channel = connection.channel()

# Declare queue
channel.queue_declare(queue='sms.send', durable=True)

# Send SMS
message = {
    "from": "MyApp",
    "to": "+1234567890",
    "text": "Hello from Python!",
    "dlr-mask": 31
}

channel.basic_publish(
    exchange='',
    routing_key='sms.send',
    body=json.dumps(message),
    properties=pika.BasicProperties(
        delivery_mode=2,  # persistent
        content_type='application/json'
    )
)

print("SMS sent to queue")
connection.close()
```

## License

MIT License - see LICENSE file.

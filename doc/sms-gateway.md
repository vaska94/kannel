# SMS Gateway Setup

This guide covers setting up Kannel as an SMS gateway.

## Architecture Overview

```
+--------+     HTTP      +--------+     Internal    +-----------+     SMSC      +------+
|  App   | -----------> | SMSBox | --------------> | BearerBox | ------------> | SMSC |
+--------+   API         +--------+    Protocol     +-----------+   Protocol    +------+
                              |                           |
                              |                           |
                         sendsms                    SMPP/CIMD/EMI/etc.
```

**BearerBox**: Core daemon that manages SMSC connections
**SMSBox**: HTTP interface for applications to send/receive SMS

## Starting the Gateway

### 1. Start BearerBox (Core)

```bash
bearerbox /etc/kannel/kannel.conf
```

Or with systemd:
```bash
systemctl start kannel-bearerbox
```

### 2. Start SMSBox

```bash
smsbox /etc/kannel/kannel.conf
```

Or with systemd:
```bash
systemctl start kannel-smsbox
```

## Sending SMS via HTTP

Kannel supports two methods for sending SMS:
1. **Query String** - Traditional URL parameters (GET or POST)
2. **JSON API** - Modern JSON request/response (POST only)

### Basic Send (Query String)

```bash
curl "http://localhost:13013/cgi-bin/sendsms?\
user=api&pass=secret&\
to=+358401234567&\
from=MYAPP&\
text=Hello+World"
```

### JSON API

Send SMS using JSON with token authentication:

```bash
curl -X POST http://localhost:13013/cgi-bin/sendsms \
  -H "X-API-Key: your_api_token" \
  -H "Content-Type: application/json" \
  -d '{"to":"+358401234567","from":"MYAPP","text":"Hello World"}'
```

**Response (success):**
```json
{"status":0,"message":"Sent."}
```

**Response (error):**
```json
{"error":"Authorization failed"}
```

### JSON Request Fields

| Field | Required | Description |
|-------|----------|-------------|
| `to` | Yes | Recipient phone number |
| `text` | Yes | Message text |
| `from` | No | Sender ID |
| `smsc` | No | Force specific SMSC |
| `charset` | No | Character set (default: UTF-8) |
| `coding` | No | Data coding (0=GSM, 1=binary, 2=UCS2) |
| `mclass` | No | Message class (0-3) |
| `udh` | No | User Data Header (hex string) |
| `dlr-mask` | No | Delivery report mask (1-31) |
| `dlr-url` | No | DLR callback URL |
| `validity` | No | Message validity in minutes |
| `deferred` | No | Deferred delivery in minutes |
| `priority` | No | Message priority (0-3) |
| `meta-data` | No | Custom metadata |

### JSON with Delivery Report

```bash
curl -X POST http://localhost:13013/cgi-bin/sendsms \
  -H "X-API-Key: your_api_token" \
  -H "Content-Type: application/json" \
  -d '{
    "to": "+358401234567",
    "text": "Hello",
    "dlr-mask": 31,
    "dlr-url": "http://myapp.com/dlr?msgid=%d&status=%d"
  }'
```

### Query String Parameters

| Parameter | Required | Description |
|-----------|----------|-------------|
| `user` | Yes | SendSMS username |
| `pass` | Yes | SendSMS password |
| `to` | Yes | Recipient phone number |
| `text` | Yes* | Message text (URL-encoded) |
| `from` | No | Sender ID |
| `smsc` | No | Force specific SMSC |
| `mclass` | No | Message class (0-3) |
| `coding` | No | Data coding (0=GSM, 1=binary, 2=UCS2) |
| `charset` | No | Character set for text |
| `udh` | No | User Data Header (hex) |
| `dlr-mask` | No | Delivery report mask |
| `dlr-url` | No | DLR callback URL |

### Send with Delivery Report

```bash
curl "http://localhost:13013/cgi-bin/sendsms?\
user=api&pass=secret&\
to=+358401234567&\
text=Hello&\
dlr-mask=31&\
dlr-url=http://myapp.com/dlr?msgid=%d&status=%d"
```

### Send Binary/UDH Message

```bash
# Send concatenated SMS (UDH for multipart)
curl "http://localhost:13013/cgi-bin/sendsms?\
user=api&pass=secret&\
to=+358401234567&\
udh=050003FF0201&\
text=Part+1+of+message"
```

### Send Unicode (UCS2)

```bash
curl "http://localhost:13013/cgi-bin/sendsms?\
user=api&pass=secret&\
to=+358401234567&\
charset=UTF-8&\
coding=2&\
text=Hello+%D0%9C%D0%B8%D1%80"  # "Hello Мир" in Cyrillic
```

## Receiving SMS (MO)

### Configure SMS Service

```ini
group = sms-service
keyword = INFO
get-url = "http://myapp.com/sms?from=%p&to=%P&text=%a&smsc=%i"
max-messages = 0                # Don't send reply
```

### HTTP Callback Parameters

Your application receives:

| Parameter | Description |
|-----------|-------------|
| `from` (%p) | Sender phone number |
| `to` (%P) | Recipient (your short code) |
| `text` (%a) | Message text (URL-encoded) |
| `smsc` (%i) | SMSC ID |
| `time` (%t) | Timestamp |
| `udh` (%u) | UDH if present (hex) |
| `coding` (%c) | Data coding scheme |

### Reply to Incoming SMS

Your HTTP endpoint can return a reply:

```
HTTP/1.1 200 OK
Content-Type: text/plain

Thank you for your message!
```

Or for no reply, return empty 200 or `204 No Content`.

## HTTP Admin Interface

### Check Status

```bash
curl "http://localhost:13000/status?password=secret"
curl "http://localhost:13000/status.xml?password=secret"
```

### Health Check Endpoint

The `/health` endpoint provides a simple health check for load balancers and container orchestration systems (Kubernetes, Docker Swarm, etc.). It requires no authentication.

```bash
curl "http://localhost:13000/health"
```

Returns JSON with HTTP 200 if healthy, HTTP 503 if unhealthy:

```json
{
  "status": "running",
  "healthy": true,
  "uptime_seconds": 3600,
  "smscs": {
    "total": 2,
    "online": 2
  },
  "queued_messages": 0
}
```

**Health Criteria:**
- Gateway status is running/isolated/suspended/full (not shutdown/dead)
- At least one SMSC is online (if any are configured)

**Use Cases:**
- Kubernetes liveness/readiness probes
- Load balancer health checks
- Monitoring systems

### Control SMSCs

```bash
# Stop an SMSC
curl "http://localhost:13000/stop-smsc?password=secret&smsc=smpp1"

# Start an SMSC
curl "http://localhost:13000/start-smsc?password=secret&smsc=smpp1"

# Restart an SMSC
curl "http://localhost:13000/restart-smsc?password=secret&smsc=smpp1"
```

### Reload SMSCs

```bash
# Reload online (add new SMSCs from config)
curl "http://localhost:13000/reload-smsc?password=secret"
```

### Shutdown

```bash
curl "http://localhost:13000/shutdown?password=secret"
# Or graceful (wait for queues to empty):
curl "http://localhost:13000/suspend?password=secret"
curl "http://localhost:13000/shutdown?password=secret"
```

## Message Routing

### By Prefix

```ini
group = smsc
smsc = smpp
smsc-id = finland
allowed-prefix = "358"

group = smsc
smsc = smpp
smsc-id = sweden
allowed-prefix = "46"

group = smsc
smsc = smpp
smsc-id = default
# Handles all other prefixes
```

### By SMSC ID

In SMS service:
```ini
group = sms-service
keyword = default
accepted-smsc = smpp1;smpp2     # Only from these SMSCs
```

In sendsms user:
```ini
group = sendsms-user
username = api
password = secret
forced-smsc = smpp1             # Always use smpp1
# Or
default-smsc = smpp1            # Use smpp1 if not specified
```

### Via HTTP

```bash
curl "http://localhost:13013/cgi-bin/sendsms?\
user=api&pass=secret&\
to=+358401234567&\
text=Hello&\
smsc=smpp1"                     # Force specific SMSC
```

## Logging

### Log Levels

| Level | Description |
|-------|-------------|
| 0 | Debug (verbose) |
| 1 | Info |
| 2 | Warning |
| 3 | Error |
| 4 | Panic |

### Configuration

```ini
group = core
log-file = /var/log/kannel/bearerbox.log
log-level = 1
access-log = /var/log/kannel/access.log

group = smsbox
log-file = /var/log/kannel/smsbox.log
log-level = 1
```

## Performance Tuning

### Throughput Control

```ini
group = smsc
smsc = smpp
smsc-id = smpp1
throughput = 100                # Max 100 msg/sec to this SMSC
```

### Connection Pooling

```ini
group = core
smsbox-max-pending = 100        # Max pending messages per smsbox

group = smsc
smsc = smpp
smsc-id = smpp1
# Use multiple connections
```

### Message Store

For high availability, use external store:

```ini
group = core
store-type = redis

group = redis-connection
id = store
host = localhost
port = 6379
max-connections = 10

group = store-db
id = store
table = kannel_queue
```

## Troubleshooting

### SMSC Not Connecting

1. Check logs: `tail -f /var/log/kannel/bearerbox.log`
2. Verify network: `telnet smsc.example.com 2775`
3. Check credentials
4. Verify firewall rules

### Messages Not Delivering

1. Check SMSC status: `curl http://localhost:13000/status?password=secret`
2. Check message queue
3. Verify routing rules
4. Check DLR for status

### High Memory Usage

1. Check message queue size in status
2. Reduce `smsbox-max-pending`
3. Use external message store
4. Check for SMSC connectivity issues causing queue buildup

## See Also

- [Configuration Guide](configuration.md)
- [SMSC Types](smsc-types.md)
- [Delivery Reports](dlr.md)

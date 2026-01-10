# Kamex Configuration Guide

Kamex uses a single configuration file with multiple "groups" defining different components.

## Configuration File Syntax

```ini
# Comments start with #
group = group-name
directive = value
another-directive = "value with spaces"
```

## Core Group (Required)

The `core` group configures the bearerbox daemon.

```ini
group = core
admin-port = 13000
admin-password = secret

# Optional settings
smsbox-port = 13001              # Port for smsbox connections
log-file = /var/log/kannel/bearerbox.log
log-level = 0                    # 0=debug, 1=info, 2=warning, 3=error, 4=panic
access-log = /var/log/kannel/access.log

# IP access control
admin-deny-ip = "*.*.*.*"
admin-allow-ip = "127.0.0.1"
box-deny-ip = "*.*.*.*"
box-allow-ip = "127.0.0.1"

# Message store (for persistent queue)
store-file = /var/lib/kannel/kannel.store
# Or use Redis/database storage
store-type = redis

# SSL/TLS for admin interface
ssl-certkey-file = /etc/kannel/kannel.pem

# Unified prefix normalization
unified-prefix = "00358,+"       # Convert +358... and 0... to 00358...
```

### Core Directives Reference

| Directive | Type | Description |
|-----------|------|-------------|
| `admin-port` | integer | HTTP admin interface port |
| `admin-password` | string | Password for admin commands |
| `status-password` | string | Password for status-only access |
| `smsbox-port` | integer | Port for smsbox connections |
| `log-file` | path | Log file location |
| `log-level` | 0-4 | Logging verbosity |
| `access-log` | path | HTTP access log |
| `store-file` | path | Message store file |
| `store-type` | string | `file`, `redis`, `mysql`, etc. |
| `unified-prefix` | string | Number normalization rules |

## SMSBox Group

Configures the SMS box daemon that provides HTTP API.

```ini
group = smsbox
bearerbox-host = localhost
sendsms-port = 13013

# Optional settings
log-file = /var/log/kannel/smsbox.log
log-level = 0
access-log = /var/log/kannel/smsbox-access.log

# Default sender ID
global-sender = "KANNEL"

# Reply messages
reply-couldnotfetch = "Service temporarily unavailable"
reply-requestfailed = "Request failed"
reply-emptymessage = ""
```

### SMSBox Directives Reference

| Directive | Type | Description |
|-----------|------|-------------|
| `bearerbox-host` | hostname | Bearerbox hostname |
| `sendsms-port` | integer | HTTP sendsms port |
| `global-sender` | string | Default sender ID |
| `sendsms-chars` | string | Allowed characters in sender |

## SendSMS User Group

Defines users allowed to send SMS via HTTP API.

```ini
group = sendsms-user
username = myapp
password = secret123

# Token-based authentication (for JSON API)
api-token = sk_live_abc123xyz

# Optional access control
user-deny-ip = "*.*.*.*"
user-allow-ip = "10.0.0.0/8;192.168.0.0/16"

# Optional defaults
default-smsc = smpp1
forced-smsc = smpp1             # Always use this SMSC
max-messages = 10               # Max messages per request
concatenation = true            # Allow long SMS
```

### Authentication Methods

Kamex supports two authentication methods:

1. **Username/Password** - Traditional query string or X-Kannel headers
   ```bash
   curl "http://host:13013/cgi-bin/sendsms?user=myapp&pass=secret123&to=..."
   ```

2. **API Token** - Modern X-API-Key header (recommended for JSON API)
   ```bash
   curl -X POST http://host:13013/cgi-bin/sendsms \
     -H "X-API-Key: sk_live_abc123xyz" \
     -H "Content-Type: application/json" \
     -d '{"to":"123","text":"Hello"}'
   ```

### SendSMS User Directives

| Directive | Type | Description |
|-----------|------|-------------|
| `username` | string | Username for authentication |
| `password` | string | Password for authentication |
| `api-token` | string | API token for X-API-Key header auth |
| `user-allow-ip` | IP list | Allowed client IPs |
| `user-deny-ip` | IP list | Denied client IPs |
| `forced-smsc` | string | Always use this SMSC |
| `default-smsc` | string | Default SMSC if not specified |
| `max-messages` | integer | Max messages per request |
| `concatenation` | boolean | Allow long SMS splitting |

## SMS Service Group

Defines how incoming SMS messages are handled.

```ini
group = sms-service
keyword = INFO
get-url = "http://myapp.example.com/sms?from=%p&text=%a"

# Or respond with static text
# text = "Thank you for your message"

# Optional settings
accepted-smsc = smpp1;smpp2     # Only from these SMSCs
allowed-prefix = "358;46"       # Only from these prefixes
max-messages = 3                # Max reply messages
catch-all = false               # Catch unmatched keywords
```

### Keyword Matching

```ini
# Exact match
keyword = HELP

# Multiple aliases
keyword = INFO
aliases = "INFORMATION;DETAILS"

# Default handler (catch-all)
keyword = default
text = "Unknown command. Reply HELP for assistance."
```

### URL Placeholders

| Placeholder | Description |
|-------------|-------------|
| `%p` | Sender phone number |
| `%P` | Receiver phone number |
| `%a` | Message text (URL-encoded) |
| `%b` | Message as binary (hex) |
| `%t` | Timestamp |
| `%i` | SMSC ID |
| `%n` | Service name |
| `%k` | Keyword |
| `%s` | Text after keyword |

## SMSC Group

See [SMSC Types](smsc-types.md) for protocol-specific configuration.

Common SMSC directives:

```ini
group = smsc
smsc = smpp                     # Protocol type
smsc-id = smpp1                 # Unique identifier

# Routing rules
allowed-prefix = "358;46"
denied-prefix = "1900"
preferred-smsc-id = smpp1
```

## Redis Connection Group

For Redis-based message store or DLR storage. Works with Redis and Valkey.

```ini
group = redis-connection
id = redis1

# TCP connection
host = localhost
port = 6379

# Or Unix socket (faster, recommended for local Redis/Valkey)
#unix-socket = /run/redis/redis.sock

password = secret
database = 0
max-connections = 5

# Set Redis TIMEOUT to prevent idle disconnects (0 = disable)
idle-timeout = 0
```

### Redis Connection Directives

| Directive | Type | Description |
|-----------|------|-------------|
| `id` | string | Connection identifier |
| `host` | hostname | Redis server hostname |
| `port` | integer | Redis server port (default: 6379) |
| `unix-socket` | path | Unix socket path (alternative to host/port) |
| `password` | string | Redis AUTH password |
| `database` | integer | Redis database number (default: 0) |
| `max-connections` | integer | Connection pool size |
| `idle-timeout` | integer | Set Redis TIMEOUT config (0 = disable) |

## Store-DB Group

Links a database connection to message storage.

```ini
group = store-db
id = redis1                     # References redis-connection id
table = kannel_store
hash = no                       # no=faster, yes=readable
```

## Include Directive

Split configuration across multiple files:

```ini
include = /etc/kannel/smsc/*.conf
include = /etc/kannel/services.conf
```

## Example Complete Configuration

```ini
# /etc/kannel/kannel.conf

group = core
admin-port = 13000
admin-password = changeme
smsbox-port = 13001
log-file = /var/log/kannel/bearerbox.log
log-level = 1

include = /etc/kannel/smsc.conf

group = smsbox
bearerbox-host = localhost
sendsms-port = 13013
log-file = /var/log/kannel/smsbox.log

group = sendsms-user
username = api
password = secret

group = sms-service
keyword = default
text = "Message received"
```

## See Also

- [SMS Gateway Setup](sms-gateway.md) - Running the gateway
- [SMSC Types](smsc-types.md) - SMSC protocol configuration
- [Delivery Reports](dlr.md) - DLR storage configuration
- [Examples](examples/) - Example configuration files

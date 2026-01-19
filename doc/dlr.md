# Delivery Reports (DLR)

Delivery reports confirm message delivery status from the SMSC.

## How DLR Works

1. Application sends SMS with `dlr-mask` and `dlr-url`
2. Kamex stores DLR info and sends message to SMSC
3. SMSC delivers message to recipient
4. SMSC sends delivery status back to Kamex
5. Kamex calls `dlr-url` with status

## Requesting DLR

### DLR Mask

The `dlr-mask` is a bitmask specifying which statuses to report:

| Bit | Value | Status |
|-----|-------|--------|
| 0 | 1 | Delivered to phone |
| 1 | 2 | Non-delivered to phone |
| 2 | 4 | Queued on SMSC |
| 3 | 8 | Delivered to SMSC |
| 4 | 16 | Non-delivered to SMSC |

Common masks:
- `1` - Only successful delivery
- `3` - Delivery or failure (1+2)
- `7` - All phone statuses (1+2+4)
- `31` - All statuses (1+2+4+8+16)

### Example Request

```bash
curl "http://localhost:13013/cgi-bin/sendsms?\
user=api&pass=secret&\
to=+358401234567&\
text=Hello&\
dlr-mask=31&\
dlr-url=http://myapp.com/dlr?id=%d&status=%d"
```

### DLR URL Placeholders

| Placeholder | Description |
|-------------|-------------|
| `%d` | DLR type (status code) |
| `%A` | DLR status text |
| `%i` | SMSC ID |
| `%I` | Message ID from SMSC |
| `%p` | Sender |
| `%P` | Recipient |
| `%t` | Submit timestamp |
| `%T` | Done timestamp |

## DLR Status Codes

| Code | Description |
|------|-------------|
| 1 | Delivered to phone |
| 2 | Non-delivered to phone |
| 4 | Queued on SMSC |
| 8 | Delivered to SMSC |
| 16 | Non-delivered to SMSC |

## DLR Storage Backends

DLR info must be stored between send and callback. Kamex supports:

### Internal (Memory)

Default, no configuration needed. Data lost on restart.

```ini
group = core
dlr-storage = internal
```

### MySQL / MariaDB

```ini
group = core
dlr-storage = mysql

group = mysql-connection
id = dlr
host = localhost
username = kannel
password = secret
database = kannel
max-connections = 5

group = dlr-db
id = dlr
table = dlr
```

Create table:
```sql
CREATE TABLE dlr (
    smsc VARCHAR(40),
    ts VARCHAR(40),
    source VARCHAR(40),
    destination VARCHAR(40),
    service VARCHAR(40),
    url VARCHAR(255),
    mask INT,
    status INT,
    boxc VARCHAR(40)
);
CREATE INDEX dlr_idx ON dlr(smsc, ts, destination);
```

### PostgreSQL

```ini
group = core
dlr-storage = pgsql

group = pgsql-connection
id = dlr
host = localhost
username = kannel
password = secret
database = kannel
max-connections = 5

group = dlr-db
id = dlr
table = dlr
```

### SQLite3

```ini
group = core
dlr-storage = sqlite3

group = sqlite3-connection
id = dlr
database = /var/lib/kannel/dlr.db
max-connections = 1

group = dlr-db
id = dlr
table = dlr
```

### Redis / Valkey

```ini
group = core
dlr-storage = redis

group = redis-connection
id = dlr
# TCP connection
host = localhost
port = 6379
# Or Unix socket
#socket = /var/run/redis/redis.sock
password = secret
database = 0
max-connections = 5

group = dlr-db
id = dlr
table = dlr
```

Redis stores DLR as hash keys with TTL.

### Oracle

Requires [Oracle Instant Client](https://www.oracle.com/database/technologies/instant-client.html) (Basic + SDK).

Build:
```bash
./configure --with-oracle \
    --with-oracle-includes=/path/to/instantclient/sdk/include \
    --with-oracle-libs=/path/to/instantclient
```

Config:
```ini
group = core
dlr-storage = oracle

group = oracle-connection
id = dlr
username = system
password = secret
tnsname = localhost:1521/FREEPDB1
max-connections = 5

group = dlr-db
id = dlr
table = dlr
field-smsc = smsc
field-timestamp = ts
field-source = source
field-destination = destination
field-service = service
field-url = url
field-mask = mask
field-status = status
field-boxc-id = boxc
```

Create table:
```sql
CREATE TABLE dlr (
    smsc VARCHAR2(40),
    ts VARCHAR2(40),
    source VARCHAR2(40),
    destination VARCHAR2(40),
    service VARCHAR2(40),
    url VARCHAR2(255),
    mask NUMBER,
    status NUMBER,
    boxc VARCHAR2(40)
);
CREATE INDEX dlr_idx ON dlr(smsc, ts, destination);
```

### MSSQL (SQL Server)

Requires [FreeTDS](https://www.freetds.org/) library.

Build:
```bash
# Install FreeTDS (Arch: pacman -S freetds, RHEL: dnf install freetds-devel)
./configure --with-mssql=/usr
```

Add server to `/etc/freetds/freetds.conf`:
```ini
[myserver]
    host = sqlserver.example.com
    port = 1433
    tds version = 7.4
```

Config:
```ini
group = core
dlr-storage = mssql

group = mssql-connection
id = dlr
server = myserver
username = sa
password = secret
database = kamex
max-connections = 5

group = dlr-db
id = dlr
table = dlr
field-smsc = smsc
field-timestamp = ts
field-source = source
field-destination = destination
field-service = service
field-url = url
field-mask = mask
field-status = status
field-boxc-id = boxc
```

Create table:
```sql
CREATE TABLE dlr (
    smsc VARCHAR(40),
    ts VARCHAR(40),
    source VARCHAR(40),
    destination VARCHAR(40),
    service VARCHAR(40),
    url VARCHAR(255),
    mask INT,
    status INT,
    boxc VARCHAR(40)
);
CREATE INDEX dlr_idx ON dlr(smsc, ts, destination);
```

## DLR Database Fields

You can customize field names:

```ini
group = dlr-db
id = dlr
table = dlr
field-smsc = smsc_id
field-timestamp = submit_time
field-source = sender
field-destination = recipient
field-service = service_name
field-url = callback_url
field-mask = dlr_mask
field-status = dlr_status
field-boxc = box_id
```

## DLR Expiry

DLRs expire after a timeout:

```ini
group = core
dlr-storage = redis

group = dlr-db
id = dlr
table = dlr
ttl = 86400                     # 24 hours (Redis only)
```

For databases, implement cleanup via cron:
```sql
DELETE FROM dlr WHERE ts < DATE_SUB(NOW(), INTERVAL 24 HOUR);
```

## Per-SMSC DLR Handling

Some SMSCs have specific DLR behaviors:

```ini
group = smsc
smsc = smpp
smsc-id = smpp1
# ...
msg-id-type = 1                 # 0=string, 1=hex, 2=decimal
```

## Troubleshooting DLR

### DLR Not Received

1. Check `dlr-mask` is set correctly
2. Verify DLR storage is configured
3. Check SMSC supports DLR
4. Verify `dlr-url` is accessible from Kannel

### DLR Delayed

1. Check SMSC configuration
2. Some carriers delay DLR significantly
3. Check for network issues

### DLR Storage Full

For Redis:
```bash
redis-cli INFO keyspace
```

For MySQL:
```sql
SELECT COUNT(*) FROM dlr;
```

Clean old entries or increase storage.

## Example DLR Callback Handler

PHP example:
```php
<?php
$msg_id = $_GET['id'] ?? '';
$status = $_GET['status'] ?? '';
$smsc = $_GET['smsc'] ?? '';

// Log the DLR
error_log("DLR: id=$msg_id status=$status smsc=$smsc");

// Update your database
$pdo->prepare("UPDATE messages SET status=? WHERE smsc_msg_id=?")
    ->execute([$status, $msg_id]);

// Return 200 OK
http_response_code(200);
echo "OK";
```

## See Also

- [SMS Gateway Setup](sms-gateway.md) - Sending SMS with DLR
- [Configuration Guide](configuration.md) - Core configuration
- [SMSC Types](smsc-types.md) - SMSC-specific DLR settings

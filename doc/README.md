# Kamex SMS Gateway Documentation

Kamex is an open source SMS gateway supporting multiple SMSC protocols.

## Documentation

| Document | Description |
|----------|-------------|
| [Configuration Guide](configuration.md) | Core configuration and bearerbox settings |
| [SMS Gateway Setup](sms-gateway.md) | Setting up SMS services and smsbox |
| [SMSC Types](smsc-types.md) | Supported SMSC protocols and configuration |
| [Delivery Reports](dlr.md) | DLR storage backends (MySQL, PostgreSQL, Redis, etc.) |
| [Examples](examples/) | Example configuration files |

## Quick Start

```bash
# 1. Build
autoreconf -fi
./configure --enable-ssl --with-mysql --with-redis
make

# 2. Configure
cp doc/examples/kannel.conf /etc/kannel/
# Edit /etc/kannel/kannel.conf

# 3. Run
bearerbox /etc/kannel/kannel.conf &
smsbox /etc/kannel/kannel.conf &
```

## Architecture

```
                    +-------------+
                    |   SMSBox    | <-- HTTP API (send/receive SMS)
                    +------+------+
                           |
+----------+        +------+------+        +----------+
|   SMSC   |<------>|  BearerBox  |<------>|   SMSC   |
+----------+        +-------------+        +----------+
                    (Core router)
```

- **BearerBox**: Core daemon managing SMSC connections and message routing
- **SMSBox**: HTTP interface for applications

## Supported SMSC Protocols

| Protocol | Type | Description |
|----------|------|-------------|
| SMPP | `smpp` | SMPP 3.3, 3.4, and 5.0 |
| CIMD | `cimd`, `cimd2` | Nokia CIMD 1.37 and 2.0 |
| EMI/UCP | `emi`, `emi2` | CMG UCP/EMI 4.0 and 3.5 |
| HTTP | `http` | HTTP-based SMSC (`kannel`, `generic`) |
| AT Modem | `at` | GSM modems via AT commands |
| SMASI | `smasi` | SM/ASI for CriticalPath InVoke |
| OIS | `ois`, `oisd` | Sema Group SMS2000 OIS |
| Fake | `fake` | Testing SMSC |
| Loopback | `loopback` | MT to MO direction switching |

## DLR Storage Backends

- Internal (memory)
- MySQL / MariaDB
- PostgreSQL
- SQLite3
- Redis / Valkey (TCP or Unix socket)

## Resources

- Website: https://kamex.dev
- GitHub: https://github.com/vaska94/kannel
- Issues: https://github.com/vaska94/kannel/issues

## Legacy Documentation

The `userguide/` directory contains the original DocBook-generated HTML documentation.
It covers both WAP and SMS functionality - note that WAP has been removed from this fork.

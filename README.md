# Kannel SMS Gateway

[![Version](https://img.shields.io/badge/version-1.6.5-blue.svg)](https://github.com/vaska94/kannel/releases/tag/v1.6.5)
[![License](https://img.shields.io/badge/license-Kannel-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)]()

Open source SMS gateway supporting SMPP, CIMD, EMI/UCP, HTTP, and GSM modems.

> **Note**: This is an independent fork maintained at [github.com/vaska94/kannel](https://github.com/vaska94/kannel).
> It has **no affiliation** with the original Kannel project (kannel.org).
> WAP components have been removed - this is an SMS-only gateway.

## Features

- **Multi-protocol support**: SMPP 3.3/3.4/5.0, CIMD, EMI/UCP, HTTP, AT modems
- **High performance**: 3,000+ messages/sec on commodity hardware
- **HTTP API**: Simple REST-like interface for sending/receiving SMS
- **Delivery reports**: Configurable DLR with multiple storage backends
- **Database support**: MySQL, PostgreSQL, SQLite3, Redis/Valkey
- **Health check endpoint**: `/health` for load balancers and Kubernetes
- **Production ready**: Battle-tested SMS gateway since 2000

## Quick Start

```bash
# Install dependencies (RHEL/Rocky/AlmaLinux 10)
sudo dnf install epel-release
sudo crb enable
sudo dnf install gcc make autoconf automake libtool libxml2-devel \
    openssl-devel pkgconfig hiredis-devel gettext-devel

# Build
autoreconf -fi
./configure --enable-ssl --with-redis
make

# Configure
sudo mkdir -p /etc/kannel
sudo cp doc/examples/kannel.conf /etc/kannel/
# Edit /etc/kannel/kannel.conf with your SMSC details

# Run
./gw/bearerbox /etc/kannel/kannel.conf &
./gw/smsbox /etc/kannel/kannel.conf &

# Send SMS via HTTP
curl "http://localhost:13013/cgi-bin/sendsms?user=tester&pass=foobar&to=+1234567890&text=Hello"
```

## Documentation

| Document | Description |
|----------|-------------|
| [Configuration Guide](doc/configuration.md) | Core configuration options |
| [SMS Gateway Setup](doc/sms-gateway.md) | HTTP API and message routing |
| [SMSC Types](doc/smsc-types.md) | Protocol-specific configuration |
| [Delivery Reports](doc/dlr.md) | DLR storage backends |
| [Examples](doc/examples/) | Sample configuration files |

## Supported SMSC Protocols

| Protocol | Config | Description |
|----------|--------|-------------|
| SMPP | `smsc = smpp` | SMPP 3.3, 3.4, 5.0 (most common) |
| CIMD | `smsc = cimd2` | Nokia CIMD 1.37 and 2.0 |
| EMI/UCP | `smsc = emi2` | CMG UCP/EMI 4.0 and 3.5 |
| HTTP | `smsc = http` | HTTP-based gateways |
| AT Modem | `smsc = at` | GSM modems via serial/USB |
| SMASI | `smsc = smasi` | CriticalPath InVoke |
| OIS | `smsc = ois` | Sema Group SMS2000 |

## Architecture

```
+--------+     HTTP      +--------+    Internal     +-----------+     SMSC      +------+
|  App   | -----------> | SMSBox | --------------> | BearerBox | ------------> | SMSC |
+--------+   :13013      +--------+    Protocol     +-----------+   Protocol    +------+
```

- **BearerBox**: Core daemon managing SMSC connections and message routing
- **SMSBox**: HTTP interface for applications to send/receive SMS

## Build Dependencies

<details>
<summary>RHEL/Rocky/AlmaLinux (EL9+)</summary>

```bash
sudo dnf install epel-release
sudo crb enable
sudo dnf install gcc make autoconf automake libtool libxml2-devel \
    openssl-devel pkgconfig mariadb-devel libpq-devel \
    libsqlite3x-devel hiredis-devel gettext-devel
```
</details>

<details>
<summary>Fedora</summary>

```bash
sudo dnf install gcc make autoconf automake libtool libxml2-devel \
    openssl-devel pkgconfig mariadb-devel libpq-devel \
    libsqlite3x-devel hiredis-devel gettext-devel
```
</details>

<details>
<summary>Debian/Ubuntu</summary>

```bash
sudo apt install build-essential autotools-dev autoconf automake \
    libtool libxml2-dev libssl-dev pkg-config \
    libmysqlclient-dev libpq-dev libsqlite3-dev libhiredis-dev
```
</details>

<details>
<summary>Arch Linux</summary>

```bash
sudo pacman -S base-devel autoconf automake libtool libxml2 openssl \
    mariadb-libs postgresql-libs sqlite hiredis
```
</details>

## Build Options

Full build with all features:
```bash
./configure --enable-ssl --disable-ssl-thread-test \
            --with-mysql --with-pgsql --with-sqlite3 --with-redis \
            ac_cv_sys_file_offset_bits=64
```

| Option | Description |
|--------|-------------|
| `--enable-ssl` | Enable SSL/TLS support |
| `--disable-ssl-thread-test` | Skip SSL threading test (recommended) |
| `--with-mysql` | MySQL/MariaDB support |
| `--with-pgsql` | PostgreSQL support |
| `--with-sqlite3` | SQLite3 support |
| `--with-redis` | Redis/Valkey support |
| `ac_cv_sys_file_offset_bits=64` | Fix file offset detection on modern 64-bit systems |

Minimal build (no databases):
```bash
./configure ac_cv_sys_file_offset_bits=64
```

## Performance

Benchmark results (WSL2, AMD Ryzen 9 5550X):

| Test | Throughput | Latency |
|------|------------|---------|
| HTTP API | ~2,778 req/sec | - |
| SMS/SMPP | ~3,125 msg/sec | 339ms avg |

Run your own benchmarks:
```bash
make ssl-certs  # Required for benchmarks
./benchmarks/run-benchmarks benchmarks/bench_http.sh benchmarks/bench_sms.sh
```

## systemd Integration

```bash
# Install systemd service files
sudo ./contrib/systemd/setup-kannel-user.sh
sudo systemctl enable kannel-bearerbox kannel-smsbox
sudo systemctl start kannel-bearerbox kannel-smsbox
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run tests: `make check`
5. Submit a pull request

## License

Kannel is licensed under the Kannel Software License (BSD-style).
See [LICENSE](LICENSE) for details.

## Links

- **Issues**: [github.com/vaska94/kannel/issues](https://github.com/vaska94/kannel/issues)
- **Documentation**: [doc/](doc/)

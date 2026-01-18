# Kamex SMS Gateway

[![Version](https://img.shields.io/badge/version-1.8.1-blue.svg)](https://github.com/vaska94/Kamex/releases)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)]()
[![Docker](https://img.shields.io/badge/docker-ghcr.io-blue.svg)](https://ghcr.io/vaska94/kamex)

Modern, high-performance SMS gateway with built-in admin panel. Supports SMPP, EMI/UCP, HTTP, and GSM modems.

> **Kamex** is a maintained fork of Kannel with new features and active development.
> Original Kannel code remains under its original license (see LICENSE.kannel).
> Configuration files are compatible with Kannel.

## What's New in Kamex

- **Prometheus Metrics** - Native `/metrics` endpoint for Prometheus/Grafana monitoring
- **Async Logging** - Non-blocking logging with dedicated writer thread (~10x throughput)
- **Web Admin Panel** - Built-in dashboard at `/` with real-time monitoring
- **JSON API** - Modern `/status.json` and `/api/sendsms` endpoints
- **Health Checks** - `/health` endpoint for load balancers and Kubernetes
- **Redis/Valkey** - Native support for DLR and message store
- **Reproducible Builds** - Enterprise-grade build verification with `SOURCE_DATE_EPOCH`
- **Removed Legacy** - Dropped RADIUS, WAP, libxml2, and dead SMSC protocols

## Features

- **Multi-protocol support**: SMPP 3.3/3.4/5.0, EMI/UCP, HTTP, AT modems
- **High performance**: 16,000+ messages/sec on commodity hardware
- **HTTP API**: Simple REST-like interface for sending/receiving SMS
- **Web Admin Panel**: Real-time dashboard, SMSC control, message queue viewer
- **Delivery reports**: Configurable DLR with multiple storage backends
- **Database support**: MySQL, PostgreSQL, SQLite3, Redis/Valkey, Cassandra, Oracle*, MSSQL*
  <br><sub>*untested</sub>
- **Health check endpoint**: `/health` for load balancers and Kubernetes
- **Prometheus metrics**: `/metrics` endpoint for monitoring with Prometheus/Grafana
- **Production ready**: Battle-tested SMS gateway since 2000

## Installation

### 1. Install Dependencies

<details open>
<summary>RHEL/Rocky/AlmaLinux (EL9+)</summary>

```bash
sudo dnf install epel-release
sudo crb enable
sudo dnf install gcc make autoconf automake libtool \
    openssl-devel pkgconfig mariadb-devel libpq-devel \
    libsqlite3x-devel hiredis-devel gettext-devel
```
</details>

<details>
<summary>Fedora</summary>

```bash
sudo dnf install gcc make autoconf automake libtool \
    openssl-devel pkgconfig mariadb-devel libpq-devel \
    libsqlite3x-devel hiredis-devel gettext-devel
```
</details>

<details>
<summary>Debian/Ubuntu</summary>

```bash
sudo apt install build-essential autotools-dev autoconf automake \
    libtool libssl-dev pkg-config \
    libmysqlclient-dev libpq-dev libsqlite3-dev libhiredis-dev
```
</details>

<details>
<summary>Arch Linux</summary>

```bash
sudo pacman -S base-devel autoconf automake libtool openssl \
    mariadb-libs postgresql-libs sqlite hiredis
```
</details>

### 2. Build and Install

```bash
autoreconf -fi
./configure --enable-ssl --with-mysql --with-pgsql --with-sqlite3 --with-redis
make
sudo make install-strip
```

| Configure Option | Description |
|------------------|-------------|
| `--enable-ssl` | Enable SSL/TLS support |
| `--with-mysql` | MySQL/MariaDB support |
| `--with-pgsql` | PostgreSQL support |
| `--with-sqlite3` | SQLite3 support |
| `--with-redis` | Redis/Valkey support |
| `--with-cassandra` | Cassandra support |
| `--with-oracle` | Oracle support (untested) |
| `--with-mssql` | MSSQL support (untested) |
| `--enable-reproducible` | Reproducible builds (auto if SOURCE_DATE_EPOCH set) |

### 3. Configure and Run

```bash
# Configure
sudo mkdir -p /etc/kamex
sudo cp doc/examples/kannel.conf /etc/kamex/kamex.conf
# Edit /etc/kamex/kamex.conf with your SMSC details

# Run
bearerbox /etc/kamex/kamex.conf &
smsbox /etc/kamex/kamex.conf &

# Open admin panel
open http://localhost:13000/

# Test SMS sending (classic)
curl "http://localhost:13013/cgi-bin/sendsms?user=tester&pass=foobar&from=Kamex&to=+1234567890&text=Hello"

# Test SMS sending (JSON API)
curl -X POST http://localhost:13013/cgi-bin/sendsms \
  -H "Content-Type: application/json" \
  -H "X-API-Key: your-api-token" \
  -d '{"from":"Kamex","to":"+1234567890","text":"Hello"}'
```

## Admin Panel

Access the built-in admin panel at `http://localhost:13000/`

- **Dashboard**: SMS/DLR traffic, SMSC status, connected boxes
- **Queue**: View pending messages in store
- **Send SMS**: Test SMS sending
- **Controls**: Gateway management (suspend/resume/shutdown)

Use `admin-password` for full control, `status-password` for view-only access.

## Documentation

| Document | Description |
|----------|-------------|
| [Configuration Guide](doc/configuration.md) | Core configuration options |
| [SMS Gateway Setup](doc/sms-gateway.md) | HTTP API and message routing |
| [SMSC Types](doc/smsc-types.md) | Protocol-specific configuration |
| [Delivery Reports](doc/dlr.md) | DLR storage backends |
| [Logging](doc/logging.md) | Async logging architecture |
| [Metrics](doc/metrics.md) | Prometheus monitoring setup |
| [OpenAPI](doc/openapi.yaml) | API specification (Swagger) |
| [Docker](doc/docker.md) | Docker deployment guide |
| [Addons](doc/addons.md) | Building SQLBox and OpenSMPPBox |
| [Examples](doc/examples/) | Sample configuration files |

## Supported SMSC Protocols

| Protocol | Config | Description |
|----------|--------|-------------|
| SMPP | `smsc = smpp` | SMPP 3.3, 3.4, 5.0 (industry standard) |
| EMI/UCP | `smsc = emi` | CMG UCP/EMI (European operators) |
| HTTP | `smsc = http` | HTTP-based gateways |
| AT Modem | `smsc = at` | GSM modems via serial/USB |

> **Note:** Legacy protocols (CIMD, SMASI, OIS, SEMA, CGW, EMI/X.25) were removed in v1.7.5.
> These protocols have no active deployments. Use SMPP instead.

## Architecture

```
+--------+     HTTP      +--------+    Internal     +-----------+     SMSC      +------+
|  App   | -----------> | SMSBox | --------------> | BearerBox | ------------> | SMSC |
+--------+   :13013      +--------+    Protocol     +-----------+   Protocol    +------+
                                                          |
                                                    Admin Panel
                                                      :13000
```

- **BearerBox**: Core daemon managing SMSC connections and message routing
- **SMSBox**: HTTP interface for applications to send/receive SMS

## Performance

Benchmark results (Linux, Intel i5-13500):

| Test | Throughput | Latency |
|------|------------|---------|
| HTTP API | ~14,000 req/sec | - |
| SMS/SMPP | ~16,000 msg/sec | 80ms avg |

Run your own benchmarks:
```bash
make ssl-certs  # Required for benchmarks
./benchmarks/run-benchmarks benchmarks/bench_http.sh benchmarks/bench_sms.sh
```

## Docker

No build required - just create two files and run:

```bash
mkdir kamex && cd kamex
# Create docker-compose.yml and kamex.conf (see doc/docker.md)
docker compose up -d
```

Includes bearerbox, smsbox, and Valkey for DLR storage. See [Docker Guide](doc/docker.md) for full setup.

## systemd Integration

```bash
# Install systemd service files
sudo ./contrib/systemd/setup-kamex-user.sh
sudo systemctl enable kamex-bearerbox kamex-smsbox
sudo systemctl start kamex-bearerbox kamex-smsbox
```

## Reproducible Builds

Kamex supports [reproducible builds](https://reproducible-builds.org/) for enterprise verification and compliance.

```bash
# Build with fixed timestamp from git commit
export SOURCE_DATE_EPOCH=$(git log -1 --format=%ct)
autoreconf -fi
./configure --enable-ssl --with-redis
make

# Verify reproducibility - all binaries have identical hashes
sha256sum gw/.libs/bearerbox gw/.libs/smsbox gwlib/.libs/libgwlib.so
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run tests: `make check`
5. Submit a pull request

## License

Kamex is dual-licensed:
- **New code**: MIT License (see [LICENSE](LICENSE))
- **Original Kannel code**: Kannel Software License 1.0 (see [LICENSE.kannel](LICENSE.kannel))

## Links

- **Website**: [kamex.dev](https://kamex.dev)
- **Issues**: [GitHub Issues](https://github.com/vaska94/Kamex/issues)
- **Documentation**: [doc/](doc/)

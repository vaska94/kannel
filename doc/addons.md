# Building Kamex Addons

Kamex has two optional addon boxes that extend gateway functionality:

- **SQLBox** - Database queue for SMS messages
- **OpenSMPPBox** - SMPP proxy server

Both require the main Kamex gateway to be built and installed first.

## Prerequisites

Build and install the main gateway:

```bash
cd /path/to/kamex
autoreconf -fi
./configure --prefix=/usr/local
make
sudo make install
```

## SQLBox

SQLBox sits between SMSBox and BearerBox, storing messages in a database queue.

```
App -> SMSBox -> SQLBox -> BearerBox -> SMSC
```

### Building SQLBox

```bash
cd addons/sqlbox
autoreconf -fi
./configure --with-kannel-dir=/usr/local
make
sudo make install
```

### Configure Options

| Option | Description |
|--------|-------------|
| `--with-kannel-dir=DIR` | Kamex install prefix (default: /usr/local) |
| `--enable-ssl` | Enable SSL support |
| `--with-mysql` | MySQL/MariaDB support (from Kamex) |
| `--with-pgsql` | PostgreSQL support (from Kamex) |
| `--with-mssql=DIR` | FreeTDS for MSSQL |

### Running SQLBox

```bash
sqlbox /etc/kamex/sqlbox.conf
```

See `addons/sqlbox/example/sqlbox.conf.example` for configuration.

## OpenSMPPBox

OpenSMPPBox acts as an SMPP server, allowing external SMPP clients to connect to Kamex.

```
SMPP Client -> OpenSMPPBox -> BearerBox -> SMSC
```

### Building OpenSMPPBox

```bash
cd addons/opensmppbox
autoreconf -fi
./configure --with-kannel-dir=/usr/local
make
sudo make install
```

### Configure Options

| Option | Description |
|--------|-------------|
| `--with-kannel-dir=DIR` | Kamex install prefix (default: /usr/local) |
| `--enable-ssl` | Enable SSL support |
| `--enable-pam` | Enable PAM authentication |

### Running OpenSMPPBox

```bash
opensmppbox /etc/kamex/opensmppbox.conf
```

See `addons/opensmppbox/example/opensmppbox.conf.example` for configuration.

## systemd Services

If using RPM packages or manual systemd setup:

```bash
# SQLBox
sudo systemctl enable kamex-sqlbox
sudo systemctl start kamex-sqlbox

# OpenSMPPBox
sudo systemctl enable kamex-opensmppbox
sudo systemctl start kamex-opensmppbox
```

Service files are in `contrib/systemd/`.

# Command Line Usage

## bearerbox

The core gateway daemon that manages SMSC connections and message routing.

```
bearerbox [OPTIONS] [config-file]
```

### Options

| Flag | Long Form | Description |
|------|-----------|-------------|
| `-t` | `--test` | Test configuration and exit (nginx-style) |
| `-S` | `--suspended` | Start in suspended mode (messages queued, not delivered) |
| `-I` | `--isolated` | Start in isolated mode (no new messages accepted) |
| `-v` | `--verbosity` | Console log level (0=debug, 1=info, 2=warning, 3=error, 4=panic) |
| `-F` | `--logfile` | Log file path |
| `-V` | `--fileverbosity` | Log file verbosity level |
| `-D` | `--debug` | Enable debug for specific places (e.g., `-D bb.sms`) |
| `-d` | `--daemonize` | Run as daemon in background |
| `-p` | `--pid-file` | PID file path |
| `-u` | `--user` | Run as specified user |
| `-P` | `--parachute` | Enable parachute (restart on crash) |
| `-X` | `--panic-script` | Script to run on panic |
| `-g` | `--generate` | Generate UUID and exit |
| | `--version` | Show version and exit |

### Examples

```bash
# Test configuration
bearerbox -t /etc/kamex/kamex.conf

# Run with debug logging
bearerbox -v 0 /etc/kamex/kamex.conf

# Run as daemon
bearerbox -d -p /var/run/kamex/bearerbox.pid /etc/kamex/kamex.conf

# Start suspended (for maintenance)
bearerbox -S /etc/kamex/kamex.conf
```

## smsbox

The HTTP interface for sending and receiving SMS messages.

```
smsbox [OPTIONS] [config-file]
```

### Options

| Flag | Long Form | Description |
|------|-----------|-------------|
| `-t` | `--test` | Test configuration and exit (nginx-style) |
| `-v` | `--verbosity` | Console log level (0=debug, 1=info, 2=warning, 3=error, 4=panic) |
| `-F` | `--logfile` | Log file path |
| `-V` | `--fileverbosity` | Log file verbosity level |
| `-D` | `--debug` | Enable debug for specific places |
| `-d` | `--daemonize` | Run as daemon in background |
| `-p` | `--pid-file` | PID file path |
| `-u` | `--user` | Run as specified user |
| `-P` | `--parachute` | Enable parachute (restart on crash) |
| `-X` | `--panic-script` | Script to run on panic |
| | `--version` | Show version and exit |

### Examples

```bash
# Test configuration
smsbox -t /etc/kamex/kamex.conf

# Run with info logging to file
smsbox -F /var/log/kamex/smsbox.log -V 1 /etc/kamex/kamex.conf

# Run as daemon with specific user
smsbox -d -u kamex -p /var/run/kamex/smsbox.pid /etc/kamex/kamex.conf
```

## Configuration Test

Both bearerbox and smsbox support nginx-style configuration testing:

```bash
$ bearerbox -t /etc/kamex/kamex.conf
bearerbox: configuration file /etc/kamex/kamex.conf syntax is ok
bearerbox: configuration file /etc/kamex/kamex.conf test is successful

$ smsbox -t /etc/kamex/kamex.conf
smsbox: configuration file /etc/kamex/kamex.conf syntax is ok
smsbox: configuration file /etc/kamex/kamex.conf test is successful
```

Exit codes:
- `0` - Configuration is valid
- `1` - Configuration has errors

This is useful for CI/CD pipelines:

```bash
# Validate before deployment
bearerbox -t /etc/kamex/kamex.conf && smsbox -t /etc/kamex/kamex.conf
```

## Signals (Hot-Reload)

Bearerbox supports runtime signals for zero-downtime operations:

| Signal | Effect |
|--------|--------|
| `SIGHUP` | Hot-reload configuration and re-open logs |
| `SIGUSR2` | Re-open log files only (for logrotate) |
| `SIGQUIT` | Report memory usage (debug) |
| `SIGTERM` | Graceful shutdown |
| `SIGINT` | Graceful shutdown |

### SIGHUP - Config Hot-Reload

Reload configuration without restarting:

```bash
# Send SIGHUP to bearerbox
kill -HUP $(cat /var/run/kamex/bearerbox.pid)

# Or using systemctl
systemctl reload kamex-bearerbox
```

What gets reloaded:
- **SMSC connections** - New SMSCs added, removed SMSCs stopped, changed SMSCs restarted
- **Routing rules** - Updated without restarting unchanged SMSCs
- **Black/white lists** - Reloaded from files
- **Log files** - Re-opened (for logrotate)

What does NOT change (requires restart):
- Listen ports (admin-port, smsbox-port)
- SSL certificates
- Core settings (store-type, etc.)

### SIGUSR2 - Log Rotation

Re-open log files without config reload:

```bash
kill -USR2 $(cat /var/run/kamex/bearerbox.pid)
```

Use with logrotate:

```
/var/log/kamex/*.log {
    daily
    rotate 7
    compress
    postrotate
        kill -USR2 $(cat /var/run/kamex/bearerbox.pid) 2>/dev/null || true
    endscript
}
```

### Systemd Integration

The systemd service file supports reload:

```bash
# Reload config (sends SIGHUP)
systemctl reload kamex-bearerbox

# Check status
systemctl status kamex-bearerbox
```

## Debug Places

The `-D` flag enables debug output for specific code areas:

```bash
# Debug SMS routing
bearerbox -D bb.sms /etc/kamex/kamex.conf

# Debug SMPP protocol
bearerbox -D smsc.smpp /etc/kamex/kamex.conf

# Multiple debug areas
bearerbox -D bb.sms -D smsc.smpp /etc/kamex/kamex.conf
```

## See Also

- [Configuration Guide](configuration.md)
- [Logging](logging.md)
- [Docker](docker.md)

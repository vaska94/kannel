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

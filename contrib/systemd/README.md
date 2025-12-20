# Kannel Systemd Service Files

This directory contains systemd service files for running Kannel components as system services with enhanced security and monitoring.

## Prerequisites

### User and Group Creation

1. Create a dedicated kannel user and group for security:
   ```bash
   # Create kannel group
   sudo groupadd kannel
   
   # Create kannel user with system account settings
   sudo useradd -r -g kannel -d /var/lib/kannel -s /bin/false -c "Kannel SMS Gateway" kannel
   
   # Create home directory for kannel user
   sudo mkdir -p /var/lib/kannel
   sudo chown kannel:kannel /var/lib/kannel
   sudo chmod 750 /var/lib/kannel
   ```

2. Create required directories with proper permissions:
   ```bash
   sudo mkdir -p /var/log/kannel /var/spool/kannel /var/run/kannel /etc/kannel
   sudo chown -R kannel:kannel /var/log/kannel /var/spool/kannel /var/run/kannel
   sudo chmod 755 /var/log/kannel /var/spool/kannel /var/run/kannel
   sudo chmod 755 /etc/kannel
   ```

### Alternative: Automated User Setup

For convenience, you can use this one-liner to set up everything:
```bash
curl -sSL https://raw.githubusercontent.com/vaska94/Kannel/main/contrib/systemd/setup-kannel-user.sh | sudo bash
```

Or manually run the setup script:
```bash
sudo /usr/src/Kannel/contrib/systemd/setup-kannel-user.sh
```

## Installation

1. Copy the service files to systemd directory:
   ```bash
   sudo cp kannel-bearerbox.service /etc/systemd/system/
   sudo cp kannel-smsbox.service /etc/systemd/system/
   ```

2. Copy the status checking script:
   ```bash
   sudo cp kannel-status.sh /usr/local/bin/
   sudo chmod +x /usr/local/bin/kannel-status.sh
   ```

3. Install logrotate configuration:
   ```bash
   sudo cp kannel.logrotate /etc/logrotate.d/kannel
   ```

4. Reload systemd configuration:
   ```bash
   sudo systemctl daemon-reload
   ```

4. Enable services to start at boot:
   ```bash
   sudo systemctl enable kannel-bearerbox.service
   sudo systemctl enable kannel-smsbox.service
   ```

5. Start services:
   ```bash
   sudo systemctl start kannel-bearerbox.service
   sudo systemctl start kannel-smsbox.service
   ```

## Configuration

- **Configuration file**: `/etc/kannel/kannel.conf`
- **Log directories**: `/var/log/kannel` (owned by kannel:kannel)
- **Spool directories**: `/var/spool/kannel` (owned by kannel:kannel)
- **Runtime directories**: `/var/run/kannel` (owned by kannel:kannel)

The services now run as the `kannel` user instead of root for improved security.

## Service Management

```bash
# Check systemd service status
sudo systemctl status kannel-bearerbox.service
sudo systemctl status kannel-smsbox.service

# Check Kannel application status (via HTTP admin interface)
sudo /usr/local/bin/kannel-status.sh

# View logs
sudo journalctl -u kannel-bearerbox.service -f
sudo journalctl -u kannel-smsbox.service -f

# Stop services (stop smsbox first, then bearerbox)
sudo systemctl stop kannel-smsbox.service
sudo systemctl stop kannel-bearerbox.service

# Start services (start bearerbox first, then smsbox)
sudo systemctl start kannel-bearerbox.service
sudo systemctl start kannel-smsbox.service

# Restart services
sudo systemctl restart kannel-bearerbox.service
sudo systemctl restart kannel-smsbox.service

# Reload configuration (sends HUP signal)
sudo systemctl reload kannel-bearerbox.service
sudo systemctl reload kannel-smsbox.service
```

## Status Monitoring

The included `kannel-status.sh` script provides detailed status information by querying Kannel's HTTP admin interface:

```bash
# Basic status check
sudo /usr/local/bin/kannel-status.sh

# Status check with custom port
sudo /usr/local/bin/kannel-status.sh --port 13001

# Status check with custom config directory
sudo /usr/local/bin/kannel-status.sh --config-dir /opt/kannel

# Show help
sudo /usr/local/bin/kannel-status.sh --help
```

## Security Features

- **Dedicated user**: Services run as `kannel` user, not root
- **Sandboxing**: Protected system directories and home directories
- **No privilege escalation**: `NoNewPrivileges=true`
- **Timeout controls**: 30-second start/stop timeouts prevent hanging
- **Controlled access**: Only specific directories are writable

## Dependencies

- **kannel-bearerbox.service**: Starts after network is available
- **kannel-smsbox.service**: Requires bearerbox to be running first

If you're using Redis for storage, modify the service files to add Redis dependencies as needed.

## Troubleshooting

1. **Permission errors**: Ensure kannel user owns the required directories
2. **Status check fails**: Verify admin-port and admin-password in kannel.conf
3. **Service won't start**: Check journalctl logs for detailed error messages
4. **Configuration issues**: Use `kannel-status.sh` to verify HTTP admin interface
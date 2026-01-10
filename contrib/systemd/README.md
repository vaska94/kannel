# Kamex Systemd Service Files

This directory contains systemd service files for running Kamex components as system services with enhanced security and monitoring.

## Prerequisites

### User and Group Creation

1. Create a dedicated kamex user and group for security:
   ```bash
   # Create kamex group
   sudo groupadd kamex

   # Create kamex user with system account settings
   sudo useradd -r -g kamex -d /var/lib/kamex -s /bin/false -c "Kamex SMS Gateway" kamex

   # Create home directory for kamex user
   sudo mkdir -p /var/lib/kamex
   sudo chown kamex:kamex /var/lib/kamex
   sudo chmod 750 /var/lib/kamex
   ```

2. Create required directories with proper permissions:
   ```bash
   sudo mkdir -p /var/log/kamex /var/spool/kamex /var/run/kamex /etc/kamex
   sudo chown -R kamex:kamex /var/log/kamex /var/spool/kamex /var/run/kamex
   sudo chmod 755 /var/log/kamex /var/spool/kamex /var/run/kamex
   sudo chmod 755 /etc/kamex
   ```

3. Create tmpfiles.d configuration (ensures /var/run/kamex persists after reboot):
   ```bash
   echo 'd /var/run/kamex 0755 kamex kamex -' | sudo tee /etc/tmpfiles.d/kamex.conf
   ```

### Alternative: Automated User Setup

For convenience, you can use this one-liner to set up everything:
```bash
curl -sSL https://raw.githubusercontent.com/vaska94/Kamex/main/contrib/systemd/setup-kamex-user.sh | sudo bash
```

Or manually run the setup script:
```bash
sudo /usr/src/Kamex/contrib/systemd/setup-kamex-user.sh
```

## Installation

1. Copy the service files to systemd directory:
   ```bash
   sudo cp kamex-bearerbox.service /etc/systemd/system/
   sudo cp kamex-smsbox.service /etc/systemd/system/
   ```

2. Copy the status checking script:
   ```bash
   sudo cp kamex-status.sh /usr/local/bin/
   sudo chmod +x /usr/local/bin/kamex-status.sh
   ```

3. Install logrotate configuration:
   ```bash
   sudo cp kamex.logrotate /etc/logrotate.d/kamex
   ```

4. Reload systemd configuration:
   ```bash
   sudo systemctl daemon-reload
   ```

4. Enable services to start at boot:
   ```bash
   sudo systemctl enable kamex-bearerbox.service
   sudo systemctl enable kamex-smsbox.service
   ```

5. Start services:
   ```bash
   sudo systemctl start kamex-bearerbox.service
   sudo systemctl start kamex-smsbox.service
   ```

## Configuration

- **Configuration file**: `/etc/kamex/kamex.conf`
- **Log directories**: `/var/log/kamex` (owned by kamex:kamex)
- **Spool directories**: `/var/spool/kamex` (owned by kamex:kamex)
- **Runtime directories**: `/var/run/kamex` (owned by kamex:kamex)

The services now run as the `kamex` user instead of root for improved security.

## Service Management

```bash
# Check systemd service status
sudo systemctl status kamex-bearerbox.service
sudo systemctl status kamex-smsbox.service

# Check Kamex application status (via HTTP admin interface)
sudo /usr/local/bin/kamex-status.sh

# View logs
sudo journalctl -u kamex-bearerbox.service -f
sudo journalctl -u kamex-smsbox.service -f

# Stop services (stop smsbox first, then bearerbox)
sudo systemctl stop kamex-smsbox.service
sudo systemctl stop kamex-bearerbox.service

# Start services (start bearerbox first, then smsbox)
sudo systemctl start kamex-bearerbox.service
sudo systemctl start kamex-smsbox.service

# Restart services
sudo systemctl restart kamex-bearerbox.service
sudo systemctl restart kamex-smsbox.service

# Reload configuration (sends HUP signal)
sudo systemctl reload kamex-bearerbox.service
sudo systemctl reload kamex-smsbox.service
```

## Status Monitoring

The included `kamex-status.sh` script provides detailed status information by querying Kamex's HTTP admin interface:

```bash
# Basic status check
sudo /usr/local/bin/kamex-status.sh

# Status check with custom port
sudo /usr/local/bin/kamex-status.sh --port 13001

# Status check with custom config directory
sudo /usr/local/bin/kamex-status.sh --config-dir /opt/kamex

# Show help
sudo /usr/local/bin/kamex-status.sh --help
```

## Security Features

- **Dedicated user**: Services run as `kamex` user, not root
- **Sandboxing**: Protected system directories and home directories
- **No privilege escalation**: `NoNewPrivileges=true`
- **Timeout controls**: 30-second start/stop timeouts prevent hanging
- **Controlled access**: Only specific directories are writable

## Dependencies

- **kamex-bearerbox.service**: Starts after network is available
- **kamex-smsbox.service**: Requires bearerbox to be running first

If you're using Redis for storage, modify the service files to add Redis dependencies as needed.

## Troubleshooting

1. **Permission errors**: Ensure kamex user owns the required directories
2. **Status check fails**: Verify admin-port and admin-password in kamex.conf
3. **Service won't start**: Check journalctl logs for detailed error messages
4. **Configuration issues**: Use `kamex-status.sh` to verify HTTP admin interface

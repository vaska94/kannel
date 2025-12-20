#!/bin/bash
#
# Kannel Setup Script
# This script creates the kannel user, directories, and installs systemd services
#

set -e

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# GitHub raw URL for downloading files
GITHUB_RAW="https://raw.githubusercontent.com/vaska94/kannel/main/contrib/systemd"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to get file (local or download from GitHub)
get_file() {
    local filename="$1"
    local dest="$2"

    if [[ -f "$SCRIPT_DIR/$filename" ]]; then
        cp "$SCRIPT_DIR/$filename" "$dest"
        return 0
    else
        # Try to download from GitHub
        if command -v curl >/dev/null 2>&1; then
            if curl -sSf "$GITHUB_RAW/$filename" -o "$dest" 2>/dev/null; then
                return 0
            fi
        elif command -v wget >/dev/null 2>&1; then
            if wget -q "$GITHUB_RAW/$filename" -O "$dest" 2>/dev/null; then
                return 0
            fi
        fi
    fi
    return 1
}

# Check if running as root
if [[ $EUID -ne 0 ]]; then
    print_error "This script must be run as root (use sudo)"
    exit 1
fi

print_status "Setting up Kannel user and directories..."

# Create kannel group if it doesn't exist
if ! getent group kannel >/dev/null 2>&1; then
    print_status "Creating kannel group..."
    groupadd kannel
    print_status "✓ Kannel group created"
else
    print_warning "Kannel group already exists"
fi

# Create kannel user if it doesn't exist
if ! getent passwd kannel >/dev/null 2>&1; then
    print_status "Creating kannel user..."
    useradd -r -g kannel -d /var/lib/kannel -s /bin/false -c "Kannel SMS Gateway" kannel
    print_status "✓ Kannel user created"
else
    print_warning "Kannel user already exists"
fi

# Create home directory for kannel user
print_status "Creating kannel home directory..."
mkdir -p /var/lib/kannel
chown kannel:kannel /var/lib/kannel
chmod 750 /var/lib/kannel
print_status "✓ Kannel home directory created: /var/lib/kannel"

# Create required directories
print_status "Creating required directories..."
mkdir -p /var/log/kannel /var/spool/kannel /var/run/kannel /etc/kannel

# Set ownership and permissions
chown -R kannel:kannel /var/log/kannel /var/spool/kannel /var/run/kannel
chmod 755 /var/log/kannel /var/spool/kannel /var/run/kannel
chmod 755 /etc/kannel

print_status "✓ Directories created and permissions set:"
print_status "  - /var/log/kannel (kannel:kannel, 755)"
print_status "  - /var/spool/kannel (kannel:kannel, 755)"
print_status "  - /var/run/kannel (kannel:kannel, 755)"
print_status "  - /etc/kannel (root:root, 755)"

# Create tmpfiles.d configuration for /var/run/kannel persistence
print_status "Creating tmpfiles.d configuration..."
echo 'd /var/run/kannel 0755 kannel kannel -' > /etc/tmpfiles.d/kannel.conf
print_status "✓ Created /etc/tmpfiles.d/kannel.conf"

# Install systemd service files
print_status "Installing systemd service files..."
if get_file "kannel-bearerbox.service" "/etc/systemd/system/kannel-bearerbox.service"; then
    print_status "✓ Installed kannel-bearerbox.service"
else
    print_error "Failed to install kannel-bearerbox.service"
    exit 1
fi

if get_file "kannel-smsbox.service" "/etc/systemd/system/kannel-smsbox.service"; then
    print_status "✓ Installed kannel-smsbox.service"
else
    print_error "Failed to install kannel-smsbox.service"
    exit 1
fi

# Install status script
if get_file "kannel-status.sh" "/usr/local/bin/kannel-status.sh"; then
    chmod +x /usr/local/bin/kannel-status.sh
    print_status "✓ Installed kannel-status.sh to /usr/local/bin/"
else
    print_warning "Failed to install kannel-status.sh (optional)"
fi

# Install logrotate configuration
if get_file "kannel.logrotate" "/etc/logrotate.d/kannel"; then
    print_status "✓ Installed logrotate configuration"
else
    print_warning "Failed to install logrotate configuration (optional)"
fi

# Reload systemd
print_status "Reloading systemd daemon..."
systemctl daemon-reload
print_status "✓ Systemd daemon reloaded"

# Verify setup
print_status "Verifying setup..."

# Check user exists
if getent passwd kannel >/dev/null 2>&1; then
    USER_INFO=$(getent passwd kannel)
    print_status "✓ User verification: $USER_INFO"
else
    print_error "✗ Kannel user not found"
    exit 1
fi

# Check group exists
if getent group kannel >/dev/null 2>&1; then
    GROUP_INFO=$(getent group kannel)
    print_status "✓ Group verification: $GROUP_INFO"
else
    print_error "✗ Kannel group not found"
    exit 1
fi

# Check directories exist and have correct permissions
for dir in /var/lib/kannel /var/log/kannel /var/spool/kannel /var/run/kannel /etc/kannel; do
    if [[ -d "$dir" ]]; then
        PERMS=$(stat -c "%a %U:%G" "$dir")
        print_status "✓ Directory $dir exists with permissions: $PERMS"
    else
        print_error "✗ Directory $dir not found"
        exit 1
    fi
done

print_status ""
print_status "🎉 Kannel setup completed successfully!"
print_status ""
print_status "Next steps:"
print_status "1. Place your kannel.conf in /etc/kannel/"
print_status "2. Enable services: systemctl enable kannel-bearerbox kannel-smsbox"
print_status "3. Start services: systemctl start kannel-bearerbox kannel-smsbox"
print_status ""
print_status "For more information, see the README.md in contrib/systemd/"
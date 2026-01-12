#!/bin/bash
#
# Kamex Setup Script
# This script creates the kamex user, directories, and installs systemd services
#

set -e

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# GitHub raw URL for downloading files
GITHUB_RAW="https://raw.githubusercontent.com/vaska94/Kamex/main/contrib/systemd"

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

# Detect sbin directory (where bearerbox is installed)
detect_sbindir() {
    # Check common locations
    if [[ -x "/usr/sbin/bearerbox" ]]; then
        echo "/usr/sbin"
    elif [[ -x "/usr/local/sbin/bearerbox" ]]; then
        echo "/usr/local/sbin"
    else
        # Default to /usr/local/sbin for manual installs
        echo "/usr/local/sbin"
    fi
}

SBINDIR=$(detect_sbindir)
print_status "Detected sbin directory: $SBINDIR"

# Function to get file (local or download from GitHub)
# Handles both generated .service files and .service.in templates
get_file() {
    local filename="$1"
    local dest="$2"

    # First check for already-generated file (from ./configure)
    if [[ -f "$SCRIPT_DIR/$filename" ]]; then
        cp "$SCRIPT_DIR/$filename" "$dest"
        return 0
    fi

    # Check for .in template and substitute
    if [[ -f "$SCRIPT_DIR/${filename}.in" ]]; then
        sed "s|@SBINDIR@|$SBINDIR|g" "$SCRIPT_DIR/${filename}.in" > "$dest"
        return 0
    fi

    # Try to download from GitHub
    local url
    for suffix in "" ".in"; do
        url="$GITHUB_RAW/${filename}${suffix}"
        if command -v curl >/dev/null 2>&1; then
            if curl -sSf "$url" -o "$dest.tmp" 2>/dev/null; then
                if [[ -n "$suffix" ]]; then
                    # Downloaded template, substitute
                    sed "s|@SBINDIR@|$SBINDIR|g" "$dest.tmp" > "$dest"
                    rm -f "$dest.tmp"
                else
                    mv "$dest.tmp" "$dest"
                fi
                return 0
            fi
        elif command -v wget >/dev/null 2>&1; then
            if wget -q "$url" -O "$dest.tmp" 2>/dev/null; then
                if [[ -n "$suffix" ]]; then
                    sed "s|@SBINDIR@|$SBINDIR|g" "$dest.tmp" > "$dest"
                    rm -f "$dest.tmp"
                else
                    mv "$dest.tmp" "$dest"
                fi
                return 0
            fi
        fi
        rm -f "$dest.tmp"
    done
    return 1
}

# Check if running as root
if [[ $EUID -ne 0 ]]; then
    print_error "This script must be run as root (use sudo)"
    exit 1
fi

print_status "Setting up Kamex user and directories..."

# Create kamex group if it doesn't exist
if ! getent group kamex >/dev/null 2>&1; then
    print_status "Creating kamex group..."
    groupadd kamex
    print_status "Created kamex group"
else
    print_warning "Kamex group already exists"
fi

# Create kamex user if it doesn't exist
if ! getent passwd kamex >/dev/null 2>&1; then
    print_status "Creating kamex user..."
    useradd -r -g kamex -d /var/lib/kamex -s /bin/false -c "Kamex SMS Gateway" kamex
    print_status "Created kamex user"
else
    print_warning "Kamex user already exists"
fi

# Create home directory for kamex user
print_status "Creating kamex home directory..."
mkdir -p /var/lib/kamex
chown kamex:kamex /var/lib/kamex
chmod 750 /var/lib/kamex
print_status "Created kamex home directory: /var/lib/kamex"

# Create required directories
print_status "Creating required directories..."
mkdir -p /var/log/kamex /var/spool/kamex /var/run/kamex /etc/kamex

# Set ownership and permissions
chown -R kamex:kamex /var/log/kamex /var/spool/kamex /var/run/kamex
chmod 755 /var/log/kamex /var/spool/kamex /var/run/kamex
chmod 755 /etc/kamex

print_status "Directories created and permissions set:"
print_status "  - /var/log/kamex (kamex:kamex, 755)"
print_status "  - /var/spool/kamex (kamex:kamex, 755)"
print_status "  - /var/run/kamex (kamex:kamex, 755)"
print_status "  - /etc/kamex (root:root, 755)"

# Create tmpfiles.d configuration for /var/run/kamex persistence
print_status "Creating tmpfiles.d configuration..."
echo 'd /var/run/kamex 0755 kamex kamex -' > /etc/tmpfiles.d/kamex.conf
print_status "Created /etc/tmpfiles.d/kamex.conf"

# Install systemd service files
print_status "Installing systemd service files..."
if get_file "kamex-bearerbox.service" "/etc/systemd/system/kamex-bearerbox.service"; then
    print_status "Installed kamex-bearerbox.service"
else
    print_error "Failed to install kamex-bearerbox.service"
    exit 1
fi

if get_file "kamex-smsbox.service" "/etc/systemd/system/kamex-smsbox.service"; then
    print_status "Installed kamex-smsbox.service"
else
    print_error "Failed to install kamex-smsbox.service"
    exit 1
fi

# Install sqlbox service (optional - only if sqlbox is installed)
if get_file "kamex-sqlbox.service" "/etc/systemd/system/kamex-sqlbox.service"; then
    print_status "Installed kamex-sqlbox.service"
else
    print_warning "Failed to install kamex-sqlbox.service (optional - install kamex-sqlbox package)"
fi

# Install opensmppbox service (optional - only if opensmppbox is installed)
if get_file "kamex-opensmppbox.service" "/etc/systemd/system/kamex-opensmppbox.service"; then
    print_status "Installed kamex-opensmppbox.service"
else
    print_warning "Failed to install kamex-opensmppbox.service (optional - install kamex-opensmppbox package)"
fi

# Install status script
if get_file "kamex-status.sh" "/usr/local/bin/kamex-status.sh"; then
    chmod +x /usr/local/bin/kamex-status.sh
    print_status "Installed kamex-status.sh to /usr/local/bin/"
else
    print_warning "Failed to install kamex-status.sh (optional)"
fi

# Install logrotate configuration
if get_file "kamex.logrotate" "/etc/logrotate.d/kamex"; then
    print_status "Installed logrotate configuration"
else
    print_warning "Failed to install logrotate configuration (optional)"
fi

# Reload systemd
print_status "Reloading systemd daemon..."
systemctl daemon-reload
print_status "Systemd daemon reloaded"

# Verify setup
print_status "Verifying setup..."

# Check user exists
if getent passwd kamex >/dev/null 2>&1; then
    USER_INFO=$(getent passwd kamex)
    print_status "User verification: $USER_INFO"
else
    print_error "Kamex user not found"
    exit 1
fi

# Check group exists
if getent group kamex >/dev/null 2>&1; then
    GROUP_INFO=$(getent group kamex)
    print_status "Group verification: $GROUP_INFO"
else
    print_error "Kamex group not found"
    exit 1
fi

# Check directories exist and have correct permissions
for dir in /var/lib/kamex /var/log/kamex /var/spool/kamex /var/run/kamex /etc/kamex; do
    if [[ -d "$dir" ]]; then
        PERMS=$(stat -c "%a %U:%G" "$dir")
        print_status "Directory $dir exists with permissions: $PERMS"
    else
        print_error "Directory $dir not found"
        exit 1
    fi
done

print_status ""
print_status "Kamex setup completed successfully!"
print_status ""
print_status "Next steps:"
print_status "1. Place your kamex.conf in /etc/kamex/"
print_status "2. Enable services: systemctl enable kamex-bearerbox kamex-smsbox"
print_status "3. Start services: systemctl start kamex-bearerbox kamex-smsbox"
print_status ""
print_status "For more information, see the README.md in contrib/systemd/"

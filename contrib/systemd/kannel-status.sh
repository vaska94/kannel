#!/bin/bash
#
# Kannel Status Checking Script for Systemd
# Based on the status functionality from the original init.d script
#

CONFDIR=/etc/kannel
DEFAULT_ADMIN_PORT=13000
DEFAULT_ADMIN_PASS=""

# Function to extract configuration values
get_config_value() {
    local key="$1"
    local config_file="$2"
    grep "^${key}" "$config_file" 2>/dev/null | sed "s/.*=[[:space:]]*//" | head -1
}

# Find the core configuration file
find_core_config() {
    if [ -f "$CONFDIR/kannel.conf" ]; then
        # Check if this file contains core group
        if grep -q 'group[[:space:]]*=[[:space:]]*core' "$CONFDIR/kannel.conf" 2>/dev/null; then
            echo "$CONFDIR/kannel.conf"
            return 0
        fi
    fi
    
    # Search for any config file with core group
    local core_conf=$(grep -r 'group[[:space:]]*=[[:space:]]*core' "$CONFDIR" 2>/dev/null | cut -d: -f1 | head -1)
    if [ -n "$core_conf" ]; then
        echo "$core_conf"
        return 0
    fi
    
    return 1
}

# Main status check function
check_kannel_status() {
    local core_conf
    local admin_port
    local admin_pass
    local status_url
    
    # Find core configuration
    core_conf=$(find_core_config)
    if [ $? -ne 0 ]; then
        echo "Error: Could not find Kannel core configuration file"
        echo "Expected location: $CONFDIR/kannel.conf"
        return 1
    fi
    
    # Extract admin port and password
    admin_port=$(get_config_value "admin-port" "$core_conf")
    admin_pass=$(get_config_value "admin-password" "$core_conf")
    
    # Use defaults if not found
    admin_port=${admin_port:-$DEFAULT_ADMIN_PORT}
    admin_pass=${admin_pass:-$DEFAULT_ADMIN_PASS}
    
    # Construct status URL
    if [ -n "$admin_pass" ]; then
        status_url="http://127.0.0.1:${admin_port}/status.txt?password=${admin_pass}"
    else
        status_url="http://127.0.0.1:${admin_port}/status.txt"
    fi
    
    echo "Checking Kannel status..."
    echo "Config file: $core_conf"
    echo "Admin port: $admin_port"
    echo "Status URL: $status_url"
    echo ""
    
    # Try to fetch status using curl (preferred) or wget
    if command -v curl >/dev/null 2>&1; then
        curl -s --connect-timeout 5 --max-time 10 "$status_url"
        local exit_code=$?
    elif command -v wget >/dev/null 2>&1; then
        wget -q -O - --timeout=10 "$status_url"
        local exit_code=$?
    else
        echo "Error: Neither curl nor wget found. Please install one of them."
        return 1
    fi
    
    if [ $exit_code -eq 0 ]; then
        echo ""
        echo "Status check completed successfully"
        return 0
    else
        echo ""
        echo "Error: Failed to retrieve status (exit code: $exit_code)"
        echo "This may indicate:"
        echo "  - Bearerbox is not running"
        echo "  - Admin interface is not accessible"
        echo "  - Incorrect admin-port or admin-password configuration"
        return 1
    fi
}

# Show usage
show_usage() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  --help, -h          Show this help message"
    echo "  --config-dir DIR    Use alternative config directory (default: $CONFDIR)"
    echo "  --port PORT         Override admin port"
    echo "  --password PASS     Override admin password"
    echo ""
    echo "Examples:"
    echo "  $0                          # Check status with default settings"
    echo "  $0 --port 13001             # Check status on custom port"
    echo "  $0 --config-dir /opt/kannel # Use alternative config directory"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --help|-h)
            show_usage
            exit 0
            ;;
        --config-dir)
            CONFDIR="$2"
            shift 2
            ;;
        --port)
            DEFAULT_ADMIN_PORT="$2"
            shift 2
            ;;
        --password)
            DEFAULT_ADMIN_PASS="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Run the status check
check_kannel_status
exit $?
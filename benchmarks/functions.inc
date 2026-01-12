function check_for_errors {
    if grep 'WARNING:|ERROR:|PANIC:' "$@" >/dev/null
    then
	    echo "$0 failed" 1>&2
	    echo "See bench_sms*.log for info" 1>&2
	    exit 1
    fi
}

function plot {
    base="$1"
    shift

    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    if command -v python3 >/dev/null 2>&1 && python3 -c "import matplotlib" 2>/dev/null; then
        python3 "$SCRIPT_DIR/plot.py" "$base" "$@"
        return 0
    fi

    echo "Warning: python3-matplotlib not available, skipping plots" >&2
    echo "Install with: dnf install epel-release && dnf install python3-matplotlib" >&2
    return 0
}

# Changelog

All notable changes to Kamex (formerly Kannel) will be documented in this file.

## [1.7.5] - 2026-01-10

### Rebrand
- **Renamed from Kannel to Kamex** due to licensing restrictions
- New MIT license for Kamex code, original Kannel code remains under Kannel Software License 1.0
- Configuration files remain compatible with Kannel
- Systemd service files renamed to `kamex-bearerbox`, `kamex-smsbox`
- Paths changed to `/etc/kamex`, `/var/log/kamex`, etc.

### Added
- **Web Admin Panel** - Built-in dashboard at `/` and `/admin` with real-time monitoring
  - Dashboard with SMS/DLR traffic stats and SMSC status
  - Queue viewer showing pending messages from store-status
  - Send SMS form for testing
  - Gateway controls (suspend/resume/shutdown/restart SMSCs)
  - Auto-refresh toggle (5s/15s/30s/Off)
  - Admin mode vs view-only mode detection
- **JSON API** - Modern REST-like endpoints
  - `/api/sendsms` - POST-only JSON endpoint for sending SMS
  - `/status.json` - JSON status output with rates and SMSC details
  - Token authentication via `X-API-Key` header and `api-token` config
- **Health Check** - `/health` endpoint for load balancers and Kubernetes
- CORS headers for smsbox sendsms endpoint

### Removed
- **libxml2 dependency** - No longer required
- **WAP/WML support** - Removed all WAP-related code and files
- **RADIUS support** - Removed RADIUS authentication
- Legacy platform support (Solaris, Interix3, FreeBSD c_r)
- SVN/CVS artifacts and dead code

### Changed
- Admin panel HTML embedded in binary (no external file needed)
- OpenSSL 1.1+ thread safety test skipped (always thread-safe)
- Modernized autoconf configuration

### Fixed
- JSON SMSC status comma handling for multiple SMSCs
- OpenSSL auto-detection for modern distros
- iconv library detection for Linux systems

## [1.6.5] - 2025-12-01

### Added
- Unix socket support for Redis connections
- Systemd service files with security hardening
- Logrotate configuration
- Performance benchmarks
- GitHub-friendly README.md and markdown documentation

### Changed
- Updated build dependencies for Fedora/EL10
- Replaced bootstrap.sh with standard autoreconf

### Fixed
- OpenSSL detection for modern distros
- gettext m4 macros
- Benchmark scripts

## [1.6.4] and earlier

See the original Kannel changelog for historical changes.

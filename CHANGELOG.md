# Changelog

All notable changes to Kamex (formerly Kannel) will be documented in this file.

## [1.7.9] - 2026-01-12

### Added
- **Async logging** - Log messages are now queued and written by a dedicated writer thread
  - Bounded queue (512K entries, ~100MB memory) prevents unbounded growth
  - Calling threads no longer block on I/O - ~10x throughput improvement
  - PANIC level remains synchronous (crash context must hit disk immediately)
  - Per-SMSC exclusive logging preserved via `exclusive_idx` routing
- **Logging observability** - New monitoring endpoints for log queue health
  - `/health` returns `warn` status when queue >= 80% or messages dropped
  - `/status.json` includes `logging` section with queue depth, dropped count, writer status
- **Architecture documentation** - `doc/logging.md` explains async logging design

### Fixed
- **test_headers.c** - Removed WAP/WSP dependencies, now tests HTTP headers only
- **check_sendsms.sh** - Fixed incorrect path and cumulative auth failure count
- **check_headers.sh** - Updated for simplified test_headers
- **run-checks** - Now checks exit codes instead of treating any stderr as failure

### Changed
- Log writer thread uses `gwthread_create()` for proper gwlib integration
- `LogQueueStatus` struct added to `gwlib/log.h` for queue monitoring

## [1.7.8] - 2026-01-12

### Added
- **OpenSMPPBox packaging** - RPM package for kamex-opensmppbox addon
- **OpenSMPPBox systemd service** - `kamex-opensmppbox.service` with security hardening

### Changed
- Modernized OpenSMPPBox configure.ac, removed DocBook build system
- GitHub workflow now builds all 3 packages: kamex, kamex-sqlbox, kamex-opensmppbox

## [1.7.7] - 2026-01-12

### Removed
- **SQLite2 support** - Removed obsolete SQLite 2.x database backend (use SQLite3)
- **libsdb support** - Removed dead libsdb database abstraction library
- Removed ~500 lines of dead code from gwlib, gw, and sqlbox

### Changed
- Cleaned up database pool enum and initialization code
- Updated test_dbpool.c to remove SQLite2 tests

## [1.7.6] - 2026-01-12

### Added
- **SQLBox packaging** - RPM package for kamex-sqlbox addon
- **SQLBox systemd service** - `kamex-sqlbox.service` with security hardening

### Changed
- **Systemd services** - Use `RuntimeDirectory`, `StateDirectory`, `LogsDirectory` for better compatibility
- **Systemd paths** - Service files now use `@SBINDIR@` template for correct paths in both `make install` and RPM

### Fixed
- **Namespace errors** - Fixed `status=226/NAMESPACE` errors in containers/VMs
- **SQLBox build** - Modernized configure.ac, removed DocBook build system

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

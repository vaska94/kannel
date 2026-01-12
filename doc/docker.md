# Docker Deployment

Kamex provides official Docker images for containerized deployments. No need to clone the repo or build anything.

## Quick Start

```bash
mkdir kamex && cd kamex
# Create docker-compose.yml (see below)
# Create kamex.conf (see below)
docker compose up -d
```

## Image

Official image: `ghcr.io/vaska94/kamex:latest`

Based on Red Hat UBI10 minimal. Includes all database backends (MySQL, PostgreSQL, SQLite, Redis).

```bash
# Check version
docker run --rm ghcr.io/vaska94/kamex:latest --version
```

## Docker Compose

Create `docker-compose.yml`:

```yaml
services:
  bearerbox:
    image: ghcr.io/vaska94/kamex:latest
    container_name: kamex-bearerbox
    command: ["/usr/sbin/bearerbox", "/etc/kamex/kamex.conf"]
    volumes:
      - ./kamex.conf:/etc/kamex/kamex.conf:ro
      - kamex-spool:/var/spool/kamex
      - kamex-logs:/var/log/kamex
    ports:
      - "13000:13000"
    networks:
      - kamex
    healthcheck:
      test: ["CMD", "curl", "-sf", "http://localhost:13000/health"]
      interval: 10s
      timeout: 5s
      start_period: 10s
    restart: unless-stopped

  smsbox:
    image: ghcr.io/vaska94/kamex:latest
    container_name: kamex-smsbox
    command: ["/usr/sbin/smsbox", "/etc/kamex/kamex.conf"]
    volumes:
      - ./kamex.conf:/etc/kamex/kamex.conf:ro
      - kamex-logs:/var/log/kamex
    ports:
      - "13013:13013"
    networks:
      - kamex
    depends_on:
      bearerbox:
        condition: service_healthy
    restart: unless-stopped

  valkey:
    image: valkey/valkey:9-alpine
    container_name: kamex-valkey
    volumes:
      - valkey-data:/data
    networks:
      - kamex
    restart: unless-stopped

volumes:
  kamex-spool:
  kamex-logs:
  valkey-data:

networks:
  kamex:
    driver: bridge
```

| Service | Description | Port |
|---------|-------------|------|
| bearerbox | Core gateway, SMSC connections | 13000 |
| smsbox | HTTP API for sending SMS | 13013 |
| valkey | Redis-compatible store for DLR | - |

## Configuration

Create `kamex.conf` with your SMSC settings. See [Configuration Guide](configuration.md) for options.

Minimal example:

```
group = core
admin-port = 13000
admin-password = secret
log-file = /var/log/kamex/bearerbox.log
log-level = 0

group = smsbox
bearerbox-host = bearerbox
sendsms-port = 13013
log-file = /var/log/kamex/smsbox.log

group = sms-service
keyword = default
text = "Hello from Kamex"

group = smsc
smsc = smpp
smsc-id = mysmsc
host = smpp.provider.com
port = 2775
smsc-username = user
smsc-password = pass
system-type = ""
```

Note: Use `bearerbox-host = bearerbox` (container name) for smsbox to connect.

### Volumes

| Volume | Purpose |
|--------|---------|
| kamex-spool | Message store/queue |
| kamex-logs | Log files |
| valkey-data | DLR persistence |

### Commands

```bash
# Start
docker compose up -d

# View logs
docker compose logs -f bearerbox
docker compose logs -f smsbox

# Stop
docker compose down

# Stop and remove volumes
docker compose down -v
```

## Health Checks

The bearerbox container includes a health check on `/health`:

```bash
curl http://localhost:13000/health
```

Docker Compose waits for bearerbox to be healthy before starting smsbox.

## Production Considerations

### Reverse Proxy

Put nginx in front for TLS termination and rate limiting:

```yaml
services:
  nginx:
    image: nginx:alpine
    ports:
      - "443:443"
    volumes:
      - ./nginx.conf:/etc/nginx/nginx.conf:ro
      - ./certs:/etc/nginx/certs:ro
    depends_on:
      - smsbox
```

### Resource Limits

Add resource limits for production:

```yaml
services:
  bearerbox:
    deploy:
      resources:
        limits:
          cpus: '2'
          memory: 1G
```

### Persistent Storage

For production, use named volumes or bind mounts to persistent storage:

```yaml
volumes:
  kamex-spool:
    driver: local
    driver_opts:
      type: none
      o: bind
      device: /data/kamex/spool
```

## Building Custom Image

To build with custom compile options:

```dockerfile
FROM rockylinux:10 AS builder
# Install build deps, compile with custom flags
# ...

FROM registry.access.redhat.com/ubi10/ubi-minimal:latest
COPY --from=builder /usr/local/sbin/bearerbox /usr/sbin/
# ...
```

See `.github/workflows/build-rpm.yml` for the full build process.

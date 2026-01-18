# Logging Architecture

Kamex uses asynchronous logging to prevent log I/O from blocking message processing.

## Overview

```
Application Threads                    Writer Thread
       |                                    |
   debug()  --> format --> enqueue ---------+---> consume --> fputs --> fflush
   info()   --> format --> enqueue ---------|
   error()  --> format --> enqueue ---------|
       |                                    |
   panic()  --> format --> DIRECT WRITE ----+---> fflush (synchronous)
```

## Design Decisions

### Bounded Queue (128K entries)
- Prevents unbounded memory growth under load
- ~512MB max (128K × 4KB), ~0 typical (queue stays empty)
- When full: drop new entries, increment counter
- Similar to Logback's AsyncAppender design

### PANIC is Synchronous
- Crash context must hit disk immediately
- Process may abort() right after
- Bypasses queue entirely

### Per-SMSC Logging
- Each SMSC can have its own log file
- Thread calls `log_thread_to(idx)` to set exclusive file
- `exclusive_idx` captured at enqueue time
- Writer routes to correct file

### No Smart Discard
- Considered Logback-style "drop DEBUG/INFO at 80%"
- Rejected: PANIC is already sync, simpler to just drop all at 100%
- `dropped_total` counter visible in `/status.json`

## Observability

### /health endpoint
Returns `warn` status (HTTP 200) when:
- Queue depth >= 80%
- Any messages dropped

```json
{"status": "warn", "warnings": ["log queue at 85% (223K/262K)"]}
```

### /status.json
```json
{
  "logging": {
    "queue_depth": 1523,
    "queue_max": 524288,
    "queue_percent": 0.6,
    "dropped_total": 0,
    "writer_running": true
  }
}
```

## Configuration

| Option | Default | Description |
|--------|---------|-------------|
| `log-file` | stdout | Main log file path |
| `log-level` | 0 | 0=DEBUG, 1=INFO, 2=WARNING, 3=ERROR, 4=PANIC |
| `log-format` | text | Log format: `text` (default) or `json` |
| `access-log` | none | HTTP access log (smsbox) |

Note: Log levels are inverted from common convention (0 = most verbose).

## Structured JSON Logging

Enable JSON format for structured logging, log aggregation (ELK, Loki, etc.), and machine parsing:

```
group = core
log-format = json
```

### JSON Output Format

```json
{"timestamp":"2024-01-15T10:30:45Z","level":"INFO","pid":12345,"tid":1,"message":"SMS received from +1234567890"}
{"timestamp":"2024-01-15T10:30:45Z","level":"ERROR","pid":12345,"tid":2,"message":"SMSC connection failed: timeout"}
```

### Fields

| Field | Type | Description |
|-------|------|-------------|
| `timestamp` | string | ISO 8601 UTC timestamp |
| `level` | string | DEBUG, INFO, WARNING, ERROR, PANIC |
| `pid` | number | Process ID |
| `tid` | number | Thread ID |
| `message` | string | Log message (JSON-escaped) |

### Use Cases

- **Log aggregation**: Direct ingestion into Elasticsearch, Loki, Splunk
- **Kubernetes**: Works with fluentd, filebeat, promtail sidecars
- **Filtering**: Easy to filter by level, grep by field
- **Alerting**: Parse and alert on specific error patterns

### Example with jq

```bash
# Filter errors only
tail -f /var/log/kamex/bearerbox.log | jq 'select(.level == "ERROR")'

# Count messages by level
cat bearerbox.log | jq -s 'group_by(.level) | map({level: .[0].level, count: length})'
```

## Performance

| Metric | Sync (old) | Async (new) |
|--------|------------|-------------|
| Log calls/sec | ~50k | ~500k+ |
| Thread blocking | Yes | No |
| fflush per log | 1 | 1 per batch |

## Troubleshooting

**High queue_depth**: Writer can't keep up. Check disk I/O, reduce log verbosity.

**dropped_total > 0**: Queue overflowed. Increase disk speed or reduce log volume.

**writer_running = false**: Writer thread died. Check for crashes in logs.

## Implementation Details

### Key Files
- `gwlib/log.c` - Async logging implementation
- `gwlib/log.h` - Public API including `LogQueueStatus`
- `gw/bearerbox.c` - Health/status endpoints

### Thread Safety
- Queue operations use `gwlist_produce()`/`gwlist_timed_consume()` (mutex-protected)
- `dropped_count` uses `__atomic_add_fetch()` for lock-free increment
- RWLock only held during file I/O in writer thread

### Graceful Shutdown
1. `log_writer_running = 0` signals writer to stop
2. `gwlist_remove_producer()` allows queue to drain
3. `gwthread_join()` waits for writer to finish
4. All pending logs flushed before exit

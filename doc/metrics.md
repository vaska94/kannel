# Prometheus Metrics

Kamex exposes metrics at `/metrics` in Prometheus text exposition format for monitoring with Prometheus and Grafana.

## Endpoint

```
GET http://localhost:13000/metrics
```

- No authentication required (standard for metrics endpoints)
- Content-Type: `text/plain; version=0.0.4; charset=utf-8`

## Available Metrics

### Counters (monotonically increasing)

| Metric | Description |
|--------|-------------|
| `kamex_sms_received_total` | Total SMS messages received (MO) |
| `kamex_sms_sent_total` | Total SMS messages sent (MT) |
| `kamex_dlr_received_total` | Total delivery reports received |
| `kamex_dlr_sent_total` | Total delivery reports sent |
| `kamex_log_dropped_total` | Log messages dropped due to queue overflow |

### Gauges (current value)

| Metric | Description |
|--------|-------------|
| `kamex_uptime_seconds` | Gateway uptime in seconds |
| `kamex_sms_queue_incoming` | Messages in incoming queue |
| `kamex_sms_queue_outgoing` | Messages in outgoing queue |
| `kamex_store_messages` | Messages in persistent store (-1 if disabled) |
| `kamex_dlr_queue` | Delivery reports in queue |
| `kamex_smsc_total` | Total SMSCs configured |
| `kamex_smsc_online` | Currently online SMSCs |
| `kamex_sms_received_rate` | Inbound SMS per second |
| `kamex_sms_sent_rate` | Outbound SMS per second |
| `kamex_dlr_received_rate` | Inbound DLR per second |
| `kamex_dlr_sent_rate` | Outbound DLR per second |
| `kamex_log_queue_depth` | Current async log queue depth |
| `kamex_log_queue_max` | Max async log queue capacity |

## Prometheus Configuration

Add to `prometheus.yml`:

```yaml
scrape_configs:
  - job_name: 'kamex'
    static_configs:
      - targets: ['localhost:13000']
    metrics_path: /metrics
    scrape_interval: 15s
```

For multiple gateways:

```yaml
scrape_configs:
  - job_name: 'kamex'
    static_configs:
      - targets:
        - 'gateway1.example.com:13000'
        - 'gateway2.example.com:13000'
    metrics_path: /metrics
```

## Grafana Dashboard

### Useful Queries

**SMS Throughput (messages/sec):**
```promql
rate(kamex_sms_sent_total[5m])
```

**Total SMS sent in last 24h:**
```promql
increase(kamex_sms_sent_total[24h])
```

**SMSC availability percentage:**
```promql
kamex_smsc_online / kamex_smsc_total * 100
```

**Queue depth:**
```promql
kamex_sms_queue_incoming + kamex_sms_queue_outgoing
```

**Log queue utilization:**
```promql
kamex_log_queue_depth / kamex_log_queue_max * 100
```

## Alerting Rules

Example Prometheus alerting rules (`alerts.yml`):

```yaml
groups:
  - name: kamex
    rules:
      - alert: KamexDown
        expr: up{job="kamex"} == 0
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "Kamex gateway is down"

      - alert: KamexNoSMSCOnline
        expr: kamex_smsc_online == 0 and kamex_smsc_total > 0
        for: 2m
        labels:
          severity: critical
        annotations:
          summary: "No SMSCs are online"

      - alert: KamexHighQueueDepth
        expr: kamex_sms_queue_outgoing > 10000
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "SMS queue depth is high ({{ $value }})"

      - alert: KamexLogQueueHigh
        expr: kamex_log_queue_depth / kamex_log_queue_max > 0.8
        for: 2m
        labels:
          severity: warning
        annotations:
          summary: "Log queue is {{ $value | humanizePercentage }} full"

      - alert: KamexLogDropped
        expr: increase(kamex_log_dropped_total[5m]) > 0
        labels:
          severity: warning
        annotations:
          summary: "Log messages are being dropped"
```

## Example Output

```
# HELP kamex_sms_received_total Total SMS messages received
# TYPE kamex_sms_received_total counter
kamex_sms_received_total 12345

# HELP kamex_sms_sent_total Total SMS messages sent
# TYPE kamex_sms_sent_total counter
kamex_sms_sent_total 54321

# HELP kamex_uptime_seconds Gateway uptime in seconds
# TYPE kamex_uptime_seconds gauge
kamex_uptime_seconds 86400

# HELP kamex_smsc_total Total SMSCs configured
# TYPE kamex_smsc_total gauge
kamex_smsc_total 3

# HELP kamex_smsc_online Online SMSCs
# TYPE kamex_smsc_online gauge
kamex_smsc_online 3

# HELP kamex_sms_sent_rate Outbound SMS per second
# TYPE kamex_sms_sent_rate gauge
kamex_sms_sent_rate 150.25
```

## Related

- [Health Check](/health) - Returns health status for load balancers
- [JSON Status](/status.json) - Detailed status in JSON format
- [Logging Architecture](logging.md) - Async logging design

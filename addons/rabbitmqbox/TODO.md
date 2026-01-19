# rabbitmqbox TODO

Remaining features to implement, based on opensmppbox feature parity.

## High Priority

### Multi-part SMS Support
- [x] Concatenation assembly for incoming long messages
- [x] Auto-split outgoing messages that exceed 160 chars (GSM) / 70 chars (UCS-2)
- [x] UDH handling for concatenated message parts
- [x] Reference-based message assembly (by sender + reference)
- [x] Config option: `disable-multipart-catenation`
- [x] Config option: `multipart-timeout`

### Advanced Routing
- [ ] Route by receiver number (smsc-by-receiver)
- [ ] Route by sender ID (smsc-by-sender)
- [ ] Multiple `smsc-route` config groups
- [ ] Shortcode-based routing with semicolon-separated lists

### Load Tracking & Statistics
- [ ] Message counters (sent, received, failed)
- [ ] Per-queue statistics
- [ ] HTTP admin interface with stats endpoint
- [ ] Prometheus metrics export

## Medium Priority

### Charset Conversion
- [ ] UTF-8 to GSM 03.38 conversion
- [ ] UCS-2 encoding support
- [ ] ISO-8859-* encoding support
- [ ] Config option: `alt-charset`
- [ ] Auto-detect best encoding for message

### DLR Enhancements
- [ ] Include original message text (first 12 chars) in DLR
- [ ] Submission/delivery timestamps in DLR
- [ ] Error code mapping to human-readable messages
- [ ] DLR storage integration (for external DLR tracking)

### Message Priority
- [ ] Default priority config option
- [ ] Per-message priority from JSON
- [ ] Priority queue handling

### Admin Interface
- [ ] HTTP admin port with password protection
- [ ] Status endpoint (connections, queues, stats)
- [ ] Reload config via SIGHUP
- [ ] Graceful shutdown with queue drain

## Low Priority

### Exchange Support
- [ ] Declare exchange on connect
- [ ] Publish to exchange with routing keys
- [ ] Topic exchange support for flexible routing
- [ ] Headers exchange for attribute-based routing

### Connection Improvements
- [ ] Multiple RabbitMQ server support (failover)
- [ ] Connection pooling
- [ ] Lazy connection (connect on first message)
- [ ] Exponential backoff for reconnection

### Message Enhancements
- [ ] Message TTL (time-to-live)
- [ ] Dead letter queue configuration
- [ ] Message deduplication
- [ ] Batch publishing for efficiency

### Monitoring
- [ ] Health check endpoint
- [ ] RabbitMQ queue depth monitoring
- [ ] Alert on queue backlog
- [ ] Log rotation support

## Nice to Have

### Protocol Features
- [ ] AMQP 1.0 support (for Azure Service Bus compatibility)
- [ ] Message compression
- [ ] Publisher confirms for guaranteed delivery
- [ ] Consumer prefetch tuning per queue

### Testing
- [ ] Unit tests for JSON parsing
- [ ] Integration tests with RabbitMQ
- [ ] Load testing scripts
- [ ] Mock bearerbox for standalone testing

### Documentation
- [ ] API documentation
- [ ] Deployment guide
- [ ] Performance tuning guide
- [ ] Troubleshooting guide

## Completed

- [x] Basic box skeleton (connect to bearerbox)
- [x] RabbitMQ connection handling
- [x] Consumer thread (queue → bearerbox)
- [x] Publisher thread (bearerbox → queue)
- [x] JSON parsing with gwlib/json.h
- [x] Configuration parsing
- [x] Build system (configure.ac, Makefile.am)
- [x] Example configuration
- [x] SMSC routing (route-to-smsc)
- [x] TLS/SSL support for RabbitMQ
- [x] Allowed senders authentication
- [x] Message persistence (store-file)
- [x] README documentation
- [x] Multi-part SMS support (split outgoing, assemble incoming)
- [x] UDH concatenation handling (8-bit and 16-bit references)
- [x] Multipart timeout and cleanup

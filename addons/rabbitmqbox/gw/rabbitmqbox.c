/*
 * rabbitmqbox.c - RabbitMQ message queue box for Kamex
 *
 * Bridges RabbitMQ message queues with Kamex bearerbox.
 * - Consumes SMS send requests from RabbitMQ queue
 * - Publishes MO (incoming SMS) and DLR to RabbitMQ queues
 *
 * Copyright (c) 2026 Vasil Jamalashvili <shapeless@pm.me>
 * MIT License
 */

#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "gwlib/gwlib.h"
#include "gwlib/json.h"
#include "gw/msg.h"
#include "gw/sms.h"
#include "gw/shared.h"

#include "rmq_connection.h"

/* Program version */
#define RABBITMQBOX_VERSION "1.0.0"

/* Status */
static volatile sig_atomic_t running = 1;
static volatile sig_atomic_t restart_box = 0;

/* Configuration */
static Cfg *cfg;
static Octstr *cfg_filename;

/* Bearerbox connection */
static Octstr *bearerbox_host;
static long bearerbox_port;
static int bearerbox_port_ssl = 0;

/* RabbitMQ connection */
static RMQConnection *rmq;

/* Box identity */
static Octstr *box_id;

/* Default SMSC routing */
static Octstr *route_to_smsc;

/* Allowed senders (authentication) */
static Dict *allowed_senders;
static int require_auth;

/* Local message store for persistence */
static Octstr *store_file;
static List *pending_msgs;
static Mutex *store_lock;

/* Reconnect delay */
#define RECONNECT_DELAY 5.0

/*
 * Signal handlers
 */
static void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM) {
        info(0, "Received signal %d, shutting down...", signum);
        running = 0;
    } else if (signum == SIGHUP) {
        info(0, "Received SIGHUP, will restart...");
        restart_box = 1;
        running = 0;
    }
}

static void setup_signals(void)
{
    struct sigaction act;

    act.sa_handler = signal_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGHUP, &act, NULL);

    /* Ignore SIGPIPE */
    act.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &act, NULL);
}

/*
 * Store pending message to file for persistence
 */
static void store_pending_msg(Msg *msg)
{
    Octstr *packed;
    FILE *fp;

    if (store_file == NULL)
        return;

    packed = msg_pack(msg);
    if (packed == NULL) {
        error(0, "Failed to pack message for storage");
        return;
    }

    mutex_lock(store_lock);

    fp = fopen(octstr_get_cstr(store_file), "ab");
    if (fp != NULL) {
        /* Write length + data */
        long len = octstr_len(packed);
        fwrite(&len, sizeof(len), 1, fp);
        fwrite(octstr_get_cstr(packed), len, 1, fp);
        fclose(fp);
        debug("rabbitmqbox", 0, "Stored pending message to file");
    } else {
        error(0, "Cannot open store file for writing: %s",
              octstr_get_cstr(store_file));
    }

    mutex_unlock(store_lock);
    octstr_destroy(packed);
}

/*
 * Load pending messages from store file
 */
static void load_pending_msgs(void)
{
    FILE *fp;
    long len;
    char *buf;
    Octstr *packed;
    Msg *msg;
    int count = 0;

    if (store_file == NULL)
        return;

    fp = fopen(octstr_get_cstr(store_file), "rb");
    if (fp == NULL)
        return;

    pending_msgs = gwlist_create();

    while (fread(&len, sizeof(len), 1, fp) == 1) {
        if (len <= 0 || len > 1024 * 1024) {
            error(0, "Invalid message length in store file: %ld", len);
            break;
        }

        buf = gw_malloc(len);
        if (fread(buf, len, 1, fp) != 1) {
            gw_free(buf);
            break;
        }

        packed = octstr_create_from_data(buf, len);
        gw_free(buf);

        msg = msg_unpack(packed);
        octstr_destroy(packed);

        if (msg != NULL) {
            gwlist_append(pending_msgs, msg);
            count++;
        }
    }

    fclose(fp);

    if (count > 0) {
        info(0, "Loaded %d pending messages from store", count);
        /* Truncate the file after loading */
        fp = fopen(octstr_get_cstr(store_file), "wb");
        if (fp != NULL)
            fclose(fp);
    }
}

/*
 * Retry sending pending messages
 */
static void retry_pending_msgs(void)
{
    Msg *msg;

    if (pending_msgs == NULL)
        return;

    while ((msg = gwlist_extract_first(pending_msgs)) != NULL) {
        debug("rabbitmqbox", 0, "Retrying pending message");
        if (deliver_to_bearerbox(msg) < 0) {
            /* Still failing, store again */
            store_pending_msg(msg);
            msg_destroy(msg);
        }
    }
}

/*
 * Load allowed senders from file
 * Format: one sender per line, # for comments
 */
static int load_allowed_senders(Octstr *filename)
{
    FILE *fp;
    char buf[1024];
    Octstr *sender;
    int count = 0;

    fp = fopen(octstr_get_cstr(filename), "r");
    if (fp == NULL) {
        error(0, "Cannot open allowed-senders file: %s", octstr_get_cstr(filename));
        return -1;
    }

    allowed_senders = dict_create(32, octstr_destroy_item);

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        /* Skip comments and empty lines */
        if (buf[0] == '#' || buf[0] == '\n' || buf[0] == '\r')
            continue;

        /* Remove trailing newline */
        buf[strcspn(buf, "\r\n")] = 0;

        if (strlen(buf) == 0)
            continue;

        sender = octstr_create(buf);
        dict_put(allowed_senders, sender, octstr_create("1"));
        debug("rabbitmqbox", 0, "Allowed sender: %s", buf);
        count++;
    }

    fclose(fp);
    info(0, "Loaded %d allowed senders from %s", count, octstr_get_cstr(filename));

    return 0;
}

/*
 * Check if sender is allowed
 */
static int is_sender_allowed(Octstr *sender)
{
    if (allowed_senders == NULL)
        return 1;  /* No auth configured, allow all */

    if (dict_get(allowed_senders, sender) != NULL)
        return 1;

    return 0;
}

/*
 * JSON conversion helpers
 */

/* Convert JSON message to Msg struct */
static Msg *json_to_msg(Octstr *json_str)
{
    Msg *msg;
    JSON *json;
    Octstr *from, *to, *text, *smsc_id, *udh, *charset;
    long coding, mclass, validity, deferred, dlr_mask, priority;

    /* Parse JSON */
    json = json_parse(json_str);
    if (json == NULL) {
        error(0, "Failed to parse JSON message");
        return NULL;
    }

    if (!json_is_object(json)) {
        error(0, "JSON message is not an object");
        json_destroy(json);
        return NULL;
    }

    /* Extract required fields */
    from = json_get_string(json, "from");
    to = json_get_string(json, "to");
    text = json_get_string(json, "text");

    /* Validate required fields */
    if (from == NULL || to == NULL || text == NULL) {
        error(0, "Invalid message: missing required fields (from, to, text)");
        octstr_destroy(from);
        octstr_destroy(to);
        octstr_destroy(text);
        json_destroy(json);
        return NULL;
    }

    /* Extract optional fields */
    smsc_id = json_get_string(json, "smsc-id");
    udh = json_get_string(json, "udh");
    charset = json_get_string(json, "charset");

    coding = json_get_integer(json, "coding", 0);
    mclass = json_get_integer(json, "mclass", 0);
    validity = json_get_integer(json, "validity", -1);
    deferred = json_get_integer(json, "deferred", -1);
    dlr_mask = json_get_integer(json, "dlr-mask", 0);
    priority = json_get_integer(json, "priority", 0);

    /* Create message */
    msg = msg_create(sms);
    msg->sms.sms_type = mt_push;

    msg->sms.sender = from;
    msg->sms.receiver = to;
    msg->sms.msgdata = text;

    /* Apply SMSC routing: use message smsc-id or fall back to default */
    if (smsc_id != NULL)
        msg->sms.smsc_id = smsc_id;
    else if (route_to_smsc != NULL)
        msg->sms.smsc_id = octstr_duplicate(route_to_smsc);

    msg->sms.coding = coding;
    msg->sms.mclass = mclass;
    msg->sms.dlr_mask = dlr_mask;
    msg->sms.priority = priority;

    if (udh != NULL) {
        msg->sms.udhdata = octstr_hex_to_binary(udh);
        octstr_destroy(udh);
    }

    if (charset != NULL) {
        msg->sms.charset = charset;
    }

    if (validity >= 0)
        msg->sms.validity = time(NULL) + validity * 60;
    if (deferred >= 0)
        msg->sms.deferred = time(NULL) + deferred * 60;

    /* Generate message ID */
    uuid_generate(msg->sms.id);

    debug("rabbitmqbox", 0, "Parsed message: from=%s to=%s",
          octstr_get_cstr(msg->sms.sender), octstr_get_cstr(msg->sms.receiver));

    json_destroy(json);
    return msg;
}

/* Convert Msg struct to JSON for MO/DLR */
static Octstr *msg_to_json(Msg *msg, const char *type)
{
    Octstr *json;
    Octstr *escaped;
    char uuid_str[UUID_STR_LEN + 1];

    uuid_unparse(msg->sms.id, uuid_str);

    json = octstr_create("{");

    octstr_format_append(json, "\"type\":\"%s\",", type);
    octstr_format_append(json, "\"id\":\"%s\",", uuid_str);

    if (msg->sms.sender) {
        escaped = json_escape_string(msg->sms.sender);
        octstr_format_append(json, "\"from\":%S,", escaped);
        octstr_destroy(escaped);
    }
    if (msg->sms.receiver) {
        escaped = json_escape_string(msg->sms.receiver);
        octstr_format_append(json, "\"to\":%S,", escaped);
        octstr_destroy(escaped);
    }
    if (msg->sms.msgdata) {
        escaped = json_escape_string(msg->sms.msgdata);
        octstr_format_append(json, "\"text\":%S,", escaped);
        octstr_destroy(escaped);
    }
    if (msg->sms.smsc_id) {
        escaped = json_escape_string(msg->sms.smsc_id);
        octstr_format_append(json, "\"smsc-id\":%S,", escaped);
        octstr_destroy(escaped);
    }

    octstr_format_append(json, "\"coding\":%d,", msg->sms.coding);

    if (msg->sms.sms_type == report_mo) {
        octstr_format_append(json, "\"dlr-type\":%d,", msg->sms.dlr_mask);
    }

    /* Add timestamp */
    octstr_format_append(json, "\"timestamp\":%ld", (long)time(NULL));

    octstr_append_cstr(json, "}");

    return json;
}

/*
 * Consumer thread - reads from RabbitMQ, sends to bearerbox
 */
static void consumer_thread(void *arg)
{
    Octstr *json;
    Msg *msg;

    info(0, "Consumer thread started");

    while (running) {
        /* Check RabbitMQ connection */
        if (!rmq_is_connected(rmq)) {
            warning(0, "RabbitMQ disconnected, reconnecting...");
            if (rmq_reconnect(rmq) < 0) {
                gwthread_sleep(RECONNECT_DELAY);
                continue;
            }
        }

        /* Consume message from queue */
        json = rmq_consume(rmq, 1.0);  /* 1 second timeout */
        if (json == NULL) {
            continue;  /* Timeout or error */
        }

        debug("rabbitmqbox", 0, "Consumed message: %s", octstr_get_cstr(json));

        /* Parse JSON to Msg */
        msg = json_to_msg(json);
        if (msg == NULL) {
            /* Invalid message - publish to failed queue */
            Octstr *failed = octstr_format(
                "{\"error\":\"Invalid message format\",\"original\":%S}",
                json);
            rmq_publish(rmq, rmq->queue_failed, failed);
            octstr_destroy(failed);
            octstr_destroy(json);
            continue;
        }

        octstr_destroy(json);

        /* Check sender authentication */
        if (require_auth && !is_sender_allowed(msg->sms.sender)) {
            warning(0, "Sender '%s' not in allowed-senders list, rejecting",
                    octstr_get_cstr(msg->sms.sender));
            Octstr *failed = octstr_format(
                "{\"error\":\"Sender not authorized\",\"from\":\"%S\",\"to\":\"%S\"}",
                msg->sms.sender, msg->sms.receiver);
            rmq_publish(rmq, rmq->queue_failed, failed);
            octstr_destroy(failed);
            msg_destroy(msg);
            continue;
        }

        /* Set box ID */
        if (box_id)
            msg->sms.boxc_id = octstr_duplicate(box_id);

        /* Send to bearerbox */
        if (deliver_to_bearerbox(msg) < 0) {
            error(0, "Failed to deliver message to bearerbox");
            /* Store for retry if persistence enabled */
            if (store_file != NULL) {
                store_pending_msg(msg);
            }
            msg_destroy(msg);
        } else {
            debug("rabbitmqbox", 0, "Message sent to bearerbox");
        }
    }

    info(0, "Consumer thread stopped");
}

/*
 * Reader thread - reads from bearerbox, publishes to RabbitMQ
 */
static void reader_thread(void *arg)
{
    Msg *msg;
    int ret;
    Octstr *json;
    Octstr *queue;

    info(0, "Reader thread started");

    while (running) {
        /* Read from bearerbox */
        ret = read_from_bearerbox(&msg, 1.0);  /* 1 second timeout */

        if (ret == -1) {
            /* Connection error */
            error(0, "Connection to bearerbox lost");
            running = 0;
            break;
        }

        if (ret == 1) {
            /* Timeout */
            continue;
        }

        if (msg == NULL) {
            continue;
        }

        /* Handle different message types */
        if (msg_type(msg) == sms) {
            if (msg->sms.sms_type == mo) {
                /* Mobile Originated - incoming SMS */
                debug("rabbitmqbox", 0, "Received MO from %s",
                      octstr_get_cstr(msg->sms.sender));

                json = msg_to_json(msg, "mo");
                queue = rmq->queue_mo;

            } else if (msg->sms.sms_type == report_mo) {
                /* Delivery Report */
                debug("rabbitmqbox", 0, "Received DLR for %s",
                      octstr_get_cstr(msg->sms.receiver));

                json = msg_to_json(msg, "dlr");
                queue = rmq->queue_dlr;

            } else {
                /* Other SMS type - ignore */
                msg_destroy(msg);
                continue;
            }

            /* Check RabbitMQ connection */
            if (!rmq_is_connected(rmq)) {
                warning(0, "RabbitMQ disconnected, reconnecting...");
                if (rmq_reconnect(rmq) < 0) {
                    octstr_destroy(json);
                    msg_destroy(msg);
                    gwthread_sleep(RECONNECT_DELAY);
                    continue;
                }
            }

            /* Publish to RabbitMQ */
            if (rmq_publish(rmq, queue, json) < 0) {
                error(0, "Failed to publish to RabbitMQ");
            } else {
                debug("rabbitmqbox", 0, "Published to %s: %s",
                      octstr_get_cstr(queue), octstr_get_cstr(json));
            }

            octstr_destroy(json);

        } else if (msg_type(msg) == admin) {
            /* Admin message from bearerbox */
            if (msg->admin.command == cmd_shutdown) {
                info(0, "Bearerbox requested shutdown");
                running = 0;
            } else if (msg->admin.command == cmd_restart) {
                info(0, "Bearerbox requested restart");
                restart_box = 1;
                running = 0;
            }
        }

        msg_destroy(msg);
    }

    info(0, "Reader thread stopped");
}

/*
 * Identify box to bearerbox
 */
static void identify_to_bearerbox(void)
{
    Msg *msg;

    msg = msg_create(admin);
    msg->admin.command = cmd_identify;
    msg->admin.boxc_id = octstr_duplicate(box_id);

    write_to_bearerbox(msg);
}

/*
 * Read configuration
 */
static int read_config(Octstr *filename)
{
    CfgGroup *grp;
    Octstr *log_file;
    long log_level;

    cfg = cfg_create(filename);

    if (cfg_read(cfg) == -1) {
        error(0, "Failed to read config file '%s'", octstr_get_cstr(filename));
        return -1;
    }

    /* Get core group */
    grp = cfg_get_single_group(cfg, octstr_imm("core"));
    if (grp == NULL) {
        error(0, "Missing 'core' group in config");
        return -1;
    }

    /* Log settings */
    log_file = cfg_get(grp, octstr_imm("log-file"));
    if (log_file != NULL) {
        if (cfg_get_integer(&log_level, grp, octstr_imm("log-level")) == -1)
            log_level = 0;
        log_open(octstr_get_cstr(log_file), log_level, GW_NON_EXCL);
        octstr_destroy(log_file);
    }

    /* Get rabbitmqbox group */
    grp = cfg_get_single_group(cfg, octstr_imm("rabbitmqbox"));
    if (grp == NULL) {
        error(0, "Missing 'rabbitmqbox' group in config");
        return -1;
    }

    /* Bearerbox connection */
    bearerbox_host = cfg_get(grp, octstr_imm("bearerbox-host"));
    if (bearerbox_host == NULL)
        bearerbox_host = octstr_create("localhost");

    if (cfg_get_integer(&bearerbox_port, grp, octstr_imm("bearerbox-port")) == -1)
        bearerbox_port = 13001;

    cfg_get_bool(&bearerbox_port_ssl, grp, octstr_imm("bearerbox-port-ssl"));

    /* Box ID */
    box_id = cfg_get(grp, octstr_imm("box-id"));
    if (box_id == NULL)
        box_id = octstr_create("rabbitmqbox");

    /* SMSC routing */
    route_to_smsc = cfg_get(grp, octstr_imm("route-to-smsc"));
    if (route_to_smsc != NULL) {
        info(0, "Default SMSC routing: %s", octstr_get_cstr(route_to_smsc));
    }

    /* Allowed senders (authentication) */
    {
        Octstr *senders_file = cfg_get(grp, octstr_imm("allowed-senders"));
        if (senders_file != NULL) {
            if (load_allowed_senders(senders_file) < 0) {
                octstr_destroy(senders_file);
                return -1;
            }
            require_auth = 1;
            octstr_destroy(senders_file);
        }
    }

    /* Message store for persistence */
    store_file = cfg_get(grp, octstr_imm("store-file"));
    if (store_file != NULL) {
        info(0, "Message store enabled: %s", octstr_get_cstr(store_file));
        store_lock = mutex_create();
        load_pending_msgs();
    }

    /* Create RabbitMQ connection from config */
    rmq = rmq_connection_create(grp);
    if (rmq == NULL) {
        error(0, "Failed to create RabbitMQ connection config");
        return -1;
    }

    return 0;
}

/*
 * Print usage
 */
static void usage(const char *progname)
{
    printf("Usage: %s [options] config-file\n", progname);
    printf("\n");
    printf("Options:\n");
    printf("  -v, --version    Print version and exit\n");
    printf("  -h, --help       Print this help and exit\n");
    printf("  -d, --debug      Enable debug logging\n");
    printf("\n");
}

/*
 * Main
 */
int main(int argc, char **argv)
{
    int opt;
    int debug_lvl = 0;
    long consumer_id, reader_id;

    /* Parse command line */
    while ((opt = getopt(argc, argv, "vhd")) != -1) {
        switch (opt) {
        case 'v':
            printf("rabbitmqbox version %s\n", RABBITMQBOX_VERSION);
            return 0;
        case 'h':
            usage(argv[0]);
            return 0;
        case 'd':
            debug_lvl = 1;
            break;
        default:
            usage(argv[0]);
            return 1;
        }
    }

    if (optind >= argc) {
        error(0, "Missing config file argument");
        usage(argv[0]);
        return 1;
    }

    cfg_filename = octstr_create(argv[optind]);

    /* Initialize gwlib */
    gwlib_init();

    if (debug_lvl)
        log_set_output_level(GW_DEBUG);

    info(0, "rabbitmqbox version %s starting", RABBITMQBOX_VERSION);

    /* Setup signal handlers */
    setup_signals();

restart:
    running = 1;
    restart_box = 0;

    /* Read configuration */
    if (read_config(cfg_filename) < 0) {
        error(0, "Failed to read configuration");
        goto shutdown;
    }

    /* Connect to RabbitMQ */
    if (rmq_connect(rmq) < 0) {
        error(0, "Failed to connect to RabbitMQ");
        goto shutdown;
    }

    /* Connect to bearerbox */
    info(0, "Connecting to bearerbox at %s:%ld...",
         octstr_get_cstr(bearerbox_host), bearerbox_port);

    connect_to_bearerbox(bearerbox_host, bearerbox_port, bearerbox_port_ssl, NULL);

    /* Identify ourselves */
    identify_to_bearerbox();

    info(0, "Connected to bearerbox");

    /* Retry any pending messages from store */
    retry_pending_msgs();

    /* Start threads */
    consumer_id = gwthread_create(consumer_thread, NULL);
    reader_id = gwthread_create(reader_thread, NULL);

    /* Wait for threads to finish */
    gwthread_join(consumer_id);
    gwthread_join(reader_id);

    /* Cleanup */
    close_connection_to_bearerbox();
    rmq_disconnect(rmq);

    if (restart_box) {
        info(0, "Restarting...");
        rmq_connection_destroy(rmq);
        rmq = NULL;
        cfg_destroy(cfg);
        cfg = NULL;
        goto restart;
    }

shutdown:
    info(0, "Shutting down...");

    if (rmq) {
        rmq_connection_destroy(rmq);
    }

    octstr_destroy(bearerbox_host);
    octstr_destroy(box_id);
    octstr_destroy(cfg_filename);

    if (cfg)
        cfg_destroy(cfg);

    gwlib_shutdown();

    info(0, "rabbitmqbox stopped");

    return 0;
}

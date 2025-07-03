/* ==================================================================== 
 * The Kannel Software License, Version 1.0 
 * 
 * Copyright (c) 2001-2018 Kannel Group  
 * Copyright (c) 1998-2001 WapIT Ltd.   
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in 
 *    the documentation and/or other materials provided with the 
 *    distribution. 
 * 
 * 3. The end-user documentation included with the redistribution, 
 *    if any, must include the following acknowledgment: 
 *       "This product includes software developed by the 
 *        Kannel Group (http://www.kannel.org/)." 
 *    Alternately, this acknowledgment may appear in the software itself, 
 *    if and wherever such third-party acknowledgments normally appear. 
 * 
 * 4. The names "Kannel" and "Kannel Group" must not be used to 
 *    endorse or promote products derived from this software without 
 *    prior written permission. For written permission, please  
 *    contact org@kannel.org. 
 * 
 * 5. Products derived from this software may not be called "Kannel", 
 *    nor may "Kannel" appear in their name, without prior written 
 *    permission of the Kannel Group. 
 * 
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED.  IN NO EVENT SHALL THE KANNEL GROUP OR ITS CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,  
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR  
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,  
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE  
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 * ==================================================================== 
 * 
 * This software consists of voluntary contributions made by many 
 * individuals on behalf of the Kannel Group.  For more information on  
 * the Kannel Group, please see <http://www.kannel.org/>. 
 * 
 * Portions of this software are based upon software originally written at  
 * WapIT Ltd., Helsinki, Finland for the Kannel project.  
 */ 

/*
 * smsc_http.c - interface to various HTTP based content/SMS gateways
 *
 * HTTP based "SMSC Connection" is meant for gateway connections,
 * and has following features:
 *
 * o Kannel listens to certain (HTTP server) port for MO SMS messages.
 *   The exact format of these HTTP calls are defined by type of HTTP based
 *   connection. Kannel replies to these messages as ACK, but does not
 *   support immediate reply. Thus, if Kannel is linked to another Kannel,
 *   only 'max-messages = 0' services are practically supported - any
 *   replies must be done with SMS PUSH (sendsms)
 *
 * o For MT messages, Kannel does HTTP GET or POST to given address, in format
 *   defined by type of HTTP based protocol
 *
 * The 'type' of requests and replies are defined by 'system-type' variable.
 * The only type of HTTP requests currently supported are basic Kannel.
 * If new support is added, smsc_http_create is modified accordingly and new
 * functions added.
 *
 *
 * KANNEL->KANNEL linking: (UDH not supported in MO messages)
 *
 *****
 * FOR CLIENT/END-POINT KANNEL:
 *
 *  group = smsc
 *  smsc = http
 *  system-type = kannel
 *  port = NNN
 *  smsc-username = XXX
 *  smsc-password = YYY
 *  send-url = "server.host:PORT"
 *
 *****
 * FOR SERVER/RELAY KANNEL:
 *
 *  group = smsbox
 *  sendsms-port = PORT
 *  ...
 *
 *  group = sms-service
 *  keyword = ...
 *  url = "client.host:NNN/sms?user=XXX&pass=YYY&from=%p&to=%P&text=%a"
 *  max-messages = 0
 *
 *  group = send-sms
 *  username = XXX
 *  password = YYY
 *
 * Kalle Marjola for Project Kannel 2001
 * Stipe Tolj <st@tolj.org>
 * Alexander Malysh <amalysh at kannel.org>
 * Tobias Weber <weber@wapme.de>
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <limits.h>

#include "gwlib/gwlib.h"
#include "smscconn.h"
#include "smscconn_p.h"
#include "bb_smscconn_cb.h"
#include "msg.h"
#include "sms.h"
#include "dlr.h"
#include "urltrans.h"
#include "meta_data.h"

#include "smsc_http_p.h"

#define DEFAULT_CHARSET         "UTF-8"
#define DEFAULT_UCS2_CHARSET    "UTF-16BE"

static void conndata_destroy(ConnData *conndata)
{
    if (conndata == NULL)
        return;
    if (conndata->http_ref)
        http_caller_destroy(conndata->http_ref);
    octstr_destroy(conndata->allow_ip);
    octstr_destroy(conndata->send_url);
    octstr_destroy(conndata->dlr_url);
    octstr_destroy(conndata->username);
    octstr_destroy(conndata->password);
    octstr_destroy(conndata->system_id);
    octstr_destroy(conndata->alt_charset);
    counter_destroy(conndata->open_sends);
    gwlist_destroy(conndata->msg_to_send, NULL);
    if (conndata->max_pending_sends)
        semaphore_destroy(conndata->max_pending_sends);

    gw_free(conndata);
}


/*
 * Thread to listen to HTTP requests from SMSC entity
 */
static void httpsmsc_receiver(void *arg)
{
    SMSCConn *conn = arg;
    ConnData *conndata = conn->data;
    HTTPClient *client;
    Octstr *ip, *url, *body;
    List *headers, *cgivars;

    /* Make sure we log into our own log-file if defined */
    log_thread_to(conn->log_idx);

    while (conndata->shutdown == 0) {
        /* reset */
        ip = url = body = NULL;
        headers = cgivars = NULL;

        /* XXX if conn->is_stopped, do not receive new messages.. */

        client = http_accept_request(conndata->port, &ip, &url,
                                     &headers, &body, &cgivars);
        if (client == NULL)
            break;

        if (cgivars != NULL) {
        	octstr_append_char(url, '?');
        	http_cgivar_dump_into(cgivars, url);
        }

        debug("smsc.http", 0, "HTTP[%s]: Got request `%s'",
              octstr_get_cstr(conn->id), octstr_get_cstr(url));

        if (connect_denied(conndata->allow_ip, ip)) {
            info(0, "HTTP[%s]: Connection `%s' tried from denied "
                    "host %s, ignored", octstr_get_cstr(conn->id),
                    octstr_get_cstr(url), octstr_get_cstr(ip));
            http_close_client(client);
        } else
            conndata->callbacks->receive_sms(conn, client, headers, body, cgivars);

        debug("smsc.http", 0, "HTTP[%s]: Destroying client information",
              octstr_get_cstr(conn->id));
        octstr_destroy(url);
        octstr_destroy(ip);
        octstr_destroy(body);
        http_destroy_headers(headers);
        http_destroy_cgiargs(cgivars);
    }
    debug("smsc.http", 0, "HTTP[%s]: httpsmsc_receiver dying",
          octstr_get_cstr(conn->id));

    conndata->shutdown = 1;
    http_close_port(conndata->port);

    /* unblock http_receive_result() if there are no open sends */
    if (counter_value(conndata->open_sends) == 0)
        http_caller_signal_shutdown(conndata->http_ref);

    if (conndata->sender_thread != -1) {
        gwthread_wakeup(conndata->sender_thread);
        gwthread_join(conndata->sender_thread);
    }
    if (conndata->send_cb_thread != -1) {
        gwthread_wakeup(conndata->send_cb_thread);
        gwthread_join(conndata->send_cb_thread);
    }

    mutex_lock(conn->flow_mutex);
    conn->status = SMSCCONN_DEAD;
    mutex_unlock(conn->flow_mutex);

    if (conndata->callbacks != NULL && conndata->callbacks->destroy != NULL)
        conndata->callbacks->destroy(conn);
    conn->data = NULL;
    conndata_destroy(conndata);
    bb_smscconn_killed();
}


/*
 * Thread to send queued messages
 */
static void httpsmsc_sender(void *arg)
{
    SMSCConn *conn = arg;
    ConnData *conndata = conn->data;
    Msg *msg;
    double delay = 0;

    /* Make sure we log into our own log-file if defined */
    log_thread_to(conn->log_idx);

    if (conn->throughput) {
        delay = 1.0 / conn->throughput;
    }

    while (conndata->shutdown == 0) {
        /* check if we can send ; otherwise block on semaphore */
        if (conndata->max_pending_sends)
            semaphore_down(conndata->max_pending_sends);

        if (conndata->shutdown) {
            if (conndata->max_pending_sends)
                semaphore_up(conndata->max_pending_sends);
            break;
        }

        msg = gwlist_consume(conndata->msg_to_send);
        if (msg == NULL)
            break;

        /* obey throughput speed limit, if any */
        if (conn->throughput > 0) {
            gwthread_sleep(delay);
        }
        counter_increase(conndata->open_sends);
        if (conndata->callbacks->send_sms(conn, msg) == -1) {
            counter_decrease(conndata->open_sends);
            if (conndata->max_pending_sends)
                semaphore_up(conndata->max_pending_sends);
        }
    }

    /* put outstanding sends back into global queue */
    while((msg = gwlist_extract_first(conndata->msg_to_send)))
        bb_smscconn_send_failed(conn, msg, SMSCCONN_FAILED_SHUTDOWN, NULL);

    /* if there no receiver shutdown */
    if (conndata->port <= 0) {
        /* unblock http_receive_result() if there are no open sends */
        if (counter_value(conndata->open_sends) == 0)
            http_caller_signal_shutdown(conndata->http_ref);

        if (conndata->send_cb_thread != -1) {
            gwthread_wakeup(conndata->send_cb_thread);
            gwthread_join(conndata->send_cb_thread);
        }
        mutex_lock(conn->flow_mutex);
        conn->status = SMSCCONN_DEAD;
        mutex_unlock(conn->flow_mutex);

        if (conndata->callbacks != NULL && conndata->callbacks->destroy != NULL)
            conndata->callbacks->destroy(conn);
        conn->data = NULL;
        conndata_destroy(conndata);
        bb_smscconn_killed();
    }
}

/*
 * Thread to handle finished sendings
 */
static void httpsmsc_send_cb(void *arg)
{
    SMSCConn *conn = arg;
    ConnData *conndata = conn->data;
    Msg *msg;
    int status;
    List *headers;
    Octstr *final_url, *body;

    /* Make sure we log into our own log-file if defined */
    log_thread_to(conn->log_idx);

    while(conndata->shutdown == 0 || counter_value(conndata->open_sends)) {

        msg = http_receive_result(conndata->http_ref, &status,
                                  &final_url, &headers, &body);

        if (msg == NULL)
            break;  /* they told us to die, by unlocking */

        counter_decrease(conndata->open_sends);
        if (conndata->max_pending_sends)
            semaphore_up(conndata->max_pending_sends);

        /* Handle various states here. */

        /* request failed and we are not in shutdown mode */
        if (status == -1 && conndata->shutdown == 0) {
            error(0, "HTTP[%s]: Couldn't connect to SMS center."
                     "(retrying in %ld seconds) %ld.",
                     octstr_get_cstr(conn->id), conn->reconnect_delay, counter_value(conndata->open_sends));
            mutex_lock(conn->flow_mutex);
            conn->status = SMSCCONN_RECONNECTING;
            mutex_unlock(conn->flow_mutex);
            /* XXX how should we know whether it's temp. error ?? */
            bb_smscconn_send_failed(conn, msg, SMSCCONN_FAILED_TEMPORARILY, NULL);
            /*
             * Just sleep reconnect delay and set conn to ACTIVE again;
             * otherwise if no pending request are here, we leave conn in
             * RECONNECTING state for ever and no routing (trials) take place.
             */
            if (counter_value(conndata->open_sends) == 0) {
                gwthread_sleep(conn->reconnect_delay);
                /* and now enable routing again */
                mutex_lock(conn->flow_mutex);
                conn->status = SMSCCONN_ACTIVE;
                time(&conn->connect_time);
                mutex_unlock(conn->flow_mutex);
                /* tell bearerbox core that we are connected again */
                bb_smscconn_connected(conn);
            }
            continue;
        }
        /* request failed and we *are* in shutdown mode, drop the message */
        else if (status == -1 && conndata->shutdown == 1) {
            bb_smscconn_send_failed(conn, msg, SMSCCONN_FAILED_SHUTDOWN, NULL);
        }
        /* request succeeded */
        else {
            /* we received a response, so this link is considered online again */
            if (conn->status != SMSCCONN_ACTIVE) {
                mutex_lock(conn->flow_mutex);
                conn->status = SMSCCONN_ACTIVE;
                time(&conn->connect_time);
                mutex_unlock(conn->flow_mutex);
                /* tell bearerbox core that we are connected again */
                bb_smscconn_connected(conn);
            }
            conndata->callbacks->parse_reply(conn, msg, status, headers, body);
        }

        http_destroy_headers(headers);
        octstr_destroy(final_url);
        octstr_destroy(body);
    }
    debug("smsc.http", 0, "HTTP[%s]: httpsmsc_send_cb dying",
          octstr_get_cstr(conn->id));
    conndata->shutdown = 1;

    if (counter_value(conndata->open_sends)) {
        warning(0, "HTTP[%s]: Shutdown while <%ld> requests are pending.",
                octstr_get_cstr(conn->id), counter_value(conndata->open_sends));
    }
}


/*-----------------------------------------------------------------
 * functions to implement various smscconn operations
 */

static int httpsmsc_send(SMSCConn *conn, Msg *msg)
{
    ConnData *conndata = conn->data;
    Msg *sms;


    /* don't crash if no send_sms handle defined */
    if (!conndata || !conndata->callbacks->send_sms)
        return -1;

    sms = msg_duplicate(msg);
    /* convert character encoding if required */
    if (conndata->alt_charset) {
        /*
         * Converted now to the target character set based on the
         * one we got in the msg, which is either UTF-8 (our normal
         * inter-box encoding), but may also be UCS-2, so beware.
         * In addition, IF we convert to an "extra" encoding here
         * we also revert the .coding vaue to DC_UNDEF, in order
         * that all API specific code doesn't indicate an encoding
         * which is no longer inside the payload here.
         */
        if (sms->sms.coding == DC_7BIT) {
            if (charset_convert(sms->sms.msgdata, DEFAULT_CHARSET,
                    octstr_get_cstr(conndata->alt_charset)) == 0) {
                sms->sms.coding = DC_UNDEF;
            } else {
                error(0, "Failed to convert msgdata from charset <%s> to <%s>, will send as is.",
                         DEFAULT_CHARSET, octstr_get_cstr(conndata->alt_charset));
            }
        }
        else if (sms->sms.coding == DC_UCS2) {
            if (charset_convert(sms->sms.msgdata, DEFAULT_UCS2_CHARSET,
                    octstr_get_cstr(conndata->alt_charset)) == 0) {
               sms->sms.coding = DC_UNDEF;
            } else {
                error(0, "Failed to convert msgdata from charset <%s> to <%s>, will send as is.",
                        DEFAULT_UCS2_CHARSET, octstr_get_cstr(conndata->alt_charset));
            }
        }
    }

    gwlist_produce(conndata->msg_to_send, sms);

    return 0;
}


static long httpsmsc_queued(SMSCConn *conn)
{
    ConnData *conndata = conn->data;

    conn->load = (conndata ? (conn->status != SMSCCONN_DEAD ?
            gwlist_len(conndata->msg_to_send) : 0) : 0);

    return conn->load;
}


static int httpsmsc_shutdown(SMSCConn *conn, int finish_sending)
{
    ConnData *conndata = conn->data;

    if (conndata == NULL)
        return 0;

    debug("httpsmsc_shutdown", 0, "HTTP[%s]: Shutting down",
          octstr_get_cstr(conn->id));

    mutex_lock(conn->flow_mutex);
    conn->why_killed = SMSCCONN_KILLED_SHUTDOWN;

    conndata->shutdown = 1;

    if (conndata->port > 0)
        http_close_port(conndata->port);
    gwlist_remove_producer(conndata->msg_to_send);
    if (conndata->receive_thread != -1)
        gwthread_wakeup(conndata->receive_thread);
    if (conndata->sender_thread != -1)
        gwthread_wakeup(conndata->sender_thread);
    mutex_unlock(conn->flow_mutex);

    return 0;
}


int smsc_http_create(SMSCConn *conn, CfgGroup *cfg)
{
    ConnData *conndata = NULL;
    Octstr *type = NULL, *callbackname = NULL;
    int ssl = 0;   /* indicate if SSL-enabled server should be used */
    long max_ps;

    if ((type = cfg_get(cfg, octstr_imm("system-type"))) == NULL) {
        error(0, "HTTP[%s]: 'system-type' missing in smsc 'http' record.",
              octstr_get_cstr(conn->id));
        octstr_destroy(type);
        return -1;
    }

    conndata = gw_malloc(sizeof(ConnData));
    /* reset conndata */
    memset(conndata, 0, sizeof(ConnData));

    conn->data = conndata;
    conndata->http_ref = NULL;
    conndata->data = NULL;

    if (cfg_get_integer(&conndata->port, cfg, octstr_imm("port")) == -1) {
        warning(0, "HTTP[%s]: 'port' not set in smsc 'http' group.",
              octstr_get_cstr(conn->id));
        conndata->port = -1;
    }

    conndata->allow_ip = cfg_get(cfg, octstr_imm("connect-allow-ip"));
    conndata->send_url = cfg_get(cfg, octstr_imm("send-url"));
    conndata->username = cfg_get(cfg, octstr_imm("smsc-username"));
    conndata->password = cfg_get(cfg, octstr_imm("smsc-password"));
    conndata->system_id = cfg_get(cfg, octstr_imm("system-id"));
    cfg_get_bool(&conndata->no_sender, cfg, octstr_imm("no-sender"));
    cfg_get_bool(&conndata->no_coding, cfg, octstr_imm("no-coding"));
    cfg_get_bool(&conndata->no_sep, cfg, octstr_imm("no-sep"));
    cfg_get_bool(&conndata->mobile_originated, cfg, octstr_imm("mobile-originated"));
    cfg_get_bool(&ssl, cfg, octstr_imm("use-ssl"));
    conndata->dlr_url = cfg_get(cfg, octstr_imm("dlr-url"));
    conndata->alt_charset = cfg_get(cfg, octstr_imm("alt-charset"));
    cfg_get_bool(&conndata->use_post, cfg, octstr_imm("use-post"));

    if (cfg_get_integer(&max_ps, cfg, octstr_imm("max-pending-submits")) == -1 || max_ps < 1)
        max_ps = 10;
    conndata->max_pending_sends = semaphore_create(max_ps);

    if (conndata->port <= 0 && conndata->send_url == NULL) {
        error(0, "HTTP[%s]: Sender and receiver disabled. Dummy SMSC not allowed.",
              octstr_get_cstr(conn->id));
        goto error;
    }
    if (conndata->send_url == NULL) {
        error(0, "HTTP[%s]: Sending not allowed. No 'send-url' specified.",
              octstr_get_cstr(conn->id));
        goto error;
    }

    /* callback struct is always in the format: smsc_http_XXX_callback where XXX is our type */
    callbackname = octstr_format("smsc_http_%S_callback", type);
    /* try to find the struct */
    if (gw_dlopen_get_symbol(NULL, octstr_get_cstr(callbackname), (void**)&conndata->callbacks) == -1 || conndata->callbacks == NULL) {
        error(0, "HTTP[%s]: system-type '%s' unknown smsc 'http' record.",
              octstr_get_cstr(conn->id), octstr_get_cstr(type));
        goto error;
    }

    if (conndata->callbacks != NULL && conndata->callbacks->init != NULL && conndata->callbacks->init(conn, cfg)) {
        error(0, "HTTP[%s]: submodule '%s' init failed.", octstr_get_cstr(conn->id), octstr_get_cstr(type));
        goto error;
    }

    conndata->open_sends = counter_create();
    conndata->msg_to_send = gwlist_create();
    gwlist_add_producer(conndata->msg_to_send);
    conndata->http_ref = http_caller_create();

    conn->name = octstr_format("HTTP%s:%S:%d", (ssl?"S":""), type, conndata->port);

    if (conndata->send_url != NULL) {
        conn->status = SMSCCONN_ACTIVE;
    } else {
        conn->status = SMSCCONN_ACTIVE_RECV;
    }

    conn->connect_time = time(NULL);

    conn->shutdown = httpsmsc_shutdown;
    conn->queued = httpsmsc_queued;
    conn->send_msg = httpsmsc_send;

    conndata->shutdown = 0;

    /* start receiver thread */
    if (conndata->port > 0) {
        if (http_open_port(conndata->port, ssl) == -1)
            goto error;
        if ((conndata->receive_thread = gwthread_create(httpsmsc_receiver, conn)) == -1)
            goto error;
    } else
        conndata->receive_thread = -1;

    /* start sender threads */
    if (conndata->send_url) {
        if ((conndata->send_cb_thread =
	        gwthread_create(httpsmsc_send_cb, conn)) == -1)
	    goto error;
        if ((conndata->sender_thread =
                gwthread_create(httpsmsc_sender, conn)) == -1)
	    goto error;
    }
    else {
        conndata->send_cb_thread = conndata->sender_thread = -1;
    }

    info(0, "HTTP[%s]: Initiated and ready", octstr_get_cstr(conn->id));

    octstr_destroy(callbackname);
    octstr_destroy(type);
    return 0;

error:
    error(0, "HTTP[%s]: Failed to create HTTP SMSC connection",
          octstr_get_cstr(conn->id));

    if (conndata->callbacks != NULL && conndata->callbacks->destroy != NULL)
        conndata->callbacks->destroy(conn);
    conn->data = NULL;
    conndata_destroy(conndata);
    conn->why_killed = SMSCCONN_KILLED_CANNOT_CONNECT;
    conn->status = SMSCCONN_DEAD;
    octstr_destroy(callbackname);
    octstr_destroy(type);
    return -1;
}


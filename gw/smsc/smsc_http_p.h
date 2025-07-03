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

/*----------------------------------------------------------------
 * SMSC-type specific functions
 *
 * 3 functions are needed for each:
 *
 *   1) send SMS
 *   2) parse send SMS result
 *   3) receive SMS (and send reply)
 *
 *   These functions do not return anything and do not destroy
 *   arguments. They must handle everything that happens therein
 *   and must call appropriate bb_smscconn functions
 */


/* callback functions set by HTTP-SMSC type */
struct smsc_http_fn_callbacks {
    int (*init) (SMSCConn *conn, CfgGroup *cfg);
    void (*destroy) (SMSCConn *conn);
    int (*send_sms) (SMSCConn *conn, Msg *msg);
    void (*parse_reply) (SMSCConn *conn, Msg *msg, int status, List *headers, Octstr *body);
    void (*receive_sms) (SMSCConn *conn, HTTPClient *client, List *headers, Octstr *body, List *cgivars);
};

/*
 * internal smsc_http data
 * Please Note: you can use values from this struct BUT never change/delete values.
 */
typedef struct conndata {
    HTTPCaller *http_ref;
    long receive_thread;
    long send_cb_thread;
    long sender_thread;
    volatile int shutdown;
    long port;   /* port for receiving SMS'es */
    Octstr *allow_ip;
    Octstr *send_url;
    Octstr *dlr_url;
    Counter *open_sends;
    Semaphore *max_pending_sends;
    Octstr *username;   /* if needed */
    Octstr *password;   /* as said */
    int no_sender;      /* ditto */
    int no_coding;      /* this, too */
    int no_sep;         /* not to mention this */
    int use_post;       /* indicate we will use HTTP POST instead of GET */
    Octstr *alt_charset;/* alternative charset use */
    List *msg_to_send;  /* our send queue */

    /* Clickatell specific options */
    Octstr *system_id;     /* API id */
    int mobile_originated; /* 'mo' parameter for a two-way service (0 or 1) */

    /* callback functions set by HTTP-SMSC type */
    struct smsc_http_fn_callbacks *callbacks;

    /* submodule specific data */
    void *data;
} ConnData;

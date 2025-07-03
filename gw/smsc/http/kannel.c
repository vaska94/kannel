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

#include "gwlib/gwlib.h"
#include "smscconn.h"
#include "smscconn_p.h"
#include "bb_smscconn_cb.h"
#include "msg.h"
#include "sms.h"
#include "dlr.h"
#include "urltrans.h"
#include "meta_data.h"

#include "../smsc_http_p.h"

/*----------------------------------------------------------------
 * Kannel
 *
 * This type allows concatenation of Kannel instances, ie:
 *
 *  <smsc>--<bearerbox2><smsbox2>--HTTP--<smsc_http><bearerbox1><smsbox1>
 *
 * Where MT messages are injected via the sendsms HTTP interface at smsbox1,
 * forwarded to bearerbo1 and routed via the SMSC HTTP type kannel to
 * sendsms HTTP interface of smsbox2 and further on.
 *
 * This allows chaining of Kannel instances for MO and MT traffic.
 *
 * DLR handling:
 * For DLR handling we have the usual effect that the "last" smsbox instance
 * of the chain is signaling the DLR-URL, since the last instance receives
 * the DLR from the upstream SMSC and the associated smsbox triggers the
 * DLR-URL.
 * For some considerations this is not what we want. If we want to transport
 * the orginal DLR-URL to the "first" smsbox instance of the calling chain
 * then we need to define a 'dlr-url' config directive in the smsc group.
 * This value defines the inbound MO/DLR port of our own smsc group and
 * maps arround the orginal DLR-URL. So the next smsbox does not signal the
 * orginal DLR-URL, but our own smsc group instance with the DLR, and we can
 * forward on to smsbox and possibly further on the chain to the first
 * instance.
 *
 * Example: (consider the 2 chain architecture from above)
 * A MT is put to smsbox1 with dlr-url=http://foobar/aaa as DLR-URL. The MT
 * is forwarded to bearerbox.
 * If no 'dlr-url' is given in the smsc HTTP for the next smsbox2, then we
 * simply pass the same value to smsbox2. Resulting that smsbox2 will call
 * the DLR-URL when we receive a DLR from the upstream SMSC connection of
 * bearerbox2.
 * If 'dlr-url = http://localhost:15015/' is given in the smsc HTTP for the
 * next smsbox2, then we map the orginal DLR-URL into this value, resulting
 * in a dlr-url=http://lcoalhost:15015/&dlr-url=http://foobar/aaa call to
 * smsbox2. So smsbox2 is not signaling http://foobar/aaa directly, but our
 * own bearerbox1's smsc HTTP port for MO/DLR receive.
 */

enum { HEX_NOT_UPPERCASE = 0 };


static int kannel_send_sms(SMSCConn *conn, Msg *sms)
{
    ConnData *conndata = conn->data;
    Octstr *url;
    List *headers;

    if (!conndata->no_sep) {
        url = octstr_format("%S?"
                "username=%E&password=%E&to=%E&text=%E",
                 conndata->send_url,
                 conndata->username, conndata->password,
                 sms->sms.receiver, sms->sms.msgdata);
    } else {
        Octstr *msgdata = octstr_duplicate(sms->sms.msgdata);

        octstr_binary_to_hex(msgdata, HEX_NOT_UPPERCASE);
        url = octstr_format("%S?"
                "username=%E&password=%E&to=%E&text=%S",
                 conndata->send_url,
                 conndata->username, conndata->password,
                 sms->sms.receiver, msgdata);
        octstr_destroy(msgdata);
    }

    if (octstr_len(sms->sms.udhdata)) {
        if (!conndata->no_sep) {
            octstr_format_append(url, "&udh=%E", sms->sms.udhdata);
        } else {
            Octstr *udhdata = octstr_duplicate(sms->sms.udhdata);

            octstr_binary_to_hex(udhdata, HEX_NOT_UPPERCASE);
            octstr_format_append(url, "&udh=%S", udhdata);
            octstr_destroy(udhdata);
        }
    }

    if (!conndata->no_sender)
        octstr_format_append(url, "&from=%E", sms->sms.sender);
    if (sms->sms.mclass != MC_UNDEF)
        octstr_format_append(url, "&mclass=%d", sms->sms.mclass);
    if (!conndata->no_coding && sms->sms.coding != DC_UNDEF)
        octstr_format_append(url, "&coding=%d", sms->sms.coding);

    /* Obey that smsbox's sendsms HTTP interface is still expecting
     * WINDOWS-1252 as default charset, while all other internal parts
     * use UTF-8 as internal encoding. This means, when we pass a SMS
     * into a next Kannel instance, we need to let the smsbox know which
     * charset we have in use.
     * XXX TODO: change smsbox interface to use UTF-8 as default
     * in next major release. */
    if (sms->sms.coding == DC_7BIT)
        octstr_append_cstr(url, "&charset=UTF-8");
    else if (sms->sms.coding == DC_UCS2)
        octstr_append_cstr(url, "&charset=UTF-16BE");

    if (sms->sms.mwi != MWI_UNDEF)
        octstr_format_append(url, "&mwi=%d", sms->sms.mwi);
    if (sms->sms.account) /* prepend account with local username */
        octstr_format_append(url, "&account=%E:%E", sms->sms.service, sms->sms.account);
    if (sms->sms.binfo) /* prepend billing info */
        octstr_format_append(url, "&binfo=%S", sms->sms.binfo);
    if (sms->sms.smsc_id) /* proxy the smsc-id to the next instance */
        octstr_format_append(url, "&smsc=%S", sms->sms.smsc_id);
    if (conndata->dlr_url) {
        char id[UUID_STR_LEN + 1];
        Octstr *mid;

        /* create Octstr from UUID */
        uuid_unparse(sms->sms.id, id);
        mid = octstr_create(id);

        octstr_format_append(url, "&dlr-url=%E", conndata->dlr_url);

        /* encapsulate the original DLR-URL, escape code for DLR mask
         * and message id */
        octstr_format_append(url, "%E%E%E%E%E",
            octstr_imm("&dlr-url="), sms->sms.dlr_url != NULL ? sms->sms.dlr_url : octstr_imm(""),
            octstr_imm("&dlr-mask=%d"),
            octstr_imm("&dlr-mid="), mid);

        octstr_destroy(mid);
    } else if (sms->sms.dlr_url != NULL)
        octstr_format_append(url, "&dlr-url=%E", sms->sms.dlr_url);
    if (sms->sms.dlr_mask != DLR_UNDEFINED && sms->sms.dlr_mask != DLR_NOTHING)
        octstr_format_append(url, "&dlr-mask=%d", sms->sms.dlr_mask);

    if (sms->sms.validity != SMS_PARAM_UNDEFINED)
        octstr_format_append(url, "&validity=%ld", (sms->sms.validity - time(NULL)) / 60);
    if (sms->sms.deferred != SMS_PARAM_UNDEFINED)
        octstr_format_append(url, "&deferred=%ld", (sms->sms.deferred - time(NULL)) / 60);

    headers = gwlist_create();
    debug("smsc.http.kannel", 0, "HTTP[%s]: Start request",
          octstr_get_cstr(conn->id));
    http_start_request(conndata->http_ref, HTTP_METHOD_GET, url, headers,
                       NULL, 0, sms, NULL);

    octstr_destroy(url);
    http_destroy_headers(headers);

    return 0;
}

static void kannel_parse_reply(SMSCConn *conn, Msg *msg, int status,
                   List *headers, Octstr *body)
{
    /* Test on three cases:
     * 1. an smsbox reply of an remote kannel instance
     * 2. an smsc_http response (if used for MT to MO looping)
     * 3. an smsbox reply of partly successful sendings */
    if ((status == HTTP_OK || status == HTTP_ACCEPTED)
        && (octstr_case_compare(body, octstr_imm("0: Accepted for delivery")) == 0 ||
            octstr_case_compare(body, octstr_imm("Sent.")) == 0 ||
            octstr_case_compare(body, octstr_imm("Ok.")) == 0 ||
            octstr_ncompare(body, octstr_imm("Result: OK"),10) == 0)) {
        char id[UUID_STR_LEN + 1];
        Octstr *mid;

        /* create Octstr from UUID */
        uuid_unparse(msg->sms.id, id);
        mid = octstr_create(id);

        /* add to our own DLR storage */
        if (DLR_IS_ENABLED_DEVICE(msg->sms.dlr_mask))
            dlr_add(conn->id, mid, msg, 0);

        octstr_destroy(mid);

        bb_smscconn_sent(conn, msg, NULL);
    } else {
        bb_smscconn_send_failed(conn, msg,
                SMSCCONN_FAILED_MALFORMED, octstr_duplicate(body));
    }
}


static void kannel_receive_sms(SMSCConn *conn, HTTPClient *client,
                               List *headers, Octstr *body, List *cgivars)
{
    ConnData *conndata = conn->data;
    Octstr *user, *pass, *from, *to, *text, *udh, *account, *binfo, *charset;
    Octstr *dlrmid, *dlrerr;
    Octstr *tmp_string, *retmsg;
    int mclass, mwi, coding, validity, deferred, dlrmask;
    List *reply_headers;
    int ret;

    mclass = mwi = coding = validity =
        deferred = dlrmask = SMS_PARAM_UNDEFINED;

    user = http_cgi_variable(cgivars, "username");
    pass = http_cgi_variable(cgivars, "password");
    from = http_cgi_variable(cgivars, "from");
    to = http_cgi_variable(cgivars, "to");
    text = http_cgi_variable(cgivars, "text");
    udh = http_cgi_variable(cgivars, "udh");
    charset = http_cgi_variable(cgivars, "charset");
    account = http_cgi_variable(cgivars, "account");
    binfo = http_cgi_variable(cgivars, "binfo");
    dlrmid = http_cgi_variable(cgivars, "dlr-mid");
    tmp_string = http_cgi_variable(cgivars, "flash");
    if (tmp_string) {
        sscanf(octstr_get_cstr(tmp_string),"%d", &mclass);
    }
    tmp_string = http_cgi_variable(cgivars, "mclass");
    if (tmp_string) {
        sscanf(octstr_get_cstr(tmp_string),"%d", &mclass);
    }
    tmp_string = http_cgi_variable(cgivars, "mwi");
    if (tmp_string) {
        sscanf(octstr_get_cstr(tmp_string),"%d", &mwi);
    }
    tmp_string = http_cgi_variable(cgivars, "coding");
    if (tmp_string) {
        sscanf(octstr_get_cstr(tmp_string),"%d", &coding);
    }
    tmp_string = http_cgi_variable(cgivars, "validity");
    if (tmp_string) {
        sscanf(octstr_get_cstr(tmp_string),"%d", &validity);
    }
    tmp_string = http_cgi_variable(cgivars, "deferred");
    if (tmp_string) {
        sscanf(octstr_get_cstr(tmp_string),"%d", &deferred);
    }
    tmp_string = http_cgi_variable(cgivars, "dlr-mask");
    if (tmp_string) {
        sscanf(octstr_get_cstr(tmp_string),"%d", &dlrmask);
    }
    dlrerr = http_cgi_variable(cgivars, "dlr-err");

    debug("smsc.http.kannel", 0, "HTTP[%s]: Received an HTTP request",
          octstr_get_cstr(conn->id));

    if (user == NULL || pass == NULL ||
        octstr_compare(user, conndata->username) != 0 ||
        octstr_compare(pass, conndata->password) != 0) {

        error(0, "HTTP[%s]: Authorization failure",
              octstr_get_cstr(conn->id));
        retmsg = octstr_create("Authorization failed for sendsms");
    } else if (dlrmask != 0 && dlrmid != NULL) {
        /* we got a DLR, and we don't require additional values */
        Msg *dlrmsg;

        dlrmsg = dlr_find(conn->id,
            dlrmid, /* message id */
            to, /* destination */
            dlrmask, 0);

        if (dlrmsg != NULL) {
            dlrmsg->sms.sms_type = report_mo;
            dlrmsg->sms.msgdata = octstr_duplicate(text);
            dlrmsg->sms.account = octstr_duplicate(conndata->username);

            debug("smsc.http.kannel", 0, "HTTP[%s]: Received DLR for DLR-URL <%s>",
                  octstr_get_cstr(conn->id), octstr_get_cstr(dlrmsg->sms.dlr_url));

            if (dlrerr != NULL) {
                /* pass errorcode as is */
                if (dlrmsg->sms.meta_data == NULL)
                    dlrmsg->sms.meta_data = octstr_create("");

                meta_data_set_value(dlrmsg->sms.meta_data, METADATA_DLR_GROUP,
                                    octstr_imm(METADATA_DLR_GROUP_ERRORCODE), dlrerr, 1);
            }

            ret = bb_smscconn_receive(conn, dlrmsg);
            if (ret == -1)
                retmsg = octstr_create("Not accepted");
            else
                retmsg = octstr_create("Sent.");
        } else {
            error(0,"HTTP[%s]: Got DLR but could not find message or was not interested "
                  "in it id<%s> dst<%s>, type<%d>",
                  octstr_get_cstr(conn->id), octstr_get_cstr(dlrmid),
                  octstr_get_cstr(to), dlrmask);
            retmsg = octstr_create("Unknown DLR, not accepted");
        }
    }
    else if (from == NULL || to == NULL || text == NULL) {

        error(0, "HTTP[%s]: Insufficient args",
              octstr_get_cstr(conn->id));
        retmsg = octstr_create("Insufficient args, rejected");
    }
    else if (udh != NULL && (octstr_len(udh) != octstr_get_char(udh, 0) + 1)) {
        error(0, "HTTP[%s]: UDH field misformed, rejected",
              octstr_get_cstr(conn->id));
        retmsg = octstr_create("UDH field misformed, rejected");
    }
    else if (udh != NULL && octstr_len(udh) > MAX_SMS_OCTETS) {
        error(0, "HTTP[%s]: UDH field is too long, rejected",
              octstr_get_cstr(conn->id));
        retmsg = octstr_create("UDH field is too long, rejected");
    }
    else {
        /* we got a normal MO SMS */
        Msg *msg;
        msg = msg_create(sms);

        debug("smsc.http.kannel", 0, "HTTP[%s]: Constructing new SMS",
              octstr_get_cstr(conn->id));

        msg->sms.service = octstr_duplicate(user);
        msg->sms.sender = octstr_duplicate(from);
        msg->sms.receiver = octstr_duplicate(to);
        msg->sms.msgdata = octstr_duplicate(text);
        msg->sms.udhdata = octstr_duplicate(udh);

        msg->sms.smsc_id = octstr_duplicate(conn->id);
        msg->sms.time = time(NULL);
        msg->sms.mclass = mclass;
        msg->sms.mwi = mwi;
        msg->sms.coding = coding;
        msg->sms.validity = (validity == SMS_PARAM_UNDEFINED ? validity : time(NULL) + validity * 60);
        msg->sms.deferred = (deferred == SMS_PARAM_UNDEFINED ? deferred : time(NULL) + deferred * 60);
        msg->sms.account = octstr_duplicate(account);
        msg->sms.binfo = octstr_duplicate(binfo);

        /* re-encode content if necessary */
        if (sms_charset_processing(charset, msg->sms.msgdata, msg->sms.coding) == -1) {
            error(0, "HTTP[%s]: Charset or body misformed, rejected",
                  octstr_get_cstr(conn->id));
            retmsg = octstr_create("Charset or body misformed, rejected");
        }
        else {

            ret = bb_smscconn_receive(conn, msg);
            if (ret == -1)
                retmsg = octstr_create("Not accepted");
            else
                retmsg = octstr_create("Sent.");
        }
    }

    reply_headers = gwlist_create();
    http_header_add(reply_headers, "Content-Type", "text/plain");
    debug("smsc.http.kannel", 0, "HTTP[%s]: Sending reply",
          octstr_get_cstr(conn->id));
    http_send_reply(client, HTTP_ACCEPTED, reply_headers, retmsg);

    octstr_destroy(retmsg);
    http_destroy_headers(reply_headers);
}

struct smsc_http_fn_callbacks smsc_http_kannel_callback = {
        .send_sms = kannel_send_sms,
        .parse_reply = kannel_parse_reply,
        .receive_sms = kannel_receive_sms,
};

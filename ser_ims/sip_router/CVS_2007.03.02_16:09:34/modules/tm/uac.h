/*
 * $Id$
 *
 *
 * Copyright (C) 2001-2003 FhG Fokus
 *
 * This file is part of ser, a free SIP server.
 *
 * ser is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * For a license to use the ser software under conditions
 * other than those described here, or to purchase support for this
 * software, please contact iptel.org by e-mail at the following addresses:
 *    info@iptel.org
 *
 * ser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * History:
 * ----------
 * 2003-02-28 protocolization of t_uac_dlg completed (jiri)
 */

#ifndef _UAC_H
#define _UAC_H

#include <stdio.h>
#include "../../str.h"
#include "dlg.h"
#include "t_hooks.h"
#include "h_table.h"

#define DEFAULT_CSEQ 10 /* Default CSeq number */

extern char *uac_from;  /* UAC From parameter */
extern int pass_provisional_replies; /* Pass provisional replies to fifo applications */

/*
 * Function prototypes
 */
typedef int (*reqwith_t)(str* m, str* h, str* b, dlg_t* d, transaction_cb c, void* cp);
typedef int (*reqout_t)(str* m, str* t, str* f, str* h, str* b, dlg_t** d, transaction_cb c, void* cp);
typedef int (*req_t)(str* m, str* ruri, str* t, str* f, str* h, str* b, str *next_hop, transaction_cb c, void* cp);
typedef int (*t_uac_t)(str* method, str* headers, str* body, dlg_t* dialog, transaction_cb cb, void* cbp);
typedef int (*t_uac_with_ids_t)(str* method, str* headers, str* body, dlg_t* dialog, transaction_cb cb, void* cbp,
		unsigned int *ret_index, unsigned int *ret_label);
typedef int (*prepare_request_within_f)(str* method, str* headers, 
		str* body, dlg_t* dialog, transaction_cb cb, void* cbp,
		struct retr_buf **request_dst);
typedef void (*send_prepared_request_f)(struct retr_buf *request_dst);


/*
 * Generate a fromtag based on given Call-ID
 */
void generate_fromtag(str* tag, str* callid);


/*
 * Initialization function
 */
int uac_init(void);


/*
 * Send a request
 */
int t_uac(str* method, str* headers, str* body, dlg_t* dialog, transaction_cb cb, void* cbp);

/*
 * Send a request
 * ret_index and ret_label will identify the new cell
 */
int t_uac_with_ids(str* method, str* headers, str* body, dlg_t* dialog, transaction_cb cb, void* cbp,
			unsigned int *ret_index, unsigned int *ret_label);
/*
 * Send a message within a dialog
 */
int req_within(str* m, str* h, str* b, dlg_t* d, transaction_cb c, void* cp);


/*
 * Send an initial request that will start a dialog
 */
int req_outside(str* m, str* t, str* f, str* h, str* b, dlg_t** d, transaction_cb c, void* cp);

/*
 * Send a transactional request, no dialogs involved
 */
int request(str* m, str* ruri, str* to, str* from, str* h, str* b, str *next_hop, transaction_cb c, void* cp);

int prepare_req_within(str* method, str* headers, str* body, dlg_t* dialog,
	  transaction_cb cb, void* cbp, struct retr_buf **dst_req);

void send_prepared_request(struct retr_buf *request);


#endif

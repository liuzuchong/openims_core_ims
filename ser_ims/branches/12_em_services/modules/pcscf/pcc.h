/*
 * $Id: p_persistency.h 236 2007-04-18 12:53:40 +0000 (Wed, 18 Apr 2007) vingarzan $
 *
 * Copyright (C) 2004-2007 FhG Fokus
 *
 * This file is part of Open IMS Core - an open source IMS CSCFs & HSS
 * implementation
 *
 * Open IMS Core is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * For a license to use the Open IMS Core software under conditions
 * other than those described here, or to purchase support for this
 * software, please contact Fraunhofer FOKUS by e-mail at the following
 * addresses:
 *     info@open-ims.org
 *
 * Open IMS Core is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * It has to be noted that this Open Source IMS Core System is not
 * intended to become or act as a product in a commercial context! Its
 * sole purpose is to provide an IMS core reference implementation for
 * IMS technology testing and IMS application prototyping for research
 * purposes, typically performed in IMS test-beds.
 *
 * Users of the Open Source IMS Core System have to be aware that IMS
 * technology may be subject of patents and licence terms, as being
 * specified within the various IMS-related IETF, ITU-T, ETSI, and 3GPP
 * standards. Thus all Open IMS Core users have to take notice of this
 * fact and have to agree to check out carefully before installing,
 * using and extending the Open Source IMS Core System, if related
 * patents and licences may become applicable to the intended usage
 * context.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/**
 * \file
 *
 * P-CSCF Policy and Charging Control interface ops
 *
 * \author Alberto Diez Albaladejo -at- fokus dot fraunhofer dot de
 */
 
#ifndef __PCC_H_
#define __PCC_H_

#include "mod.h"
#include "../cdp/cdp_load.h"

#include "dlg_state.h"




typedef struct authdata {
	str callid;
	str host;
	int port,transport;
	enum p_dialog_direction direction; // 0 ORIGINATING  1 TERMINATING

	//for registration session
	int subscribed_to_signaling_path_status;
	//for Gqprima only
	int latch;
} t_authdata;


int cscf_get_mobile_side(struct sip_msg *msg);
void terminate_pcc_session(cdp_session_t *);


AAAMessage* PCC_AAR(struct sip_msg *req, struct sip_msg *res, char *str1);
AAAMessage* PCC_STR(struct sip_msg *msg, char *str1);
AAAMessage* PCC_ASA(AAAMessage *request);
int PCC_AAA(AAAMessage *msg);



AAAMessage* PCCRequestHandler(AAAMessage *request,void *param);


#endif /*__PCC_H_*/

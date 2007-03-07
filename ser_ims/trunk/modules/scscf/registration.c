/*
 * $Id$
 *  
 * Copyright (C) 2004-2006 FhG Fokus
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
 * Serving-CSCF - Registration Related Operations
 * 
 * 
 *  \author Dragos Vingarzan vingarzan -at- fokus dot fraunhofer dot de
 * 
 */


#include "registration.h"

#include "../tm/tm_load.h"
#include "../cdp/cdp_load.h"
#include "sip.h"
#include "cx.h"
#include "cx_avp.h"
#include "conversion.h"
#include "registrar.h"
#include "sip_messages.h" 
#include "rfc2617.h"
#include "s_persistency.h"

extern struct tm_binds tmb;						/**< Structure with pointers to tm funcs 		*/
extern struct cdp_binds cdpb;					/**< Structure with pointers to cdp funcs 		*/

extern str registration_default_algorithm_s;	/**< fixed default algorithm for registration (if none present)	*/

extern str scscf_name_str;						/**< fixed name of the S-CSCF 					*/
extern str scscf_service_route;					/**< the service route header					*/

extern int auth_data_hash_size;					/**< the size of the hash table 				*/
extern int auth_vector_timeout;					/**< timeout for a sent auth vector to expire in sec 		*/
extern int auth_data_timeout;					/**< timeout for a hash entry to expire when empty in sec 	*/
extern int av_request_at_once;					/**< how many auth vectors to request in a MAR 				*/
extern int av_request_at_sync;					/**< how many auth vectors to request in a sync MAR 		*/	


str digest_akav1={"Digest-AKAv1-MD5",16};
str digest_akav2={"Digest-AKAv2-MD5",16};
str digest_md5={"Digest-MD5",10};
str akav1={"AKAv1-MD5",9};
str akav2={"AKAv2-MD5",9};
str md5={"MD5",3};

/**
 * Convert the SIP Algorithm to Diameter Authorization Scheme.
 * @param algorithm - the SIP Algorithm
 * @returns the Diameter Authorization Scheme
 */
str convertAuthSchemeSIPtoDiameter(str algorithm)
{
	if (algorithm.len == akav1.len && strncasecmp(algorithm.s,akav1.s,akav1.len)==0)
		return digest_akav1;
	if (algorithm.len == akav2.len && strncasecmp(algorithm.s,akav2.s,akav2.len)==0)
		return digest_akav2;
	if (algorithm.len == md5.len && strncasecmp(algorithm.s,md5.s,md5.len)==0)
		return digest_md5;
	return algorithm;
}

/**
 * Convert the Diameter Authorization Scheme to SIP Algorithm.
 * @param scheme - the Diameter Authorization Scheme
 * @returns the SIP Algorithm
 */
str convertAuthSchemeDiametertoSip(str scheme)
{
	if (scheme.len == digest_akav1.len && strncasecmp(scheme.s,digest_akav1.s,digest_akav1.len)==0)
		return akav1;
	if (scheme.len == digest_akav2.len && strncasecmp(scheme.s,digest_akav2.s,digest_akav2.len)==0)
		return akav2;
	if (scheme.len == digest_md5.len && strncasecmp(scheme.s,digest_md5.s,digest_md5.len)==0)
		return md5;
	return scheme;
}




/**
 * Copies the Path header from REGISTER request to reply, inserts the Service-Route.
 * @param msg - the SIP message to operator on
 * @param str1 - no used
 * @param str2 - no used
 * @returns #CSCF_RETURN_TRUE on success or #CSCF_RETURN_FALSE if not added
 */
int S_add_path_service_routes(struct sip_msg *msg,char *str1,char *str2 )
{
	struct hdr_field *h;
	str t={0,0};
	if (parse_headers(msg,HDR_EOH_F,0)<0){
		LOG(L_ERR,"ERR:"M_NAME":S_REGISTER_reply: Error parsing headers\n");
		return CSCF_RETURN_FALSE;
	}
	h = msg->headers;
	while(h){
		if (h->name.len == 4 &&
			strncasecmp(h->name.s,"Path",4)==0)
		{
			t.s = h->name.s;
			t.len = h->len;
			if (!cscf_add_header_rpl(msg,&(t))) return CSCF_RETURN_FALSE;
		}
		h = h->next;
	}
	
	if (!cscf_add_header_rpl(msg,&scscf_service_route)) return CSCF_RETURN_FALSE;

	return CSCF_RETURN_TRUE;
}

static str s_service_route_s = {"Service-Route: <",16};
static str s_service_route_e = {";lr>\r\n",6};
/**
 * Copies the Path header from REGISTER request to reply, inserts the Service-Route.
 * @param msg - the SIP message to operator on
 * @param str1 - no used
 * @param str2 - no used
 * @returns #CSCF_RETURN_TRUE on success or #CSCF_RETURN_FALSE if not added
 */
int S_add_service_route(struct sip_msg *msg,char *str1,char *str2 )
{
	str sr={0,0};
	str uri;
	uri.s = str1;
	uri.len = strlen(str1);
	
	sr.len = s_service_route_s.len+uri.len+s_service_route_e.len;
	sr.s = pkg_malloc(sr.len);
	if (!sr.s){
		LOG(L_ERR,"ERR:"M_NAME":S_add_service_route: Error allocating %d bytes\n",sr.len);
		return CSCF_RETURN_FALSE;
	}		
	sr.len = 0;
	STR_APPEND(sr,s_service_route_s);
	STR_APPEND(sr,uri);
	STR_APPEND(sr,s_service_route_e);
	
	if (!cscf_add_header_rpl(msg,&sr)) {
		if (sr.s) pkg_free(sr.s);
		return CSCF_RETURN_FALSE;
	}
	
	if (sr.s) pkg_free(sr.s);
	return CSCF_RETURN_TRUE;
}

/**
 * Replies to a REGISTER and also adds the need headers
 * Path and Service-Route are added.
 * @param msg - the SIP message to operator on
 * @param code - Reason Code for the response
 * @param text - Reason Phrase for the response
 * @returns #CSCF_RETURN_TRUE on success or #CSCF_RETURN_FALSE if not added
 */
int S_REGISTER_reply(struct sip_msg *msg, int code,  char *text)
{
	struct hdr_field *h;
	str t={0,0};
	if (parse_headers(msg,HDR_EOH_F,0)<0){
		LOG(L_ERR,"ERR:"M_NAME":S_REGISTER_reply: Error parsing headers\n");
		return -1;
	}
	h = msg->headers;
	while(h){
		if (h->name.len == 4 &&
			strncasecmp(h->name.s,"Path",4)==0)
		{
			t.s = h->name.s;
			t.len = h->len;
			cscf_add_header_rpl(msg,&(t));
		}
		h = h->next;
	}
	
	if (code==200){
		cscf_add_header_rpl(msg,&scscf_service_route);
	}
	return cscf_reply_transactional(msg,code,text);
}

/**
 * Checks if the P-Visited-Network-Id matches a regexp.
 * @param msg - the SIP message
 * @param str1 - the regexp to check with
 * @param str2 - not used
 * @returns #CSCF_RETURN_TRUE on match, #CSCF_RETURN_FALSE else
 */
int S_check_visited_network_id(struct sip_msg *msg,char *str1,char *str2 )
{
	int ret = CSCF_RETURN_FALSE;
	char c;
	str v={0,0};
	regex_t r;
	struct hdr_field *hdr;
	if (regcomp(&r,str1,REG_EXTENDED|REG_ICASE)){
		LOG(L_ERR,"ERR:"M_NAME":S_check_visited_network_id: Error compiling regexp <%s>\n",str1);
		return ret;
	}
	v = cscf_get_visited_network_id(msg,&hdr);
	c = v.s[v.len];
	v.s[v.len] = 0;
	if (regexec(&r,v.s,0,NULL,0)==0) ret = CSCF_RETURN_TRUE;
	regfree(&r);
	v.s[v.len] = c;
	return ret;
}

/**
 * Checks if the message is integrity protected.
 * @param msg - the SIP message
 * @param str1 - the realm
 * @param str2 - not used
 * @returns - boolean values to script or break on error
 */
int S_is_integrity_protected(struct sip_msg *msg,char *str1,char *str2 )
{
	str realm={0,0};
	int ret=0;

	realm.s = str1;realm.len = strlen(str1);
	if (!realm.len) {
		LOG(L_ERR,"ERR:"M_NAME":S_is_integrity_protected: No realm found\n");
		return CSCF_RETURN_BREAK;
	}
	if (cscf_get_integrity_protected(msg,realm)) ret = 1;
	
	LOG(L_DBG,"DBG:"M_NAME":S_is_integrity_protected: returns %d\n",ret);
	
	return ret?CSCF_RETURN_TRUE:CSCF_RETURN_FALSE;
}

char *zero_padded="00000000000000000000000000000000";
static str empty_s={0,0};
/**
 * Checks if the message contains a valid response to a valid challenge.
 * @param msg - the SIP message
 * @param str1 - the realm
 * @param str2 - not used
 * @returns #CSCF_RETURN_TRUE if authorized, #CSCF_RETURN_TRUE if not or #CSCF_RETURN_ERROR on error
 */
int S_is_authorized(struct sip_msg *msg,char *str1,char *str2 )
{
	int ret=CSCF_RETURN_FALSE;
	unsigned int aud_hash=0;
	str realm;
	str private_identity,public_identity;
	str nonce,response16;
	str uri={0,0};
	auth_vector *av;
	HASHHEX expected,ha1,hbody;
	int expected_len=32;

	LOG(L_DBG,"DBG:"M_NAME":S_is_authorized: Checking if REGISTER is authorized...\n");
	
	/* First check the parameters */
	if (msg->first_line.type!=SIP_REQUEST||
		msg->first_line.u.request.method.len!=8||
		memcmp(msg->first_line.u.request.method.s,"REGISTER",8)!=0)
	{
		LOG(L_ERR,"ERR:"M_NAME":S_is_authorized: This message is not a REGISTER request\n");
		goto error;
	}		
	realm.s = str1;realm.len = strlen(str1);
	if (!realm.len) {
		LOG(L_ERR,"ERR:"M_NAME":S_is_authorized: No realm found\n");
		return CSCF_RETURN_BREAK;
	}
	
	if (!cscf_get_nonce_response(msg,realm,&nonce,&response16)||
		!nonce.len || !response16.len)
	{
		LOG(L_DBG,"DBG:"M_NAME":S_is_authorized: Nonce or reponse missing\n");
		return ret;
	}
	
	private_identity = cscf_get_private_identity(msg,realm);
	if (!private_identity.len) {
		LOG(L_ERR,"ERR:"M_NAME":S_is_authorized: private identity missing\n");
		return ret;
	}
	
	public_identity = cscf_get_public_identity(msg);
	if (!public_identity.len) {
		LOG(L_ERR,"ERR:"M_NAME":S_is_authorized: public identity missing\n");		
		return ret;
	}
	uri = cscf_get_digest_uri(msg,realm);
	
	av = get_auth_vector(private_identity,public_identity,AUTH_VECTOR_SENT,&nonce,&aud_hash);
	
	if (!av) {
		LOG(L_ERR,"ERR:"M_NAME":S_is_authorized: no matching auth vector found - maybe timer expired\n");		
		return ret;
	}
	if ((av->algorithm.len == akav1.len && strncasecmp(av->algorithm.s,akav1.s,akav1.len)==0)||
		(av->algorithm.len == akav2.len && strncasecmp(av->algorithm.s,akav2.s,akav2.len)==0)||
		(av->algorithm.len == md5.len   && strncasecmp(av->algorithm.s,md5.s,md5.len)==0)){

//		LOG(L_CRIT,"A1: %.*s:%.*s:%.*s\n",private_identity.len,private_identity.s,
//			realm.len,realm.s,av->authorization.len,av->authorization.s);
			
		calc_HA1(HA_MD5,&private_identity,&realm,&(av->authorization),&(av->authenticate),0,ha1);
		calc_response(ha1,&(av->authenticate),
			&empty_s,&empty_s,&empty_s,0,
			&msg->first_line.u.request.method,&uri,hbody,expected);
		LOG(L_INFO,"DBG:"M_NAME":S_is_authorized: UE said: %.*s and we  expect %.*s ha1 %.*s\n",
			response16.len,response16.s,/*av->authorization.len,av->authorization.s,*/32,expected,32,ha1);		
	}else{
		LOG(L_ERR,"ERR:"M_NAME":S_is_authorized: algorithm %.*s is not handled.\n",
			av->algorithm.len,av->algorithm.s);
		goto error;
	}

	if (response16.len==expected_len && strncasecmp(response16.s,expected,response16.len)==0){	
		av->status = AUTH_VECTOR_USELESS;
		ret = CSCF_RETURN_TRUE;		
	}else {
		av->status = AUTH_VECTOR_USED;/* first mistake, you're out! (but maybe it's synchronization) */
		LOG(L_DBG,"DBG:"M_NAME":S_is_authorized: UE said: %.*s, but we have %.*s and expect %.*s\n",
			response16.len,response16.s,av->authorization.len,av->authorization.s,32,expected);		
	}		
	
		
	auth_data_unlock(aud_hash);
	return ret;	
error:
	ret = CSCF_RETURN_ERROR;		
	return ret;
}

/**
 * Responds with a 401 Unauthorized containing the (new) challenge.
 * @param msg - the SIP message
 * @param str1 - the realm
 * @param str2 - not used
 * @returns #CSCF_RETURN_BREAK if response sent, #CSCF_RETURN_FALSE if not or #CSCF_RETURN_ERROR on error
 */
int S_challenge(struct sip_msg *msg,char *str1,char *str2 )
{
	int ret=CSCF_RETURN_FALSE;
	unsigned int aud_hash;
	str realm,private_identity,public_identity,auts={0,0},nonce={0,0};
	auth_vector *av=0;
	str algo={0,0};
	
	LOG(L_DBG,"DBG:"M_NAME":S_challenge: Challenging the REGISTER...\n");

	/* First check the parameters */
	if (msg->first_line.type!=SIP_REQUEST||
		msg->first_line.u.request.method.len!=8||
		memcmp(msg->first_line.u.request.method.s,"REGISTER",8)!=0)
	{
		LOG(L_ERR,"ERR:"M_NAME":S_challenge: This message is not a REGISTER request\n");
		goto error;
	}		
	realm.s = str1;realm.len = strlen(str1);
	if (!realm.len) {
		LOG(L_ERR,"ERR:"M_NAME":S_challenge: No realm found\n");
		return CSCF_RETURN_BREAK;
	}
	/* get the private_identity */
	private_identity = cscf_get_private_identity(msg,realm);
	if (!private_identity.len){
		LOG(L_ERR,"ERR:"M_NAME":S_challenge: No private identity specified (Authorization: username)\n");
		S_REGISTER_reply(msg,403,MSG_403_NO_PRIVATE);
		goto abort;
	}
	/* get the public_identity */
	public_identity = cscf_get_public_identity(msg);
	if (!public_identity.len){
		LOG(L_ERR,"ERR:"M_NAME":S_challenge: No public identity specified (To:)\n");
		S_REGISTER_reply(msg,403,MSG_403_NO_PUBLIC);
		goto abort;
	}
	/* get the requested algorithm */
	algo = cscf_get_algorithm(msg,realm);
	if (algo.len==0)
		algo = registration_default_algorithm_s;
	
	/* check if it is a synchronization request */
	auts = cscf_get_auts(msg,realm);
	if (auts.len){
		LOG(L_DBG,"DBG:"M_NAME":S_challenge: Syncronization requested <%.*s>\n",
			auts.len,auts.s);
		
		nonce = cscf_get_nonce(msg,realm);
		if (nonce.len==0){
			LOG(L_DBG,"L_DBG:"M_NAME":S_challenge: Nonce not found (Authorization: nonce)\n");
			S_REGISTER_reply(msg,403,MSG_403_NO_NONCE);
			goto abort;
		}
		av = get_auth_vector(private_identity,public_identity,AUTH_VECTOR_USED,&nonce,&aud_hash);
		if (!av)
	    	av = get_auth_vector(private_identity,public_identity,AUTH_VECTOR_SENT,&nonce,&aud_hash);
					
		if (!av){
			LOG(L_ERR,"DBG:"M_NAME":S_challenge: Nonce not regonized as sent, no sync!\n");			
			auts.len = 0; auts.s=0;
		}else{
			av->status = AUTH_VECTOR_USELESS;
			auth_data_unlock(aud_hash);
			av =0;
		}
		/* if synchronization - force MAR - if MAR ok, old avs will be droped*/
		S_MAR(msg,public_identity,private_identity,av_request_at_sync,
				algo,nonce,auts,scscf_name_str,realm);
	}
	
	/* loop because some other process might steal the auth_vector that we just retrieved */
	while(!(av=get_auth_vector(private_identity,public_identity,AUTH_VECTOR_UNUSED,0,&aud_hash))){
		if (!S_MAR(msg,public_identity,private_identity,av_request_at_once,
				algo,nonce,auts,scscf_name_str,realm)) break;
		/* do sync just once */
		auts.len=0;auts.s=0;
	}

	if (!av){
		LOG(L_ERR,"ERR:"M_NAME":S_challenge: Error retrieving an auth vector\n");
		goto abort;
	}

	if (!pack_challenge(msg,realm,av)){
		S_REGISTER_reply(msg,500,MSG_500_PACK_AV);
		auth_data_unlock(aud_hash);
		goto error;
	}
	start_reg_await_timer(av);
	//S_REGISTER_reply(msg,401,MSG_401_CHALLENGE);
	auth_data_unlock(aud_hash);
	return ret;
error:
	ret = CSCF_RETURN_ERROR;	
	return 	ret;
abort:
	ret = CSCF_RETURN_BREAK;	
	return 	ret;
}


str S_WWW_Authorization_AKA={"WWW-Authenticate: Digest realm=\"%.*s\","
	" nonce=\"%.*s\", algorithm=%.*s, ck=\"%.*s\", ik=\"%.*s\" \r\n",107};
str S_WWW_Authorization_MD5={"WWW-Authenticate: Digest realm=\"%.*s\","
	" nonce=\"%.*s\", algorithm=%.*s \r\n",102};
/**
 * Adds the WWW-Authenticate header for challenge, based on the authentication vector.
 * @param msg - SIP message to add the header to
 * @param realm - the realm
 * @param av - the authentication vector
 * @returns 1 on success, 0 on error 
 */ 
int pack_challenge(struct sip_msg *msg,str realm,auth_vector *av)
{
	str x;
	char ck[32],ik[32];
	int ck_len,ik_len;
	if ((av->algorithm.len == akav1.len && strncasecmp(av->algorithm.s,akav1.s,akav1.len)==0)||
	    (av->algorithm.len == akav2.len && strncasecmp(av->algorithm.s,akav2.s,akav2.len)==0)){
	    /* AKA */
		ck_len = bin_to_base16(av->ck.s,16,ck);
		ik_len = bin_to_base16(av->ik.s,16,ik);
		x.len = S_WWW_Authorization_AKA.len +
			realm.len+
			av->authenticate.len+
			av->algorithm.len+
			ck_len+
			ik_len;		
		x.s = pkg_malloc(x.len);
		if (!x.s) {
			LOG(L_ERR,"ERR:"M_NAME":pack_challenge: Error allocating %d bytes\n",
				x.len);
			goto error;
		}			
		sprintf(x.s,S_WWW_Authorization_AKA.s,
			realm.len,realm.s,
			av->authenticate.len,av->authenticate.s,
			av->algorithm.len,av->algorithm.s,
			ck_len,ck,
			ik_len,ik);
		x.len = strlen(x.s);
	}else if (av->algorithm.len == md5.len && strncasecmp(av->algorithm.s,md5.s,md5.len)==0){
		/* MD5 */
		x.len = S_WWW_Authorization_MD5.len +
			realm.len+
			av->authenticate.len+
			av->algorithm.len;
		x.s = pkg_malloc(x.len);
		if (!x.s) {
			LOG(L_ERR,"ERR:"M_NAME":pack_challenge: Error allocating %d bytes\n",
				x.len);
			goto error;
		}			
		sprintf(x.s,S_WWW_Authorization_MD5.s,
			realm.len,realm.s,
			av->authenticate.len,av->authenticate.s,
			av->algorithm.len,av->algorithm.s);
		x.len = strlen(x.s);
	}else{
		LOG(L_CRIT,"ERR:"M_NAME":pack_challenge: not implemented for algorithm %.*s\n",
			av->algorithm.len,av->algorithm.s);
		goto error;
	}
		
	if (cscf_add_header_rpl(msg,&x)) {
		pkg_free(x.s);
		return 1;
	}
	
error:
	if (x.s) pkg_free(x.s);			
	return 0;
}

/**
 * Sends a Multimedia-Authentication-Response to retrieve some authentication vectors and maybe synchronize.
 * Must respond with a SIP reply every time it returns 0
 * @param msg - the SIP REGISTER message
 * @param public_identity - the public identity
 * @param private_identity - the private identity
 * @param count - how many vectors to request
 * @param algorithm - which algorithm to request
 * @param nonce - the challenge that will be sent
 * @param auts - the AKA synchronization or empty string if not a synchronization
 * @param server_name - the S-CSCF name to be saved on the HSS
 * @param realm - the realm
 * @returns 1 on success, 0 on failure
 */
int S_MAR(struct sip_msg *msg, str public_identity, str private_identity,
					int count,str algorithm,str nonce,str auts,str server_name,str realm)
{
	AAAMessage *maa;
	AAA_AVP *auth_data;
	int rc=-1,experimental_rc=-1;
	auth_vector *av=0, **avlist=0;
	int cnt,i,j;
	int item_number;
	int is_sync=0;
	str auth_scheme={0,0},authenticate={0,0},authorization={0,0},ck={0,0},ik={0,0};
		
	if (auts.len){
		authorization.s = pkg_malloc(nonce.len*3/4+auts.len*3/4+8);
		if (!authorization.s) goto done;
		authorization.len = base64_to_bin(nonce.s,nonce.len,authorization.s);
		authorization.len += base64_to_bin(auts.s,auts.len,authorization.s+authorization.len);		
		is_sync=1;
	}
	
	auth_scheme = convertAuthSchemeSIPtoDiameter(algorithm);
	
	maa = Cx_MAR(msg,public_identity,private_identity,count,auth_scheme,authorization,
					server_name,realm);

	if (authorization.s) pkg_free(authorization.s);
	
	if (!maa){
		//TODO - add the warning code 99 in the reply	
		S_REGISTER_reply(msg,480,MSG_480_DIAMETER_TIMEOUT);		
		goto done;
	}
	
	if (!Cx_get_result_code(maa,&rc)&&
		!Cx_get_experimental_result_code(maa,&experimental_rc))
	{
		S_REGISTER_reply(msg,480,MSG_480_DIAMETER_MISSING_AVP);		
		goto done;	
	}
	
	switch(rc){
		case -1:
			switch(experimental_rc){
				case RC_IMS_DIAMETER_ERROR_USER_UNKNOWN:
					S_REGISTER_reply(msg,403,MSG_403_USER_UNKNOWN);		
					break;
				case RC_IMS_DIAMETER_ERROR_IDENTITIES_DONT_MATCH:
					S_REGISTER_reply(msg,403,MSG_403_IDENTITIES_DONT_MATCH);		
					break;
				case RC_IMS_DIAMETER_ERROR_AUTH_SCHEME_NOT_SUPPORTED:
					S_REGISTER_reply(msg,403,MSG_403_AUTH_SCHEME_UNSOPPORTED);		
					break;
				
				default:
					S_REGISTER_reply(msg,403,MSG_403_UNKOWN_EXPERIMENTAL_RC);		
			}
			break;
		
		case AAA_UNABLE_TO_COMPLY:
			S_REGISTER_reply(msg,403,MSG_403_UNABLE_TO_COMPLY);		
			break;
				
		case AAA_SUCCESS:
			goto success;			
			break;
						
		default:
			S_REGISTER_reply(msg,403,MSG_403_UNKOWN_RC);		
	}
	
goto done;		
	
success:

	/* if HSS accepted the synchronization, drop old auth vectors */
	if (is_sync)
		drop_auth_userdata(private_identity,public_identity);

	Cx_get_sip_number_auth_items(maa,&cnt);
	if (!cnt) {
		S_REGISTER_reply(msg,403,MSG_403_NO_AUTH_DATA);		
		goto done;
	}
	avlist = shm_malloc(sizeof(auth_vector *)*cnt);
	if (!avlist) {
		S_REGISTER_reply(msg,403,MSG_480_HSS_ERROR);		
		goto done;
	}
	cnt = 0;
	auth_data = 0;
	while((Cx_get_auth_data_item_answer(maa,&auth_data,&item_number,
			&auth_scheme,&authenticate,&authorization,&ck,&ik)))
	{
		av = new_auth_vector(item_number,auth_scheme,authenticate,
			authorization,ck,ik);
		
		if (cnt==0) avlist[cnt++]=av;
		else {
			i = cnt;
			while(i>0 && avlist[i-1]->item_number > av->item_number)
				i--;
			for(j=cnt;j>i;j--)
				avlist[j]=avlist[j-1];
			avlist[i]=av;
			cnt++;
		}
				
		auth_data->code = - auth_data->code;
	}
	for(i=0;i<cnt;i++)
		if (!add_auth_vector(private_identity,public_identity,avlist[i])) 
			free_auth_vector(avlist[i]);
	
	cdpb.AAAFreeMessage(&maa);
	shm_free(avlist);	
	return 1;
done:	
	if (maa) cdpb.AAAFreeMessage(&maa);
	return 0;
}




/*
 * Storage of authentication vectors
 */
 
auth_hash_slot_t *auth_data;			/**< Authentication vector hash table */
extern int auth_data_hash_size;						/**< authentication vector hash table size */

/**
 * Locks the required slot of the auth_data.
 * @param hash - the index of the slot
 */
inline void auth_data_lock(unsigned int hash)
{
//	LOG(L_CRIT,"GET %d\n",hash);
	lock_get(auth_data[(hash)].lock);
//	LOG(L_CRIT,"GOT %d\n",hash);	
}

/**
 * UnLocks the required slot of the auth_data
 * @param hash - the index of the slot
 */
inline void auth_data_unlock(unsigned int hash)
{
	lock_release(auth_data[(hash)].lock);
//	LOG(L_CRIT,"RELEASED %d\n",hash);	
}


/**
 * Initializes the Authorization Data structures.
 * @param size - size of the hash table
 * @returns 1 on success or 0 on error
 */
int auth_data_init(int size)
{
	int i;
	auth_data = shm_malloc(sizeof(auth_hash_slot_t)*size);
	if (!auth_data) {
		LOG(L_ERR,"ERR:"M_NAME":auth_data_init: error allocating mem\n");
		return 0;
	}
	memset(auth_data,0,sizeof(auth_hash_slot_t)*size);
	auth_data_hash_size = size;
	for(i=0;i<size;i++){
		auth_data[i].lock = lock_alloc();
		lock_init(auth_data[i].lock);
	}
	return 1;
}

/**
 * Destroy the Authorization Data structures */
void auth_data_destroy()
{
	int i;
	auth_userdata *aud,*next;
	for(i=0;i<auth_data_hash_size;i++){
		auth_data_lock(i);
		lock_destroy(auth_data[i].lock);
		lock_dealloc(auth_data[i].lock);
		aud = auth_data[i].head;
		while(aud){
			next = aud->next;
			free_auth_userdata(aud);
			aud = next;
		}		
	}
	if (auth_data) shm_free(auth_data);
}

/**
 * Create new authorization vector
 * @param item_number - number to index it in the vectors list
 * @param auth_scheme - Diameter Authorization Scheme
 * @param authenticate - the challenge
 * @param authorization - the expected response
 * @param ck - the cypher key
 * @param ik - the integrity key
 * @returns the new auth_vector* or NULL on error
 */
auth_vector *new_auth_vector(int item_number,str auth_scheme,str authenticate,
			str authorization,str ck,str ik)
{
	auth_vector *x=0;
	str algorithm;
	x = shm_malloc(sizeof(auth_vector));
	if (!x){
		LOG(L_ERR,"ERR:"M_NAME":new_auth_vector: error allocating mem\n");
		goto done;
	}
	
	x->item_number = item_number;

	algorithm = convertAuthSchemeDiametertoSip(auth_scheme);
	x->algorithm.len = algorithm.len;
	x->algorithm.s = shm_malloc(algorithm.len);
	if (!x->algorithm.s){
		LOG(L_ERR,"ERR:"M_NAME":new_auth_vector: error allocating mem\n");
		goto done;
	}
	memcpy(x->algorithm.s,algorithm.s,algorithm.len);

	if ((algorithm.len == akav1.len && strncasecmp(algorithm.s,akav1.s,akav1.len)==0)||
		(algorithm.len == akav2.len && strncasecmp(algorithm.s,akav2.s,akav2.len)==0)){
		/* AKA */
		x->authenticate.len = authenticate.len*4/3+4;
		x->authenticate.s = shm_malloc(x->authenticate.len);
		if (!x->authenticate.s){
			LOG(L_ERR,"ERR:"M_NAME":new_auth_vector: error allocating mem\n");
			goto done;
		}
		x->authenticate.len = bin_to_base64(authenticate.s,authenticate.len,x->authenticate.s);
		
//		Old version - converting to base16 the XRES
//		x->authorization.len = authorization.len*2;
//		x->authorization.s = shm_malloc(x->authorization.len);
//		if (!x->authorization.s){
//			LOG(L_ERR,"ERR:"M_NAME":new_auth_vector: error allocating mem\n");
//			goto done;
//		}
//		x->authorization.len = bin_to_base16(authorization.s,authorization.len,x->authorization.s);

		x->authorization.len = authorization.len;
		x->authorization.s = shm_malloc(x->authorization.len);
		if (!x->authorization.s){
			LOG(L_ERR,"ERR:"M_NAME":new_auth_vector: error allocating mem\n");
			goto done;
		}
		memcpy(x->authorization.s,authorization.s,authorization.len);
	}
	else if (algorithm.len == md5.len && strncasecmp(algorithm.s,md5.s,md5.len)==0){
		/* MD5 and all else */
		x->authenticate.len = authenticate.len*2;
		x->authenticate.s = shm_malloc(x->authenticate.len);
		if (!x->authenticate.s){
			LOG(L_ERR,"ERR:"M_NAME":new_auth_vector: error allocating mem\n");
			goto done;
		}		
		x->authenticate.len = bin_to_base16(authenticate.s,authenticate.len,x->authenticate.s);		

		x->authorization.len = authorization.len;
		x->authorization.s = shm_malloc(x->authorization.len);
		if (!x->authorization.s){
			LOG(L_ERR,"ERR:"M_NAME":new_auth_vector: error allocating mem\n");
			goto done;
		}		
		memcpy(x->authorization.s,authorization.s,authorization.len);
		x->authorization.len = authorization.len;		
	}else{
		x->authenticate.len=0;
		x->authenticate.s=0;
	}
	

	x->ck.len = ck.len;
	x->ck.s = shm_malloc(ck.len);
	if (!x->ck.s){
		LOG(L_ERR,"ERR:"M_NAME":new_auth_vector: error allocating mem\n");
		goto done;
	}
	memcpy(x->ck.s,ck.s,ck.len);

	x->ik.len = ik.len;
	x->ik.s = shm_malloc(ik.len);
	if (!x->ik.s){
		LOG(L_ERR,"ERR:"M_NAME":new_auth_vector: error allocating mem\n");
		goto done;
	}
	memcpy(x->ik.s,ik.s,ik.len);
	
	x->next=0;
	x->prev=0;
	x->status=AUTH_VECTOR_UNUSED;
	x->expires = 0;

done:	
	return x;
}

/**
 * Frees the memory taken by a authentication vector
 * @param av - the vector to be freed
 */
void free_auth_vector(auth_vector *av)
{
	if (av){
		if (av->algorithm.s) shm_free(av->algorithm.s);
		if (av->authenticate.s) shm_free(av->authenticate.s);
		if (av->authorization.s) shm_free(av->authorization.s);
		if (av->ck.s) shm_free(av->ck.s);
		if (av->ik.s) shm_free(av->ik.s);
		shm_free(av);
	}
}

/**
 * Creates a new Authorization Userdata structure.
 * @param private_identity - the private identity to attach to
 * @param public_identity - the public identity to attach to
 * @returns the new auth_userdata* on success or NULL on error
 */
auth_userdata *new_auth_userdata(str private_identity,str public_identity)
{
	auth_userdata *x=0;
	
	x = shm_malloc(sizeof(auth_userdata));
	if (!x){
		LOG(L_ERR,"ERR:"M_NAME":new_auth_userdata: error allocating mem\n");
		goto done;
	}

	x->private_identity.len = private_identity.len;	
	x->private_identity.s = shm_malloc(private_identity.len);
	if (!x){
		LOG(L_ERR,"ERR:"M_NAME":new_auth_userdata: error allocating mem\n");
		goto done;
	}
	memcpy(x->private_identity.s,private_identity.s,private_identity.len);
	
	x->public_identity.len = public_identity.len;	
	x->public_identity.s = shm_malloc(public_identity.len);
	if (!x){
		LOG(L_ERR,"ERR:"M_NAME":new_auth_userdata: error allocating mem\n");
		goto done;
	}
	memcpy(x->public_identity.s,public_identity.s,public_identity.len);
	
	x->head=0;
	x->tail=0;
	
	x->next=0;
	x->prev=0;
done:	
	return x;
}

/**
 * Deallocates the auth_userdata.
 * @param aud - the auth_userdata to be deallocated
 */
void free_auth_userdata(auth_userdata *aud)
{
	auth_vector *av,*next;
	if (aud){
		if (aud->private_identity.s) shm_free(aud->private_identity.s);
		if (aud->public_identity.s) shm_free(aud->public_identity.s);
		av = aud->head;
		while(av){
			next = av->next;
			free_auth_vector(av);
			av = next;
		}
		shm_free(aud);
	}
}

/**
 * Computes a hash based on the private and public identities
 * @param private_identity - the private identity
 * @param public_identity - the public identity
 * @returns the hash % Auth_data->size
 */
inline unsigned int get_hash_auth(str private_identity,str public_identity)
{
#define h_inc h+=v^(v>>3)
   char* p;
   register unsigned v;
   register unsigned h;

   h=0;
   for (p=private_identity.s; p<=(private_identity.s+private_identity.len-4); p+=4){
       v=(*p<<24)+(p[1]<<16)+(p[2]<<8)+p[3];
       h_inc;
   }
   v=0;
   for (;p<(private_identity.s+private_identity.len); p++) {
       v<<=8;
       v+=*p;
   }
   h_inc;
   for (p=public_identity.s; p<=(public_identity.s+public_identity.len-4); p+=4){
       v=(*p<<24)+(p[1]<<16)+(p[2]<<8)+p[3];
       h_inc;
   }
   v=0;
   for (;p<(public_identity.s+public_identity.len); p++) {
       v<<=8;
       v+=*p;
   }

   h=((h)+(h>>11))+((h>>13)+(h>>23));
   return (h)%auth_data_hash_size;
#undef h_inc 
}

/**
 * Retrieve the auth_userdata for a user.
 * \note you will return with lock on the hash slot, so release it!
 * @param private_identity - the private identity
 * @param public_identity - the public identity
 * @returns the auth_userdata* found or newly created on success, NULL on error
 */
auth_userdata* get_auth_userdata(str private_identity,str public_identity)
{
	
	unsigned int hash=0;
	auth_userdata *aud=0;
	
	
	hash = get_hash_auth(private_identity,public_identity);
	auth_data_lock(hash);
	aud = auth_data[hash].head;
	while(aud){
		if (aud->private_identity.len == private_identity.len &&
			aud->public_identity.len == public_identity.len &&
			memcmp(aud->private_identity.s,private_identity.s,private_identity.len)==0 &&
			memcmp(aud->public_identity.s,public_identity.s,public_identity.len)==0)
		{
			return aud;
		}
		aud = aud->next;
	}
	/* if we get here, there is no auth_userdata for this user */
	aud = new_auth_userdata(private_identity,public_identity);
	if (!aud) {
		auth_data_unlock(hash);
		return 0;
	}
	
	aud->prev = auth_data[hash].tail;
	aud->next = 0;
	aud->hash = hash;
	
	if (!auth_data[hash].head) auth_data[hash].head = aud;
	if (auth_data[hash].tail) auth_data[hash].tail->next = aud;
	auth_data[hash].tail = aud;
	
	return aud;
}

//void del_auth_userdata(auth_userdata *x)
//{
//	/* You must have the lock on the hash table when you get here!!! */
//	/* it is supposed that you removed all attached auth_vectors from this one */
//	hash_slot_t *slot;
//	slot = Auth_data->table + x->hash;
//	if (x->prev) x->prev->next = x->next;
//	else slot->head = x->next;
//	if (x->next) x->next->prev = x->prev;
//	else slot->tail = x->prev;
//	free_auth_userdata(x);
//}	

/**
 * Add an authentication vector to the authentication userdata storage.
 * @param private_identity - the private identity
 * @param public_identity - the public identity
 * @param av - the authentication vector
 * @returns 1 on success or 0 on error
 */
int add_auth_vector(str private_identity,str public_identity,auth_vector *av)
{
	auth_userdata *aud;
	aud = get_auth_userdata(private_identity,public_identity);
	if (!aud) goto error;

	av->prev = aud->tail;
	av->next = 0;
	
	if (!aud->head) aud->head = av;
	if (aud->tail) aud->tail->next = av;
	aud->tail = av;
	
	auth_data_unlock(aud->hash);
	return 1;
error:
	return 0;
}

/**
 * Retrieve an authentication vector.
 * \note returns with a lock, so unlock it when done
 * @param private_identity - the private identity
 * @param public_identity - the public identity
 * @param status - the status of the authentication vector
 * @param nonce - the nonce in the auth vector
 * @param hash - the hash to unlock when done
 * @returns the auth_vector* if found or NULL if not
 */
auth_vector* get_auth_vector(str private_identity,str public_identity,int status,str *nonce,unsigned int *hash)
{
	auth_userdata *aud;
	auth_vector *av;
	aud = get_auth_userdata(private_identity,public_identity);
	if (!aud) goto error;

	av = aud->head;
	while(av){
		if (av->status == status &&
			(nonce==0 || (nonce->len == av->authenticate.len &&
						  memcmp(nonce->s,av->authenticate.s,nonce->len)==0)))
		{
			*hash = aud->hash;
			return av;
		}
		av = av->next;
	}
	
error:
	if (aud) auth_data_unlock(aud->hash);
	return 0;
}

/**
 * Declares all auth vectors as useless when we do a synchronization
 * @param private_identity - the private identity
 * @param public_identity - the public identity
 * @returns 1 on sucess, 0 on error
 */
int drop_auth_userdata(str private_identity,str public_identity)
{
	auth_userdata *aud;
	auth_vector *av;
	aud = get_auth_userdata(private_identity,public_identity);
	if (!aud) goto error;

	av = aud->head;
	while(av){
		av->status = AUTH_VECTOR_USELESS;
		av = av->next;
	}
	auth_data_unlock(aud->hash);
	return 1;
error:	
	if (aud) auth_data_unlock(aud->hash);
	return 0;
}

/**
 * Starts the reg_await_timer for an authentication vector.
 * @param av - the authentication vector
 */
inline void start_reg_await_timer(auth_vector *av)
{
	av->expires = get_ticks() + auth_vector_timeout;
	av->status = AUTH_VECTOR_SENT;
}

/**
 * Timer callback for reg await timers.
 * Drops the auth vectors that have been sent and are expired
 * Also drops the useless auth vectors - used and no longer needed
 * @param ticks - what's the time
 * @param param - a given parameter to be called with
 */
void reg_await_timer(unsigned int ticks, void* param)
{
	auth_userdata *aud,*aud_next;
	auth_vector *av,*av_next;
	auth_hash_slot_t *ad;
	int i;
	ad = (auth_hash_slot_t*) param;
	
	LOG(L_DBG,"DBG:"M_NAME":reg_await_timer: Looking for expired/useless at %d\n",ticks);
	for(i=0;i<auth_data_hash_size;i++){
		auth_data_lock(i);
		aud = auth_data[i].head;
		while(aud){
			LOG(L_DBG,"DBG:"M_NAME":reg_await_timer: . Slot %4d <%.*s>\n",
				aud->hash,aud->private_identity.len,aud->private_identity.s);
			aud_next = aud->next;
			av = aud->head;
			while(av){
				LOG(L_DBG,"DBG:"M_NAME":reg_await_timer: .. AV %4d - %d Exp %3d  %p\n",
					av->item_number,av->status,(int)av->expires,av);
				av_next = av->next;
				if (av->status == AUTH_VECTOR_USELESS ||					 
				    ( (av->status == AUTH_VECTOR_USED || av->status == AUTH_VECTOR_SENT) && av->expires<ticks)
				   )
				{
					LOG(L_DBG,"DBG:"M_NAME":reg_await_timer: ... dropping av %d - %d\n",						
						av->item_number,av->status);
					if (av->prev) av->prev->next = av->next;
					else aud->head = av->next;
					if (av->next) av->next->prev = av->prev;
					else aud->tail = av->prev;
					free_auth_vector(av);
				}
				av = av_next;
			}
			if (!aud->head){
				if (aud->expires==0){
					LOG(L_DBG,"DBG:"M_NAME":reg_await_timer: ... started empty aud drop timer\n");
					aud->expires=ticks+auth_data_timeout;
				}
				else 			
				if (aud->expires<ticks){
					LOG(L_DBG,"DBG:"M_NAME":reg_await_timer: ... dropping aud \n");
					if (aud->prev) aud->prev->next = aud->next;
					else auth_data[i].head = aud->next;
					if (aud->next) aud->next->prev = aud->prev;
					else auth_data[i].tail = aud->prev;
					free_auth_userdata(aud);	
				}
			}
			else aud->expires=0;
				
			aud = aud_next;
		}
		auth_data_unlock(i);
	}
}


/*
 * $Id$
 * 
 * Copyright (C) 2009 FhG Fokus
 * 
 * This file is part of the Wharf project.
 * 
 */

/**
 * \file
 * 
 * Client_Rf module - User Data Request Logic - Rf-Pull
 * 
 * 
 *  \author Andreea Onofrei Corici andreea dot ancuta dot corici -at- fokus dot fraunhofer dot de
 * 
 */  
 
#include "../cdp_avp/mod_export.h"

#include "acr.h"
#include "Rf_data.h"

extern cdp_avp_bind_t *cavpb;

int Rf_write_event_type_avps(AAA_AVP_LIST * avp_list, event_type_t * x)
{
	AAA_AVP_LIST aList = {0,0};
	
	if (x->sip_method){
		if(!cavpb->epcapp.add_SIP_Method(&aList, *(x->sip_method),AVP_DUPLICATE_DATA))
			goto error;
	}

	if (x->event)
		if (!cavpb->epcapp.add_Event(&aList,*(x->event),0))
			goto error;	

	if (x->expires)
		if (!cavpb->epcapp.add_Expires(avp_list, *(x->expires)))
			goto error;	

	if (!cavpb->epcapp.add_Event_Type(avp_list, &aList, AVP_DONT_FREE_DATA))
		goto error;

	return 1;
error:	
	cavpb->cdp->AAAFreeAVPList(&aList);
	LOG(L_ERR, "error while adding event type avps\n");
	return 0;
}

int Rf_write_time_stamps_avps(AAA_AVP_LIST * avp_list,  time_stamps_t* x)
{
	AAA_AVP_LIST aList = {0,0};

	if (x->sip_request_timestamp)	
		if (!cavpb->epcapp.add_SIP_Request_Timestamp(&aList, *(x->sip_request_timestamp))) 
			goto error;
	
	if (x->sip_request_timestamp_fraction)	
		if (!cavpb->epcapp.add_SIP_Request_Timestamp_Fraction(&aList, 
					*(x->sip_request_timestamp_fraction))) 
			goto error;

	if (x->sip_response_timestamp)	
		if (!cavpb->epcapp.add_SIP_Response_Timestamp(&aList, *(x->sip_response_timestamp))) 
			goto error;

	if (x->sip_response_timestamp_fraction)	
		if (!cavpb->epcapp.add_SIP_Response_Timestamp_Fraction(&aList, 
					*(x->sip_response_timestamp_fraction))) 
			goto error;

	if (!cavpb->epcapp.add_Time_Stamps(avp_list, &aList, AVP_DONT_FREE_DATA))
		goto error;


	return 1;
error:
	cavpb->cdp->AAAFreeAVPList(&aList);
	LOG(L_ERR, "error while adding time stamps avps\n");

	return 0;
}

int Rf_write_ims_information_avps(AAA_AVP_LIST * avp_list, ims_information_t* x)
{
	str_list_slot_t * sl = 0;
	AAA_AVP_LIST aList = {0,0};
	AAA_AVP_LIST aList2 = {0,0};
	service_specific_info_list_element_t * info = 0;
	ioi_list_element_t * ioi_elem = 0;

	if (x->event_type)
		if(!Rf_write_event_type_avps(&aList2, x->event_type))
			goto error;

	if (x->role_of_node)
		if (!cavpb->epcapp.add_Role_Of_Node(&aList2, *(x->role_of_node))) goto error;

	if (!cavpb->epcapp.add_Node_Functionality(&aList2, x->node_functionality))
		goto error;

	if(x->user_session_id)
		if (!cavpb->epcapp.add_User_Session_Id(&aList2,*(x->user_session_id),0)) 
			goto error;

	if(x->outgoing_session_id)
		if (!cavpb->epcapp.add_Outgoing_Session_Id(&aList2,*(x->outgoing_session_id),0))	
			goto error;

	for(sl = x->calling_party_address.head; sl; sl=sl->next){
		if(!cavpb->epcapp.add_Calling_Party_Address(&aList2,sl->data,0))
			goto error;
	}
	
	if (x->called_party_address)
		if (!cavpb->epcapp.add_Called_Party_Address(&aList2,*(x->called_party_address),0))
			goto error;	

	for(sl = x->called_asserted_identity.head;sl;sl=sl->next){
		if(!cavpb->epcapp.add_Called_Asserted_Identity(&aList2,sl->data,0))
			goto error;
	}

	if (x->requested_party_address)
		if (!cavpb->epcapp.add_Requested_Party_Address(&aList2,*(x->requested_party_address),0))
			goto error;
	
	if (x->time_stamps)
		if(!Rf_write_time_stamps_avps(&aList2, x->time_stamps))
			goto error;
	
	for (ioi_elem = x->ioi.head; ioi_elem; ioi_elem = ioi_elem->next){
		
		if (ioi_elem->info.originating_ioi)
			if (!cavpb->epcapp.add_Originating_IOI(&aList,*(ioi_elem->info.originating_ioi),0))
				goto error;

		if (ioi_elem->info.terminating_ioi)
			if (!cavpb->epcapp.add_Terminating_IOI(&aList,*(ioi_elem->info.terminating_ioi),0))
				goto error;
		
		if (!cavpb->epcapp.add_Inter_Operator_Identifier(&aList2,&aList,0))
			goto error;
		aList.head = aList.tail = 0;
	}

	if (x->icid)
		if (!cavpb->epcapp.add_IMS_Charging_Identifier(&aList2,*(x->icid),0)) 
			goto error;

	if (x->service_id)	
		if (!cavpb->epcapp.add_Service_ID(&aList2,*(x->service_id),0)) 
			goto error;
	
	for (info = x->service_specific_info.head; info; info = info->next){
		
		if(info->info.data)		
			if (!cavpb->epcapp.add_Service_Specific_Data(&aList,*(info->info.data),0))
				goto error;
		if (info->info.type)
			if (!cavpb->epcapp.add_Service_Specific_Type(&aList,*(info->info.type))) 
				goto error;
		
		if (!cavpb->epcapp.add_Service_Specific_Info(&aList2, &aList, 0))
			goto error;
		aList.head = aList.tail = 0;
	}

	if (x->cause_code)
		if (!cavpb->epcapp.add_Cause_Code(&aList2,*(x->cause_code))) 
			goto error;
	
	if (!cavpb->epcapp.add_IMS_Information(avp_list, &aList2, AVP_DONT_FREE_DATA))
		goto error;
	
	return 1;
error:
	/*free aList*/
	cavpb->cdp->AAAFreeAVPList(&aList);
	cavpb->cdp->AAAFreeAVPList(&aList2);
	LOG(L_ERR, "could not add ims information avps\n");
	return 0;
}

int Rf_write_service_information_avps(AAA_AVP_LIST * avp_list, service_information_t* x)
{
	subscription_id_list_element_t * elem = 0;
	AAA_AVP_LIST aList = {0,0};

	for (elem = x->subscription_id.head; elem; elem= elem->next){

		if(!cavpb->ccapp.add_Subscription_Id_Group(&aList,elem->s.type,elem->s.id,0))
			goto error;
	}
	
	if (x->ims_information)
		if(!Rf_write_ims_information_avps(&aList, x->ims_information))
			goto error;

	if (!cavpb->epcapp.add_Service_Information(avp_list, &aList, AVP_DONT_FREE_DATA))
		goto error;

	return 1;
error:
	cavpb->cdp->AAAFreeAVPList(&aList);
	return 0;
}

AAAMessage * Rf_write_ACR_avps(AAAMessage * acr, Rf_ACR_t* x)
{
	
	if (!acr) return 0;

	if (!cavpb->base.add_Origin_Host(&(acr->avpList),x->origin_host,0)) goto error;

	if (!cavpb->base.add_Origin_Realm(&(acr->avpList),x->origin_realm,0)) goto error;
	if (!cavpb->base.add_Destination_Realm(&(acr->avpList),x->destination_realm,0)) goto error;

	if (!cavpb->base.add_Accounting_Record_Type(&(acr->avpList),x->acct_record_type)) goto error;
	if (!cavpb->base.add_Accounting_Record_Number(&(acr->avpList),x->acct_record_number)) goto error;
	
	if (x->user_name)
		if (!cavpb->base.add_User_Name(&(acr->avpList),*(x->user_name),AVP_DUPLICATE_DATA)) goto error;
	
	if (x->acct_interim_interval)
		if (!cavpb->base.add_Acct_Interim_Interval(&(acr->avpList),*(x->acct_interim_interval))) goto error;
	
	if (x->origin_state_id)
		if (!cavpb->base.add_Origin_State_Id(&(acr->avpList),*(x->origin_state_id))) goto error;
	
	if (x->event_timestamp)
		if (!cavpb->base.add_Event_Timestamp(&(acr->avpList),*(x->event_timestamp))) goto error;	
		
	if (x->service_context_id)
		if (!cavpb->ccapp.add_Service_Context_Id(&(acr->avpList),*(x->service_context_id),0)) goto error;
	
	if (x->service_information)
		if(!Rf_write_service_information_avps(&(acr->avpList), x->service_information))
			goto error;

	return acr;
error:
	cavpb->cdp->AAAFreeMessage(&acr);
	return 0;
}

AAAMessage *Rf_new_acr(AAASession * session, Rf_ACR_t * rf_data){

	AAAMessage * acr = 0;
	acr = cavpb->cdp->AAACreateRequest(IMS_Rf, Diameter_ACR, Flag_Proxyable, session);
	if(!acr){
		LOG(L_ERR, "could not create ACR\n");
		return 0;
	}
	LOG(L_DBG, "created an ACR message\n");
	acr = Rf_write_ACR_avps(acr, rf_data);

	return acr;
}


/*int Rf_read_ACA_avps(AAAMessage *aca, Rf_ACA_t *x)
{
	if (!x) return 0;
	
	if (!cavpb->base.add_Result_Code(&(aca->avpList),x->result_code)) goto error;
	
	if (!cavpb->base.add_Accounting_Record_Type(&(aca->avpList),x->acct_record_type)) goto error;
	if (!cavpb->base.add_Accounting_Record_Number(&(aca->avpList),x->acct_record_number)) goto error;
	
	if (x->user_name)
		if (!cavpb->base.add_User_Name(&(aca->avpList),*(x->user_name),AVP_DUPLICATE_DATA)) goto error;
	
	if (x->acct_interim_interval)
		if (!cavpb->base.add_Acct_Interim_Interval(&(aca->avpList),*(x->acct_interim_interval))) goto error;
	
	if (x->origin_state_id)
		if (!cavpb->base.add_Origin_State_Id(&(aca->avpList),*(x->origin_state_id))) goto error;
	
	if (x->event_timestamp)
		if (!cavpb->base.add_Event_Timestamp(&(aca->avpList),*(x->event_timestamp))) goto error;
	
	return 1;
error:
	return 1;
}
*/
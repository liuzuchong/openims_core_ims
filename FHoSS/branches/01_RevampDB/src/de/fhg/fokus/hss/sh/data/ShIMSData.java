/*
  *  Copyright (C) 2004-2007 FhG Fokus
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
  * patents and licenses may become applicable to the intended usage
  * context. 
  *
  * You should have received a copy of the GNU General Public License
  * along with this program; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA  
  * 
  */
package de.fhg.fokus.hss.sh.data;

import java.util.Vector;

/**
 * @author adp dot fokus dot fraunhofer dot de 
 * Adrian Popescu / FOKUS Fraunhofer Institute
 */
public class ShIMSData {
	private String scscfName = null;
	private Vector<InitialFilterCriteria> ifcList;
	private int imsUserState = -1;
	private ChargingInformation chgInformation = null;
	
	public ShIMSData(){}

	public String toString(){
		StringBuffer sBuffer = new StringBuffer();
		sBuffer.append(ShDataTags.Sh_IMS_Data_s);
		
		if (scscfName != null){
			sBuffer.append(ShDataTags.SCSCFName_s);
			sBuffer.append(scscfName);
			sBuffer.append(ShDataTags.SCSCFName_e);
		}
		if (ifcList != null && ifcList.size() > 0) {
			sBuffer.append(ShDataTags.IFCs_s);
			InitialFilterCriteria ifc;
			for (int i = 0; i < ifcList.size(); i++){
				ifc = ifcList.get(i);
				sBuffer.append(ifc.toString());
			}
			sBuffer.append(ShDataTags.IFCs_e);
		}
		if (imsUserState != -1){
			sBuffer.append(ShDataTags.IMSUserState_s);
			sBuffer.append(imsUserState);
			sBuffer.append(ShDataTags.IMSUserState_e);
		}
		if (chgInformation != null){
			sBuffer.append(chgInformation.toString());
		}
		sBuffer.append(ShDataTags.Sh_IMS_Data_e);
		return sBuffer.toString();
	}
	
	public ChargingInformation getChgInformation() {
		return chgInformation;
	}

	public void setChgInformation(ChargingInformation chgInformation) {
		this.chgInformation = chgInformation;
	}

	public Vector<InitialFilterCriteria> getIfcList() {
		return ifcList;
	}

	public void setIfcList(Vector<InitialFilterCriteria> ifcList) {
		this.ifcList = ifcList;
	}

	public int getImsUserState() {
		return imsUserState;
	}

	public void setImsUserState(int imsUserState) {
		this.imsUserState = imsUserState;
	}

	public String getScscfName() {
		return scscfName;
	}

	public void setScscfName(String scscfName) {
		this.scscfName = scscfName;
	}
}
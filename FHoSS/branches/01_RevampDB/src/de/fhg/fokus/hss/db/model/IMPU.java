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
package de.fhg.fokus.hss.db.model;

import java.io.Serializable;


/**
 * @author adp dot fokus dot fraunhofer dot de 
 * Adrian Popescu / FOKUS Fraunhofer Institute
 */

public class IMPU implements Serializable{
	private static final long serialVersionUID=1L;
	
	// table fields
	private int id;
	private String identity;
	private int type;
	private int barring;
	private int user_state;
	private int id_implicit_set;
	private String wildcard_psi;
	private String display_name;
	private int psi_activation;
	private int can_register;

	private int id_sp;
	private int id_charging_info;
	public IMPU(){}

	public int getBarring() {
		return barring;
	}

	public void setBarring(int barring) {
		this.barring = barring;
	}

	public String getDisplay_name() {
		return display_name;
	}

	public void setDisplay_name(String display_name) {
		this.display_name = display_name;
	}

	public int getId() {
		return id;
	}

	public void setId(int id) {
		this.id = id;
	}

	public int getId_implicit_set() {
		return id_implicit_set;
	}

	public void setId_implicit_set(int id_implicit_set) {
		this.id_implicit_set = id_implicit_set;
	}

	public String getIdentity() {
		return identity;
	}

	public void setIdentity(String identity) {
		this.identity = identity;
	}

	public int getType() {
		return type;
	}

	public void setType(int type) {
		this.type = type;
	}

	public String getWildcard_psi() {
		return wildcard_psi;
	}

	public void setWildcard_psi(String wildcard_psi) {
		this.wildcard_psi = wildcard_psi;
	}

	public int getCan_register() {
		return can_register;
	}

	public void setCan_register(int can_register) {
		this.can_register = can_register;
	}

	public int getPsi_activation() {
		return psi_activation;
	}

	public void setPsi_activation(int psi_activation) {
		this.psi_activation = psi_activation;
	}

	public int getUser_state() {
		return user_state;
	}

	public void setUser_state(int user_state) {
		this.user_state = user_state;
	}

	public int getId_charging_info() {
		return id_charging_info;
	}

	public void setId_charging_info(int id_charging_info) {
		this.id_charging_info = id_charging_info;
	}

	public int getId_sp() {
		return id_sp;
	}

	public void setId_sp(int id_sp) {
		this.id_sp = id_sp;
	}
	
}

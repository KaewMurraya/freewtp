#include "ac.h"
#include "capwap_dfa.h"
#include "capwap_array.h"
#include "ac_session.h"

/* */
int ac_dfa_state_join(struct ac_session_t* session, struct capwap_parsed_packet* packet) {
	int i;
	struct capwap_header_data capwapheader;
	struct capwap_packet_txmng* txmngpacket;
	int status = AC_DFA_ACCEPT_PACKET;
	struct capwap_resultcode_element resultcode = { .code = CAPWAP_RESULTCODE_FAILURE };

	ASSERT(session != NULL);
	
	if (packet) {
		unsigned short binding;

		/* Check binding */
		binding = GET_WBID_HEADER(packet->rxmngpacket->header);
		if (ac_valid_binding(binding)) {
			if (packet->rxmngpacket->ctrlmsg.type == CAPWAP_JOIN_REQUEST) {
				resultcode.code = CAPWAP_RESULTCODE_SUCCESS;

				/* Get sessionid */
				memcpy(&session->sessionid, packet->messageelements.sessionid, sizeof(struct capwap_sessionid_element));

				/* Get binding */
				session->binding = binding;
			} else {
				resultcode.code = CAPWAP_RESULTCODE_MSG_UNEXPECTED_INVALID_CURRENT_STATE;
			}
		} else {
			resultcode.code = CAPWAP_RESULTCODE_JOIN_FAILURE_BINDING_NOT_SUPPORTED;
		}

		/* Create response */
		capwap_header_init(&capwapheader, CAPWAP_RADIOID_NONE, binding);
		txmngpacket = capwap_packet_txmng_create_ctrl_message(&capwapheader, CAPWAP_JOIN_RESPONSE, packet->rxmngpacket->ctrlmsg.seq, session->mtu);

		/* Add message element */
		capwap_packet_txmng_add_message_element(txmngpacket, CAPWAP_ELEMENT_RESULTCODE, &resultcode);

		/* Check is valid packet after parsing request */
		if ((resultcode.code == CAPWAP_RESULTCODE_SUCCESS) || (resultcode.code == CAPWAP_RESULTCODE_SUCCESS_NAT_DETECTED)) {
			struct capwap_list* controllist;
			struct capwap_list_item* item;

			/* Update statistics */
			ac_update_statistics();
			capwap_packet_txmng_add_message_element(txmngpacket, CAPWAP_ELEMENT_ACDESCRIPTION, &g_ac.descriptor);
			capwap_packet_txmng_add_message_element(txmngpacket, CAPWAP_ELEMENT_ACNAME, &g_ac.acname);

			if (binding == CAPWAP_WIRELESS_BINDING_IEEE80211) {
				for (i = 0; i < packet->messageelements.ieee80211.wtpradioinformation->count; i++) {
					struct capwap_80211_wtpradioinformation_element* radio;
		
					radio = *(struct capwap_80211_wtpradioinformation_element**)capwap_array_get_item_pointer(packet->messageelements.ieee80211.wtpradioinformation, i);
					capwap_packet_txmng_add_message_element(txmngpacket, CAPWAP_ELEMENT_80211_WTPRADIOINFORMATION, radio);
				}
			}

			capwap_packet_txmng_add_message_element(txmngpacket, CAPWAP_ELEMENT_ECNSUPPORT, &session->dfa.ecn);

			/* Get information from any local address */
			controllist = capwap_list_create();
			ac_get_control_information(controllist);

			for (item = controllist->first; item != NULL; item = item->next) {
				struct ac_session_control* sessioncontrol = (struct ac_session_control*)item->item;

				if (sessioncontrol->localaddress.ss_family == AF_INET) {
					struct capwap_controlipv4_element element;

					memcpy(&element.address, &((struct sockaddr_in*)&sessioncontrol->localaddress)->sin_addr, sizeof(struct in_addr));
					element.wtpcount = sessioncontrol->count;
					capwap_packet_txmng_add_message_element(txmngpacket, CAPWAP_ELEMENT_CONTROLIPV4, &element);
				} else if (sessioncontrol->localaddress.ss_family == AF_INET6) {
					struct capwap_controlipv6_element element;
					
					memcpy(&element.address, &((struct sockaddr_in6*)&sessioncontrol->localaddress)->sin6_addr, sizeof(struct in6_addr));
					element.wtpcount = sessioncontrol->count;
					capwap_packet_txmng_add_message_element(txmngpacket, CAPWAP_ELEMENT_CONTROLIPV6, &element);
				}
			}

			capwap_list_free(controllist);

			if (session->acctrladdress.ss_family == AF_INET) {
				struct capwap_localipv4_element addr;

				memcpy(&addr.address, &((struct sockaddr_in*)&session->acctrladdress)->sin_addr, sizeof(struct in_addr));
				capwap_packet_txmng_add_message_element(txmngpacket, CAPWAP_ELEMENT_LOCALIPV4, &addr);
			} else if (session->acctrladdress.ss_family == AF_INET6) {
				struct capwap_localipv6_element addr;

				memcpy(&addr.address, &((struct sockaddr_in6*)&session->acctrladdress)->sin6_addr, sizeof(struct in6_addr));
				capwap_packet_txmng_add_message_element(txmngpacket, CAPWAP_ELEMENT_LOCALIPV6, &addr);
			}

			/* CAPWAP_CREATE_ACIPV4LIST_ELEMENT */				/* TODO */
			/* CAPWAP_CREATE_ACIPV6LIST_ELEMENT */				/* TODO */
			capwap_packet_txmng_add_message_element(txmngpacket, CAPWAP_ELEMENT_TRANSPORT, &session->dfa.transport);
			/* CAPWAP_CREATE_IMAGEIDENTIFIER_ELEMENT */			/* TODO */
			/* CAPWAP_CREATE_MAXIMUMMESSAGELENGTH_ELEMENT */	/* TODO */
			/* CAPWAP_CREATE_VENDORSPECIFICPAYLOAD_ELEMENT */	/* TODO */
		}

		/* Join response complete, get fragment packets */
		ac_free_reference_last_response(session);
		capwap_packet_txmng_get_fragment_packets(txmngpacket, session->responsefragmentpacket, session->fragmentid);
		if (session->responsefragmentpacket->count > 1) {
			session->fragmentid++;
		}

		/* Free packets manager */
		capwap_packet_txmng_free(txmngpacket);

		/* Save remote sequence number */
		session->remoteseqnumber = packet->rxmngpacket->ctrlmsg.seq;
		capwap_get_packet_digest(packet->rxmngpacket, packet->connection, session->lastrecvpackethash);

		/* Send Join response to WTP */
		if (capwap_crypt_sendto_fragmentpacket(&session->ctrldtls, session->ctrlsocket.socket[session->ctrlsocket.type], session->responsefragmentpacket, &session->acctrladdress, &session->wtpctrladdress)) {
			ac_dfa_change_state(session, CAPWAP_POSTJOIN_STATE);
		} else {
			/* Error to send packets */
			capwap_logging_debug("Warning: error to send join response packet");
			ac_dfa_change_state(session, CAPWAP_JOIN_TO_DTLS_TEARDOWN_STATE);
			status = AC_DFA_NO_PACKET;
		}
	} else {
		/* Join timeout */
		ac_dfa_change_state(session, CAPWAP_JOIN_TO_DTLS_TEARDOWN_STATE);
		status = AC_DFA_NO_PACKET;
	}

	return status;
}

/* */
int ac_dfa_state_postjoin(struct ac_session_t* session, struct capwap_parsed_packet* packet) {
	int status = AC_DFA_ACCEPT_PACKET;

	ASSERT(session != NULL);

	if (packet) {
		if (packet->rxmngpacket->ctrlmsg.type == CAPWAP_CONFIGURATION_STATUS_REQUEST) {
			ac_dfa_change_state(session, CAPWAP_CONFIGURE_STATE);
			status = ac_dfa_state_configure(session, packet);
		} else if (packet->rxmngpacket->ctrlmsg.type == CAPWAP_IMAGE_DATA_REQUEST) {
			ac_dfa_change_state(session, CAPWAP_IMAGE_DATA_STATE);
			status = ac_dfa_state_imagedata(session, packet);
		} else {
			ac_dfa_change_state(session, CAPWAP_JOIN_TO_DTLS_TEARDOWN_STATE);
			status = AC_DFA_NO_PACKET;
		}
	} else {
		/* Join timeout */
		ac_dfa_change_state(session, CAPWAP_JOIN_TO_DTLS_TEARDOWN_STATE);
		status = AC_DFA_NO_PACKET;
	}

	return status;
}

/* */
int ac_dfa_state_join_to_dtlsteardown(struct ac_session_t* session, struct capwap_parsed_packet* packet) {
	return ac_session_teardown_connection(session);
}
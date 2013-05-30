#include "ac.h"
#include "capwap_dfa.h"
#include "ac_session.h"

#define PACKET_TIMEOUT			-1
#define DTLS_SHUTDOWN			-2

static int ac_network_read(struct ac_session_t* session, void* buffer, int length, int* isctrlpacket, struct timeout_control* timeout) {
	long indextimer;
	long waittimeout;
	
	ASSERT(session != NULL);
	ASSERT(buffer != NULL);
	ASSERT(length > 0);
	ASSERT(isctrlpacket != NULL);

	for (;;) {
		if (session->closesession) {
			session->closesession = 0;
			return DTLS_SHUTDOWN;
		}

		capwap_lock_enter(&session->packetslock);
		
		if ((session->controlpackets->count > 0) || (session->datapackets->count > 0)) {
			int result = 0;
			struct capwap_list_item* itempacket;

			*isctrlpacket = ((session->controlpackets->count > 0) ? 1 : 0);
			if (*isctrlpacket) {
				capwap_logging_debug("Receive control packet");
			} else {
				capwap_logging_debug("Receive data packet");
			}

			/* Get packet */
			itempacket = capwap_itemlist_remove_head((*isctrlpacket ? session->controlpackets : session->datapackets));
			capwap_lock_exit(&session->packetslock);

			if (itempacket) {
				struct ac_packet* packet = (struct ac_packet*)itempacket->item;
				long packetlength = itempacket->itemsize - sizeof(struct ac_packet);
				struct capwap_dtls* dtls = (*isctrlpacket ? &session->ctrldtls : &session->datadtls);
				
				if (!packet->plainbuffer && dtls->enable) {
					int oldaction = dtls->action;

					/* Decrypt packet */
					result = capwap_decrypt_packet(dtls, packet->buffer, packetlength, buffer, length);
					if (result == CAPWAP_ERROR_AGAIN) {
						/* Check is handshake complete */
						if ((oldaction == CAPWAP_DTLS_ACTION_HANDSHAKE) && (dtls->action == CAPWAP_DTLS_ACTION_DATA)) {
							if (*isctrlpacket) {
								if (session->state == CAPWAP_DTLS_CONNECT_STATE) {
									ac_dfa_change_state(session, CAPWAP_JOIN_STATE);
									capwap_set_timeout(session->dfa.rfcWaitJoin, &session->timeout, CAPWAP_TIMER_CONTROL_CONNECTION);
								}
							}
						}
					} else if (result == CAPWAP_ERROR_SHUTDOWN) {
						if ((oldaction == CAPWAP_DTLS_ACTION_DATA) && (dtls->action == CAPWAP_DTLS_ACTION_SHUTDOWN)) {
							result = DTLS_SHUTDOWN;
						}
					}
				} else {
					if (packetlength <= length) {
						memcpy(buffer, packet->buffer, packetlength);
						result = packetlength;
					}
				}

				/* Free packet */
				capwap_itemlist_free(itempacket);
			}

			return result;
		}

		capwap_lock_exit(&session->packetslock);

		/* Update timeout */
		capwap_update_timeout(timeout);
		waittimeout = capwap_get_timeout(timeout, &indextimer);
		if ((waittimeout <= 0) && (indextimer != CAPWAP_TIMER_UNDEF)) {
			return PACKET_TIMEOUT;
		}
		
		/* Wait packet */
		capwap_event_wait_timeout(&session->waitpacket, waittimeout);
	}

	return 0;
}

/* */
static int ac_dfa_execute(struct ac_session_t* session, struct capwap_parsed_packet* packet) {
	int action = AC_DFA_ACCEPT_PACKET;
	
	ASSERT(session != NULL);

	/* Execute state */
	switch (session->state) {
		case CAPWAP_START_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_START_TO_IDLE_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_IDLE_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}

		case CAPWAP_IDLE_TO_DISCOVERY_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_IDLE_TO_DTLS_SETUP_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_DISCOVERY_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_DISCOVERY_TO_IDLE_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_DISCOVERY_TO_SULKING_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_DISCOVERY_TO_DTLS_SETUP_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_SULKING_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}

		case CAPWAP_SULKING_TO_IDLE_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_DTLS_SETUP_STATE: {
			action = ac_dfa_state_dtlssetup(session, packet);
			break;
		}
		
		case CAPWAP_DTLS_SETUP_TO_IDLE_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_DTLS_SETUP_TO_SULKING_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_DTLS_SETUP_TO_AUTHORIZE_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_AUTHORIZE_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_AUTHORIZE_TO_DTLS_SETUP_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_AUTHORIZE_TO_DTLS_CONNECT_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_AUTHORIZE_TO_DTLS_TEARDOWN_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
					
		case CAPWAP_DTLS_CONNECT_STATE: {
			action = ac_dfa_state_dtlsconnect(session, packet);
			break;
		}
		
		case CAPWAP_DTLS_CONNECT_TO_DTLS_TEARDOWN_STATE: {
			action = ac_dfa_state_dtlsconnect_to_dtlsteardown(session, packet);
			break;
		}
		
		case CAPWAP_DTLS_CONNECT_TO_JOIN_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_DTLS_TEARDOWN_STATE: {
			action = ac_dfa_state_teardown(session, packet);
			break;
		}
		
		case CAPWAP_DTLS_TEARDOWN_TO_IDLE_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_DTLS_TEARDOWN_TO_SULKING_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_DTLS_TEARDOWN_TO_DEAD_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_JOIN_STATE: {
			action = ac_dfa_state_join(session, packet);
			break;
		}
		
		case CAPWAP_POSTJOIN_STATE: {
			action = ac_dfa_state_postjoin(session, packet);
			break;
		}
		
		case CAPWAP_JOIN_TO_DTLS_TEARDOWN_STATE: {
			action = ac_dfa_state_join_to_dtlsteardown(session, packet);
			break;
		}
		
		case CAPWAP_JOIN_TO_IMAGE_DATA_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_JOIN_TO_CONFIGURE_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_IMAGE_DATA_STATE: {
			action = ac_dfa_state_imagedata(session, packet);
			break;
		}
		
		case CAPWAP_IMAGE_DATA_TO_RESET_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_IMAGE_DATA_TO_DTLS_TEARDOWN_STATE: {
			action = ac_dfa_state_imagedata_to_dtlsteardown(session, packet);
			break;
		}
		
		case CAPWAP_CONFIGURE_STATE: {
			action = ac_dfa_state_configure(session, packet);
			break;
		}
		
		case CAPWAP_CONFIGURE_TO_RESET_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_CONFIGURE_TO_DTLS_TEARDOWN_STATE: {
			action = ac_dfa_state_configure_to_dtlsteardown(session, packet);
			break;
		}
		
		case CAPWAP_CONFIGURE_TO_DATA_CHECK_STATE: {
			/* Never called with this state */
			ASSERT(0);
			break;
		}
		
		case CAPWAP_RESET_STATE: {
			action = ac_dfa_state_reset(session, packet);
			break;
		}
		
		case CAPWAP_RESET_TO_DTLS_TEARDOWN_STATE: {
			action = ac_dfa_state_reset_to_dtlsteardown(session, packet);
			break;
		}
		
		case CAPWAP_DATA_CHECK_STATE: {
			action = ac_dfa_state_datacheck(session, packet);
			break;
		}
		
		case CAPWAP_DATA_CHECK_TO_DTLS_TEARDOWN_STATE: {
			action = ac_dfa_state_datacheck_to_dtlsteardown(session, packet);
			break;
		}
		
		case CAPWAP_DATA_CHECK_TO_RUN_STATE: {
			action = ac_dfa_state_datacheck_to_run(session, packet);
			break;
		}
		
		case CAPWAP_RUN_STATE: {
			action = ac_dfa_state_run(session, packet);
			break;
		}
		
		case CAPWAP_RUN_TO_DTLS_TEARDOWN_STATE: {
			action = ac_dfa_state_run_to_dtlsteardown(session, packet);
			break;
		}
		
		case CAPWAP_RUN_TO_RESET_STATE: {
			action = ac_dfa_state_run_to_reset(session, packet);
			break;
		}
		
		case CAPWAP_DEAD_STATE: {
			action = ac_dfa_state_dead(session, packet);
			break;
		}
		
		default: {
			capwap_logging_debug("Unknown action event: %lu", session->state);
			break;
		}			
	}
	
	return action;
}

/* */
static struct capwap_packet_rxmng* ac_get_packet_rxmng(struct ac_session_t* session, int isctrlmsg) {
	struct capwap_packet_rxmng* rxmngpacket = NULL;

	if (isctrlmsg) {
		if (!session->rxmngctrlpacket) {
			session->rxmngctrlpacket = capwap_packet_rxmng_create_message(1);
		}

		rxmngpacket = session->rxmngctrlpacket;
	} else {
		if (!session->rxmngdatapacket) {
			session->rxmngdatapacket = capwap_packet_rxmng_create_message(0);
		}

		rxmngpacket = session->rxmngdatapacket;
	}

	return rxmngpacket;
}

/* */
static void ac_free_packet_rxmng(struct ac_session_t* session, int isctrlmsg) {
	if (isctrlmsg && session->rxmngctrlpacket) {
		capwap_packet_rxmng_free(session->rxmngctrlpacket);
		session->rxmngctrlpacket = NULL;
	} else if (!isctrlmsg && session->rxmngdatapacket) {
		capwap_packet_rxmng_free(session->rxmngdatapacket);
		session->rxmngdatapacket = NULL;
	}
}

/* */
static void ac_session_run(struct ac_session_t* session) {
	int res;
	int check;
	int length;
	int isctrlsocket;
	struct capwap_connection connection;
	char buffer[CAPWAP_MAX_PACKET_SIZE];
	int action = AC_DFA_ACCEPT_PACKET;

	ASSERT(session != NULL);

	/* Configure DFA */
	if (g_ac.enabledtls) {
		action = AC_DFA_NO_PACKET;
		ac_dfa_change_state(session, CAPWAP_DTLS_SETUP_STATE);
		capwap_set_timeout(session->dfa.rfcWaitDTLS, &session->timeout, CAPWAP_TIMER_CONTROL_CONNECTION);
	} else {
		/* Wait Join request */
		ac_dfa_change_state(session, CAPWAP_JOIN_STATE);
		capwap_set_timeout(session->dfa.rfcWaitJoin, &session->timeout, CAPWAP_TIMER_CONTROL_CONNECTION);
	}
	
	while (action != AC_DFA_DEAD) {
		/* Get packet */
		if ((action == AC_DFA_ACCEPT_PACKET) || (action == AC_DFA_DROP_PACKET)) {
			length = ac_network_read(session, buffer, sizeof(buffer), &isctrlsocket, &session->timeout);
			if (length < 0) {
				if (length == PACKET_TIMEOUT) {
					action = ac_dfa_execute(session, NULL);		/* Timeout */
				} else if (length == DTLS_SHUTDOWN) {
					action = ac_session_teardown_connection(session);
				}
			} else if (length > 0) {
				/* Accept data packet only in running state */
				if (isctrlsocket || (session->state == CAPWAP_DATA_CHECK_TO_RUN_STATE) || (session->state == CAPWAP_RUN_STATE)) {
					/* Check generic capwap packet */
					check = capwap_sanity_check(isctrlsocket, CAPWAP_UNDEF_STATE, buffer, length, 0, 0);
					if (check == CAPWAP_PLAIN_PACKET) {
						struct capwap_parsed_packet packet;
						struct capwap_packet_rxmng* rxmngpacket;

						/* Defragment management */
						rxmngpacket = ac_get_packet_rxmng(session, isctrlsocket);

						/* If request, defragmentation packet */
						check = capwap_packet_rxmng_add_recv_packet(rxmngpacket, buffer, length);
						if (check == CAPWAP_RECEIVE_COMPLETE_PACKET) {
							int ignorepacket = 0;

							/* Receive all fragment */
							memcpy(&connection.socket, (isctrlsocket ? &session->ctrlsocket : &session->datasocket), sizeof(struct capwap_socket));
							memcpy(&connection.localaddr, (isctrlsocket ? &session->acctrladdress : &session->acdataaddress), sizeof(struct sockaddr_storage));
							memcpy(&connection.remoteaddr, (isctrlsocket ? &session->wtpctrladdress : &session->wtpdataaddress), sizeof(struct sockaddr_storage));

							if (isctrlsocket) {
								if (!capwap_recv_retrasmitted_request(&session->ctrldtls, rxmngpacket, &connection, session->lastrecvpackethash, session->responsefragmentpacket)) {
									/* Check message type */
									res = capwap_check_message_type(rxmngpacket);
									if (res != VALID_MESSAGE_TYPE) {
										if (res == INVALID_REQUEST_MESSAGE_TYPE) {
											/*TODO wtp_send_invalid_request(rxmngpacket, &connection);*/
										}
		
										ignorepacket = 1;
										capwap_logging_debug("Invalid message type");
									}
								} else {
									ignorepacket = 1;
									capwap_logging_debug("Retrasmitted packet");
								}
							}

							/* Parsing packet */
							if (!ignorepacket) {
								if (!capwap_parsing_packet(rxmngpacket, &connection, &packet)) {
									/* Validate packet */
									if (capwap_validate_parsed_packet(&packet, NULL)) {
										/* TODO gestione errore risposta */
										ignorepacket = 1;
										capwap_logging_debug("Failed validation parsed packet");
									}
								} else {
									ignorepacket = 1;
									capwap_logging_debug("Failed parsing packet");
								}
							}

							/* */
							if (!ignorepacket && (action == AC_DFA_ACCEPT_PACKET)) {
								action = ac_dfa_execute(session, &packet);
							}

							/* Free memory */
							capwap_free_parsed_packet(&packet);
							ac_free_packet_rxmng(session, isctrlsocket);
						} else if (check != CAPWAP_REQUEST_MORE_FRAGMENT) {
							/* Discard fragments */
							ac_free_packet_rxmng(session, isctrlsocket);
						}
					}
				}
			}
		} else {
			action = ac_dfa_execute(session, NULL);
		}
	}

	/* Release reference session */
	if (!ac_session_release_reference(session)) {
		capwap_logging_debug("Reference session is > 0 to exit thread");
	}
}

/* Change WTP state machine */
void ac_dfa_change_state(struct ac_session_t* session, int state) {
	ASSERT(session != NULL);

	if (state != session->state) {
#ifdef DEBUG
		char sessionname[33];
		capwap_sessionid_printf(&session->sessionid, sessionname);
		capwap_logging_debug("Session AC %s change state from %s to %s", sessionname, capwap_dfa_getname(session->state), capwap_dfa_getname(state));
#endif

		session->state = state;
	}
}

/* Teardown connection */
int ac_session_teardown_connection(struct ac_session_t* session) {
	ASSERT(session != NULL);

	/* Close DTSL Control */
	if (session->ctrldtls.enable) {
		capwap_crypt_close(&session->ctrldtls);
	}

	/* Close DTLS Data */
	if (session->datadtls.enable) {
		capwap_crypt_close(&session->datadtls);
	}

	/* */
	capwap_killall_timeout(&session->timeout);
	capwap_set_timeout(session->dfa.rfcDTLSSessionDelete, &session->timeout, CAPWAP_TIMER_CONTROL_CONNECTION);
	ac_dfa_change_state(session, CAPWAP_DTLS_TEARDOWN_STATE);
	return AC_DFA_DROP_PACKET;
}

/* Release reference of session */
int ac_session_release_reference(struct ac_session_t* session) {
	int remove = 0;
	struct capwap_list_item* search;
	
	ASSERT(session != NULL);

	capwap_lock_enter(&g_ac.sessionslock);

	session->count--;
	if (!session->count) {
		search = g_ac.sessions->first;
		while (search != NULL) {
			struct ac_session_t* item = (struct ac_session_t*)search->item;
			if (session == item) {
				/* Free DTSL Control */
				capwap_crypt_freesession(&session->ctrldtls);

				/* Free DTLS Data */
				capwap_crypt_freesession(&session->datadtls);

				/* Free resource */
				capwap_event_destroy(&session->waitpacket);
				capwap_lock_exit(&session->packetslock);
				capwap_list_free(session->controlpackets);
				capwap_list_free(session->datapackets);

				/* Free fragments packet */
				ac_free_packet_rxmng(session, 1);
				ac_free_packet_rxmng(session, 0);

				capwap_list_free(session->requestfragmentpacket);
				capwap_list_free(session->responsefragmentpacket);

				/* Free DFA resource */
				capwap_array_free(session->dfa.acipv4list.addresses);
				capwap_array_free(session->dfa.acipv6list.addresses);

				/* Remove item from list */
				remove = 1;
				capwap_itemlist_free(capwap_itemlist_remove(g_ac.sessions, search));
				capwap_event_signal(&g_ac.changesessionlist);

				break;
			}

			search = search->next;
		}
	}

	capwap_lock_exit(&g_ac.sessionslock);

	return remove;
}

/* */
void* ac_session_thread(void* param) {
	ASSERT(param != NULL);

	capwap_logging_debug("Session start");
	ac_session_run((struct ac_session_t*)param);
	capwap_logging_debug("Session end");

	/* Thread exit */
	pthread_exit(NULL);
	return NULL;
}

/* */
void ac_get_control_information(struct capwap_list* controllist) {
	struct capwap_list* addrlist;
	struct capwap_list_item* item;

	ASSERT(controllist != NULL);

	/* Detect local address */
	addrlist = capwap_list_create();
	capwap_interface_list(&g_ac.net, addrlist);
	
	/* Prepare control list */
	for (item = addrlist->first; item != NULL; item = item->next) {
		struct capwap_list_item* itemcontrol;
		struct ac_session_control* sessioncontrol;
		struct sockaddr_storage* address = (struct sockaddr_storage*)item->item;

		/* */
		itemcontrol = capwap_itemlist_create(sizeof(struct ac_session_control));
		sessioncontrol = (struct ac_session_control*)itemcontrol->item;
		memcpy(&sessioncontrol->localaddress, address, sizeof(struct sockaddr_storage));
		sessioncontrol->count = 0;
		
		/* Add */
		capwap_itemlist_insert_after(controllist, NULL, itemcontrol);
	}

	/* Free local address list */
	capwap_list_free(addrlist);

	/* */
	capwap_lock_enter(&g_ac.sessionslock);

	/* Get wtp count from any local address */
	for (item = controllist->first; item != NULL; item = item->next) {
		struct capwap_list_item* search;
		struct ac_session_control* sessioncontrol = (struct ac_session_control*)item->item;

		for (search = g_ac.sessions->first; search != NULL; search = search->next) {
			struct ac_session_t* session = (struct ac_session_t*)search->item;

			if (!capwap_compare_ip(&session->acctrladdress, &sessioncontrol->localaddress)) {
				sessioncontrol->count++;
			}
		}
	}

	/* */
	capwap_lock_exit(&g_ac.sessionslock);
}

/* */
void ac_free_reference_last_request(struct ac_session_t* session) {
	ASSERT(session);

	capwap_list_flush(session->requestfragmentpacket);
}

/* */
void ac_free_reference_last_response(struct ac_session_t* session) {
	ASSERT(session);

	capwap_list_flush(session->responsefragmentpacket);
	memset(&session->lastrecvpackethash[0], 0, sizeof(session->lastrecvpackethash));
}
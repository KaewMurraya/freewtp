#ifndef __CAPWAP_ELEMENT_RADIOOPRSTATE_HEADER__
#define __CAPWAP_ELEMENT_RADIOOPRSTATE_HEADER__

#define CAPWAP_ELEMENT_RADIOOPRSTATE		32

#define CAPWAP_RADIO_OPERATIONAL_STATE_ENABLED				1
#define CAPWAP_RADIO_OPERATIONAL_STATE_DISABLED				2

#define CAPWAP_RADIO_OPERATIONAL_CAUSE_NORMAL				0
#define CAPWAP_RADIO_OPERATIONAL_CAUSE_RADIOFAILURE			1
#define CAPWAP_RADIO_OPERATIONAL_CAUSE_SOFTWAREFAILURE		2
#define CAPWAP_RADIO_OPERATIONAL_CAUSE_ADMINSET				3

struct capwap_radiooprstate_element {
	unsigned char radioid;
	unsigned char state;
	unsigned char cause;
};

extern struct capwap_message_elements_ops capwap_element_radiooprstate_ops;

#endif /* __CAPWAP_ELEMENT_RADIOOPRSTATE_HEADER__ */
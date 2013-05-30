#include "capwap.h"
#include "capwap_element.h"

/********************************************************************

 0
 0 1 2 3 4 5 6 7
+-+-+-+-+-+-+-+-+
|Reservd|N|E|L|U|
+-+-+-+-+-+-+-+-+

Type:   41 for WTP Frame Tunnel Mode

Length:   1

********************************************************************/

/* */
static void capwap_wtpframetunnelmode_element_create(void* data, capwap_message_elements_handle handle, struct capwap_write_message_elements_ops* func) {
	struct capwap_wtpframetunnelmode_element* element = (struct capwap_wtpframetunnelmode_element*)data;

	ASSERT(data != NULL);

	/* */
	func->write_u8(handle, element->mode & CAPWAP_WTP_FRAME_TUNNEL_MODE_MASK);
}

/* */
static void* capwap_wtpframetunnelmode_element_parsing(capwap_message_elements_handle handle, struct capwap_read_message_elements_ops* func) {
	struct capwap_wtpframetunnelmode_element* data;

	ASSERT(handle != NULL);
	ASSERT(func != NULL);

	if (func->read_ready(handle) != 1) {
		capwap_logging_debug("Invalid WTP Frame Tunnel Mode element");
		return NULL;
	}

	/* */
	data = (struct capwap_wtpframetunnelmode_element*)capwap_alloc(sizeof(struct capwap_wtpframetunnelmode_element));
	if (!data) {
		capwap_outofmemory();
	}

	/* Retrieve data */
	memset(data, 0, sizeof(struct capwap_wtpframetunnelmode_element));
	func->read_u8(handle, &data->mode);

	return data;
}

/* */
static void capwap_wtpframetunnelmode_element_free(void* data) {
	ASSERT(data != NULL);
	
	capwap_free(data);
}

/* */
struct capwap_message_elements_ops capwap_element_wtpframetunnelmode_ops = {
	.create_message_element = capwap_wtpframetunnelmode_element_create,
	.parsing_message_element = capwap_wtpframetunnelmode_element_parsing,
	.free_parsed_message_element = capwap_wtpframetunnelmode_element_free
};
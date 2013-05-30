#include "capwap.h"
#include "capwap_element.h"

/********************************************************************

 0
 0 1 2 3 4 5 6 7
+-+-+-+-+-+-+-+-+
|  ECN Support  |
+-+-+-+-+-+-+-+-+

Type:   53 for ECN Support

Length:  1

********************************************************************/

/* */
static void capwap_ecnsupport_element_create(void* data, capwap_message_elements_handle handle, struct capwap_write_message_elements_ops* func) {
	struct capwap_ecnsupport_element* element = (struct capwap_ecnsupport_element*)data;

	ASSERT(data != NULL);

	/* */
	func->write_u8(handle, element->flag);
}

/* */
static void* capwap_ecnsupport_element_parsing(capwap_message_elements_handle handle, struct capwap_read_message_elements_ops* func) {
	struct capwap_ecnsupport_element* data;

	ASSERT(handle != NULL);
	ASSERT(func != NULL);

	if (func->read_ready(handle) != 1) {
		capwap_logging_debug("Invalid ECN Support element");
		return NULL;
	}

	/* */
	data = (struct capwap_ecnsupport_element*)capwap_alloc(sizeof(struct capwap_ecnsupport_element));
	if (!data) {
		capwap_outofmemory();
	}

	/* Retrieve data */
	memset(data, 0, sizeof(struct capwap_ecnsupport_element));
	func->read_u8(handle, &data->flag);

	return data;
}

/* */
static void capwap_ecnsupport_element_free(void* data) {
	ASSERT(data != NULL);
	
	capwap_free(data);
}

/* */
struct capwap_message_elements_ops capwap_element_ecnsupport_ops = {
	.create_message_element = capwap_ecnsupport_element_create,
	.parsing_message_element = capwap_ecnsupport_element_parsing,
	.free_parsed_message_element = capwap_ecnsupport_element_free
};
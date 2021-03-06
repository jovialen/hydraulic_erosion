#ifndef __events_event_h__
#define __events_event_h__

#include <stdbool.h>
#include <stdint.h>

#define EVENT_MAX_CALLBACKS__ (32)

typedef enum event_type_t
{
	EVENT_TYPE_CHAR_TYPE,
	EVENT_TYPE_KEY_PRESS,
	EVENT_TYPE_KEY_RELEASE,
	EVENT_TYPE_MOUSE_MOVE,
	EVENT_TYPE_MOUSE_SCROLL,
	EVENT_TYPE_MOUSE_PRESS,
	EVENT_TYPE_MOUSE_RELEASE,
	EVENT_TYPE_WINDOW_CLOSE,
	EVENT_TYPE_WINDOW_RESIZE,
	EVENT_TYPE_COUNT__,
} event_type_t;

typedef enum event_cb_layer_t
{
	EVENT_LAYER_WORLD,
	EVENT_LAYER_UI,
	EVENT_LAYER_APP,
	EVENT_LAYER_COUNT__,
} event_cb_layer_t;

typedef struct event_t event_t;

typedef struct event_bus_t event_bus_t;
typedef bool(*event_callback_fn_t)(event_bus_t *, bool, void *, event_t *);

typedef struct event_bus_desc_t
{
	void *dummy; // windows requires all structs to have at least one member
} event_bus_desc_t;

typedef struct event_bus_t
{
	void *user_pointers[EVENT_TYPE_COUNT__][EVENT_LAYER_COUNT__][EVENT_MAX_CALLBACKS__];
	event_callback_fn_t callbacks[EVENT_TYPE_COUNT__][EVENT_LAYER_COUNT__][EVENT_MAX_CALLBACKS__];
} event_bus_t;

void event_bus_init(const event_bus_desc_t *desc, event_bus_t **event_bus);
event_bus_t *event_bus_create(const event_bus_desc_t *desc);
void event_bus_free(event_bus_t *event_bus);

void event_publish(event_bus_t *bus, event_type_t type, event_t *event);
uint32_t event_subscribe(event_bus_t *bus, event_type_t type, event_cb_layer_t layer, void *user_pointer, event_callback_fn_t callback);
void event_unsubscribe(event_bus_t *bus, event_type_t type, event_cb_layer_t layer, uint32_t id);

#endif /* __events_event_h__ */

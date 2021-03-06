#ifndef __gfx_window_h__
#define __gfx_window_h__

#include <stdbool.h>
#include <stdint.h>

#include <cglm/cglm.h>
#include <glad/glad.h> // this file includes context.h, which includes buffer.h, which includes glad.h, which must be included before glfw3.h. oh boy...
#include <GLFW/glfw3.h>

#include "events/event.h"
#include "math/types.h"
#include "context.h"


typedef struct window_desc_t
{
	const char *title;
	uvec2 size;
	bool resizable;

	uint8_t samples;

	event_bus_t *event_bus;
} window_desc_t;

typedef struct window_t
{
	GLFWwindow *glfw_window;
	context_t *context;
	event_bus_t *event_bus;

	struct
	{
		vec2 last_pos;
	} mouse;
} window_t;

void window_init(const window_desc_t *desc, window_t **window);
window_t *window_create(const window_desc_t *desc);
void window_free(window_t *window);

uvec2 window_get_size(window_t *window);

bool window_process_events(window_t *window);
void window_swap_buffers(window_t *window);

#endif /* __gfx_window_h__ */

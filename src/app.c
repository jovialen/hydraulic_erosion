#include "app.h"

#include <stdio.h>
#include <string.h>

#include <GLFW/glfw3.h>
#include <stb_image.h>

#include "debug/assert.h"
#include "events/mouse_event.h"
#include "events/window_event.h"
#include "gfx/context.h"
#include "gfx/renderer.h"
#include "io/file.h"

typedef struct vertex_t
{
	vec3 position;
	vec3 color;
} vertex_t;

static void init_libs()
{
	HE_VERIFY(glfwInit(), "Failed to initialize GLFW");
	stbi_set_flip_vertically_on_load(true);
}

static void shutdown_libs()
{
	glfwTerminate();
}

static void init_resources(app_state_t *state)
{
	vertex_t vertices[] = {
		{ { -1, -1, -1 }, { 1.0f, 0.0f, 0.0f } },
		{ {  1, -1, -1 }, { 0.0f, 1.0f, 0.0f } },
		{ {  1,  1, -1 }, { 0.0f, 0.0f, 1.0f } },
		{ { -1,  1, -1 }, { 1.0f, 0.0f, 0.0f } },
		{ { -1, -1,  1 }, { 1.0f, 1.0f, 0.0f } },
		{ {  1, -1,  1 }, { 1.0f, 1.0f, 1.0f } },
		{ {  1,  1,  1 }, { 1.0f, 1.0f, 1.0f } },
		{ { -1,  1,  1 }, { 1.0f, 1.0f, 1.0f } },
	};

	int indices[] = {
		0, 1, 3, 3, 1, 2,
		1, 5, 2, 2, 5, 6,
		5, 4, 6, 6, 4, 7,
		4, 0, 7, 7, 0, 3,
		3, 2, 7, 7, 2, 6,
		4, 5, 0, 0, 5, 1,
	};

	mesh_init(&(mesh_desc_t){
		.dynamic = true,
		.vertices = vertices,
		.vertices_size = sizeof(vertices),
		.indices = indices,
		.indices_size = sizeof(indices),
		.index_count = sizeof(indices) / sizeof(indices[0]),
	}, &state->terrain.mesh);

	size_t file_size = 0;
	FILE *file = NULL;

	file = file_open("res/shaders/terrain.vs.glsl");
	file_read(file, &file_size, NULL);
	char *vs_source = calloc(1, file_size + 1);
	file_read(file, &file_size, vs_source);
	vs_source[file_size] = '\0';
	file_close(file);

	file = file_open("res/shaders/terrain.fs.glsl");
	file_read(file, &file_size, NULL);
	char *fs_source = calloc(1, file_size + 1);
	fs_source[file_size] = '\0';
	file_read(file, &file_size, fs_source);
	file_close(file);

	shader_t *vs = shader_create(&(shader_desc_t){
		.type = SHADER_TYPE_VERTEX,
		.source = vs_source,
	});

	shader_t *fs = shader_create(&(shader_desc_t){
		.type = SHADER_TYPE_FRAGMENT,
		.source = fs_source,
	});

	free(vs_source);
	free(fs_source);

	pipeline_init(&(pipeline_desc_t){
		.vs = vs,
		.fs = fs,
		.layout = {
			.location[0] = { .type = ATTRIBUTE_TYPE_FLOAT3, .offset = offsetof(vertex_t, position), },
			.location[1] = { .type = ATTRIBUTE_TYPE_FLOAT3, .offset = offsetof(vertex_t, color),    },
			.stride = sizeof(vertex_t),
		},
		.uniforms = {
			.location[0] = { .type = UNIFORM_TYPE_MAT4, .name = "u_model",      },
			.location[1] = { .type = UNIFORM_TYPE_MAT4, .name = "u_view",       },
			.location[2] = { .type = UNIFORM_TYPE_MAT4, .name = "u_projection", },
		},
		.depth_test = true,
	}, &state->terrain.pipeline);

	shader_free(vs);
	shader_free(fs);
}

static void free_resources(app_state_t *state)
{
	mesh_free(state->terrain.mesh);
	pipeline_free(state->terrain.pipeline);
}

static void on_window_close(event_bus_t *bus, event_type_t type, window_close_event_t *event)
{
	app_state_t *state = (app_state_t *)bus->user_pointer;
	state->running = false;
}

static void on_window_resize(event_bus_t *bus, event_type_t type, window_resize_event_t *event)
{
	context_t *last_ctx = context_bind(event->window->context);
	renderer_set_viewport(0, 0, event->size.w, event->size.h);

	app_state_t *state = (app_state_t *)bus->user_pointer;
	camera_update_projection(&state->camera, window_get_size(event->window));
	pipeline_set_uniform_mat4(state->terrain.pipeline, 2, state->camera.projection);

	context_bind(last_ctx);
}

static void on_mouse_move(event_bus_t *bus, event_type_t type, mouse_move_event_t *event)
{
	app_state_t *state = (app_state_t *)bus->user_pointer;
	camera_move(&state->camera, 0, (vec2){ -event->offset[0] / 10.0f, event->offset[1] / 10.0f });
}

static void on_mouse_scroll(event_bus_t *bus, event_type_t type, mouse_scroll_event_t *event)
{
	app_state_t *state = (app_state_t *)bus->user_pointer;
	camera_move(&state->camera, event->offset[1], GLM_VEC2_ZERO);
}

bool app_init(app_state_t *state)
{
	memset(state, 0, sizeof(state));
	state->running = true;

	init_libs();

	state->event_bus = event_bus_create(&(event_bus_desc_t){
		.user_pointer = state,
	});

	event_subscribe(state->event_bus, EVENT_TYPE_WINDOW_CLOSE, (event_callback_fn_t)on_window_close);
	event_subscribe(state->event_bus, EVENT_TYPE_WINDOW_RESIZE, (event_callback_fn_t)on_window_resize);
	event_subscribe(state->event_bus, EVENT_TYPE_MOUSE_MOVE, (event_callback_fn_t)on_mouse_move);
	event_subscribe(state->event_bus, EVENT_TYPE_MOUSE_SCROLL, (event_callback_fn_t)on_mouse_scroll);

	state->window = window_create(&(window_desc_t){
		.title = APP_NAME,
		.size = { 1280, 720 },
		.resizable = true,
		.samples = 16,
	});
	window_attach_event_bus(state->window, state->event_bus);

	context_bind(state->window->context);

	init_resources(state);

	camera_init(70.0f, -5.0f, window_get_size(state->window), &state->camera);
	pipeline_set_uniform_mat4(state->terrain.pipeline, 2, state->camera.projection);

	return true;
}

void app_run(app_state_t *state)
{
	while (state->running && window_process_events(state->window))
	{
		renderer_clear(&(cmd_clear_desc_t){
			.color = { 1.0f, 1.0f, 1.0f, 1.0f },
			.depth = 1,
		});

		// draw mesh
		pipeline_bind(state->terrain.pipeline);

		mat4 model      = GLM_MAT4_IDENTITY_INIT;

		mat4 view       = GLM_MAT4_IDENTITY_INIT;
		camera_create_view_matrix(&state->camera, view);

		pipeline_set_uniform_mat4(state->terrain.pipeline, 0, model);
		pipeline_set_uniform_mat4(state->terrain.pipeline, 1, view);

		mesh_draw(state->terrain.mesh);

		window_swap_buffers(state->window);
	}
}

void app_shutdown(app_state_t *state)
{
	free_resources(state);
	window_free(state->window);
	shutdown_libs();
}

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

typedef struct vertex_t
{
	vec3 position;
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
		{ -0.5f,  0.5f, 0.0f },
		{ -0.5f, -0.5f, 0.0f },
		{  0.5f,  0.5f, 0.0f },
		{  0.5f, -0.5f, 0.0f },
	};

	int indices[] = {
		0, 1, 2,
		2, 1, 3,
	};

	const char *vs_source =
		"#version 330 core\n"
		"layout(location = 0) in vec3 i_pos;"
		"uniform mat4 u_model;"
		"uniform mat4 u_view;"
		"uniform mat4 u_projection;"
		"void main() {"
		"	gl_Position = u_projection * u_view * u_model * vec4(i_pos, 1.0);"
		"}";

	const char *fs_source =
		"#version 330 core\n"
		"layout(location = 0) out vec4 o_col;"
		"void main() {"
		"	o_col = vec4(0.0);"
		"}";

	mesh_init(&(mesh_desc_t){
		.dynamic = true,
		.vertices = vertices,
		.vertices_size = sizeof(vertices),
		.indices = indices,
		.indices_size = sizeof(indices),
		.index_count = sizeof(indices) / sizeof(indices[0]),
	}, &state->terrain.mesh);

	shader_t *vs = shader_create(&(shader_desc_t){
		.type = SHADER_TYPE_VERTEX,
		.source = vs_source,
	});

	shader_t *fs = shader_create(&(shader_desc_t){
		.type = SHADER_TYPE_FRAGMENT,
		.source = fs_source,
	});

	pipeline_init(&(pipeline_desc_t){
		.vs = vs,
		.fs = fs,
		.layout = {
			.location[0] = { .type = ATTRIBUTE_TYPE_FLOAT3, .offset = offsetof(vertex_t, position), },
			.stride = sizeof(vertex_t),
		},
		.uniforms = {
			.location[0] = { .type = UNIFORM_TYPE_MAT4, .name = "u_model",      },
			.location[1] = { .type = UNIFORM_TYPE_MAT4, .name = "u_view",       },
			.location[2] = { .type = UNIFORM_TYPE_MAT4, .name = "u_projection", },
		},
	}, &state->terrain.pipeline);
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

	state->window = window_create(&(window_desc_t){
		.title = APP_NAME,
		.size = { 1280, 720 },
		.resizable = true,
		.samples = 16,
	});
	window_attach_event_bus(state->window, state->event_bus);

	context_bind(state->window->context);

	init_resources(state);

	camera_update_projection(&state->camera, window_get_size(state->window));
	pipeline_set_uniform_mat4(state->terrain.pipeline, 2, state->camera.projection);

	return true;
}

void app_run(app_state_t *state)
{
	while (state->running && window_process_events(state->window))
	{
		renderer_clear(&(cmd_clear_desc_t){
			.color = { 1.0f, 1.0f, 1.0f, 1.0f },
		});

		// draw mesh
		pipeline_bind(state->terrain.pipeline);

		mat4 model      = GLM_MAT4_IDENTITY_INIT;
		glm_rotate(model, glfwGetTime(), (vec3) { 0.0f, 0.0f, 1.0f });

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

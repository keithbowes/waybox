#include "waybox/server.h"

static void new_input_notify(struct wl_listener *listener, void *data) {
	struct wlr_input_device *device = data;
	struct wb_server *server = wl_container_of(listener, server, new_input);
	switch (device->type) {
		case WLR_INPUT_DEVICE_POINTER:
			wlr_cursor_attach_input_device(server->cursor->cursor, device);
			break;
		default:
			break;
	}
}

bool init_wb(struct wb_server* server) {

    // create display
    server->wl_display = wl_display_create();
	assert(server->wl_display);

    // create shared memory
    wl_display_init_shm(server->wl_display);

    // event loop stuff
    server->wl_event_loop = wl_display_get_event_loop(server->wl_display);
	assert(server->wl_event_loop);

    // create backend
	server->backend = wlr_backend_autocreate(server->wl_display, NULL);
	assert(server->backend);
    if(server->backend == NULL){
        printf("Failed to create backend\n");
        return false;
    }

	server->layout = wlr_output_layout_create();
	server->cursor = wb_cursor_create();
	wlr_cursor_attach_output_layout(server->cursor->cursor, server->layout);

    return true;
}

bool start_wb(struct wb_server* server) {
    wl_list_init(&server->outputs);

	server->new_output.notify = new_output_notify;
	wl_signal_add(&server->backend->events.new_output, &server->new_output);

	server->new_input.notify = new_input_notify;
	wl_signal_add(&server->backend->events.new_input, &server->new_input);

	const char *socket = wl_display_add_socket_auto(server->wl_display);
	assert(socket);

    if (!wlr_backend_start(server->backend)) {
		fprintf(stderr, "Failed to start backend\n");
		wl_display_destroy(server->wl_display);
		return false;
	}

    //printf("Running Wayland compositor on Wayland display '%s'\n", socket);
    //setenv("WAYLAND_DISPLAY", socket, true);

    wlr_gamma_control_manager_v1_create(server->wl_display);
	wlr_screencopy_manager_v1_create(server->wl_display);
#if WLR_VERSION_NUM > 1
	// wlroots 0.2+
	wlr_gtk_primary_selection_device_manager_create(server->wl_display);
#else
	wlr_primary_selection_device_manager_create(server->wl_display);
#endif
	wlr_idle_create(server->wl_display);

	server->compositor = wlr_compositor_create(server->wl_display,
			wlr_backend_get_renderer(server->backend));

	wlr_xdg_shell_create(server->wl_display);
    //wlr_idle_create(server->wl_display);

    return true;
}

bool terminate_wb(struct wb_server* server) {
    wl_display_destroy(server->wl_display);

    printf("Display destroyed.\n");

	wb_cursor_destroy(server->cursor);
	wlr_output_layout_destroy(server->layout);

    return true;
}

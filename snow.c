#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <xcb/xcb.h>

#define MAX_FLAKE_COUNT 500

uint32_t fg_color  = 0;
uint32_t bg_color  = 255;
uint16_t x         = 10;
uint16_t y         = 10;
uint16_t width     = 100;
uint16_t height    = 100;
uint16_t border_px = 1;

int done = 0;

short right = 0;
bool go_right = true;

typedef struct XCBConfig {
    xcb_connection_t    *connection;
    xcb_screen_t        *screen;
    xcb_window_t         window;
    xcb_gcontext_t      *ctx;
    xcb_generic_event_t *event;
    uint32_t             mask;
    uint32_t             values[2];
} XCBConfig;

XCBConfig xcb_config;

time_t start;
time_t now;

int snowflakeCount = 20;

static void init(XCBConfig * config);
static void generate_initial_snowflakes(XCBConfig * config);
static void simulate_snowfall(XCBConfig * config);
static void spawn_flake_at_cursor(xcb_button_press_event_t * btn_press);
static void generate_new_snowflakes(void);
static void handle_events(void);
int msleep(long msec);

xcb_point_t snow[MAX_FLAKE_COUNT];

void terminate(int sigint) {
    fprintf(stderr, "terminating...");

    xcb_disconnect(xcb_config.connection);
    exit(EXIT_SUCCESS);
}

int main() {
  // it's a surprise tool that will help us later
  srand(time(NULL));

  init(&xcb_config);
  generate_initial_snowflakes(&xcb_config);
  
	signal(SIGINT, terminate);

  time(&start);

  while(1) {
    xcb_flush(xcb_config.connection);
    msleep(200);
    generate_new_snowflakes();
    simulate_snowfall(&xcb_config);
    handle_events
  ();
  }

  exit(EXIT_SUCCESS);

    return 0;
}

static void init(XCBConfig * config) {
    config->connection = xcb_connect(NULL, NULL);

    if (xcb_connection_has_error(config->connection)) {
        fprintf(stderr, "Cannot open display\n");
        exit(EXIT_FAILURE);
    }

    config->screen = xcb_setup_roots_iterator(xcb_get_setup(config->connection)).data;

    // create black graphics context
    config->ctx = xcb_generate_id(config->connection);
    config->window = config->screen->root;
    config->mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
    config->values[0] = config->screen->white_pixel;
    config->values[1] = 0;
    xcb_create_gc(config->connection, config->ctx,
                  config->window, config->mask,
                  config->values);

    // create window
    config->window = xcb_generate_id(config->connection);
    config->mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    config->values[0] = config->screen->black_pixel;
    config->values[1] = XCB_EVENT_MASK_EXPOSURE  | XCB_EVENT_MASK_BUTTON_PRESS |
                        XCB_EVENT_MASK_KEY_PRESS;
    xcb_create_window(config->connection, config->screen->root_depth,
                      config->window, config->screen->root,
                      10, 10, 800, 600, 1,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, config->screen->root_visual,
                      config->mask, config->values);

    // map (show) the window
    xcb_map_window(config->connection, config->window);

    xcb_flush(config->connection);


    xcb_map_window(config->connection, config->window);

    xcb_flush(config->connection);

}

static void generate_initial_snowflakes(XCBConfig * config) {
  for (int i = 0; i < 20; i++) {
      int r_x = rand() % (800 + 1 - 0) + 0;
      int r_y = rand() % (60 + 1 - 0) + 0;
      xcb_point_t flake;
      flake.x = r_x;
      flake.y = r_y;
      snow[i] = flake;
  }

  xcb_poly_point(config->connection, XCB_COORD_MODE_ORIGIN,
                 config->window, config->ctx, 20, snow);
}

static void simulate_snowfall(XCBConfig * config) {
  if (right == 5) {
    go_right = false;
  } else if (right == -5) {
    go_right = true;
  }

  if (go_right == true) {
      right++;
  } else {
      right--;
  }

  for (int i = 0; i < MAX_FLAKE_COUNT; i++) {
    if(snow[i].y < 600) {
        snow[i].y++;
    }

    if (go_right == true) {
      snow[i].x++;
    } else if (go_right == false) {
      snow[i].x--;
    }
  }
  xcb_clear_area(xcb_config.connection, 0, xcb_config.window,
                   0,0, 800, 600);
   xcb_poly_point(config->connection, XCB_COORD_MODE_ORIGIN,
                  config->window, config->ctx, MAX_FLAKE_COUNT, snow);
}

int msleep(long msec) {
  struct timespec ts;
  int res;

  if (msec < 0) {
      errno = EINVAL;
      return -1;
  }

  ts.tv_sec = msec / 1000;
  ts.tv_nsec = (msec % 1000) * 1000000;

  do {
      res = nanosleep(&ts, &ts);
  } while (res && errno == EINTR);

  return res;
}

void generate_new_snowflakes(void) {
  time(&now);
  double diff_t = difftime(now, start);

  if (diff_t > 0.255) {
      if (snowflakeCount > MAX_FLAKE_COUNT-1) {
        snowflakeCount = 0;
      }

      int r_x = rand() % (800 + 1 - 0) + 0;
      int r_y = 0;
      xcb_point_t flake;
      flake.x = r_x;
      flake.y = r_y;
      snow[snowflakeCount] = flake;
      snowflakeCount++;
  }

  // update time start
  time(&start);
}

static void handle_events(void) {
  xcb_config.event = xcb_poll_for_event(xcb_config.connection);

  if (!xcb_config.event) {
    return;
  }

  switch (xcb_config.event->response_type & ~0x80) {
    case XCB_BUTTON_PRESS:
      xcb_button_press_event_t * press = (xcb_button_press_event_t * ) xcb_config.event;
      spawn_flake_at_cursor(press);
      break;
    default:
      break;
    }

    free (xcb_config.event);
}

static void spawn_flake_at_cursor(xcb_button_press_event_t * btn_press) {
      if (snowflakeCount > MAX_FLAKE_COUNT-1) {
        snowflakeCount = 0;
      }

      xcb_point_t flake;
      flake.x = btn_press->event_x;
      flake.y = btn_press->event_y;
      snow[snowflakeCount] = flake;
      snowflakeCount++;
}
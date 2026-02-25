#ifndef WIFI_REPL_H
#define WIFI_REPL_H

#include <stddef.h>
#include "pico/util/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_REPL_LINE_MAX 256

typedef void (*wifi_repl_line_cb_t)(const char *line, size_t len);

void wifi_repl_init(wifi_repl_line_cb_t cb, queue_t *error_queue);

void wifi_repl_poll_errors(void);

void wifi_repl_poll(void);

#ifdef __cplusplus
}
#endif

#endif

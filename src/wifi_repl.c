#include "wifi_repl.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/ip4_addr.h"
#include "dhserver.h"

#ifndef WIFI_SSID
#define WIFI_SSID "BadPicoKB"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "badpico1"
#endif

#ifndef REPL_PORT
#define REPL_PORT 4242
#endif

#define AP_IP_ADDR      "192.168.4.1"
#define AP_NETMASK      "255.255.255.0"
#define AP_DHCP_START   "192.168.4.2"

typedef struct repl_client {
    struct tcp_pcb *pcb;
    char buf[WIFI_REPL_LINE_MAX];
    size_t buf_len;
} repl_client_t;

static wifi_repl_line_cb_t s_line_cb = NULL;

static dhcp_entry_t s_dhcp_entries[] = {
    { {0}, {192, 168, 4, 2}, {255, 255, 255, 0}, 24 * 60 * 60 },
    { {0}, {192, 168, 4, 3}, {255, 255, 255, 0}, 24 * 60 * 60 },
    { {0}, {192, 168, 4, 4}, {255, 255, 255, 0}, 24 * 60 * 60 },
};

static dhcp_config_t s_dhcp_config = {
    .addr    = {192, 168, 4, 1},
    .port    = 67,
    .dns     = {192, 168, 4, 1},
    .domain  = "local",
    .num_entry = 3,
    .entries = s_dhcp_entries,
};

static void repl_client_close(repl_client_t *client) {
    if (client->pcb) {
        tcp_arg(client->pcb, NULL);
        tcp_recv(client->pcb, NULL);
        tcp_err(client->pcb, NULL);
        tcp_close(client->pcb);
        client->pcb = NULL;
    }
    free(client);
}

static err_t repl_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    repl_client_t *client = (repl_client_t *)arg;

    if (!p || err != ERR_OK) {
        repl_client_close(client);
        return ERR_OK;
    }

    struct pbuf *q = p;
    while (q) {
        const char *data = (const char *)q->payload;
        uint16_t len = q->len;

        for (uint16_t i = 0; i < len; ++i) {
            char c = data[i];
            if (c == '\r') {
                continue;
            }
            if (c == '\n') {
                if (client->buf_len > 0 && s_line_cb) {
                    client->buf[client->buf_len] = '\0';
                    s_line_cb(client->buf, client->buf_len);
                    const char *prompt = "> ";
                    tcp_write(tpcb, prompt, 2, TCP_WRITE_FLAG_COPY);
                }
                client->buf_len = 0;
            } else if (c == '\\' && i + 1 < len && data[i + 1] == 't') {
                if (client->buf_len < WIFI_REPL_LINE_MAX - 1) {
                    client->buf[client->buf_len++] = '\t';
                }
                ++i;
            } else if (c == '\\' && i + 1 < len && data[i + 1] == 'n') {
                if (client->buf_len < WIFI_REPL_LINE_MAX - 1) {
                    client->buf[client->buf_len++] = '\n';
                }
                ++i;
            } else {
                if (client->buf_len < WIFI_REPL_LINE_MAX - 1) {
                    client->buf[client->buf_len++] = c;
                }
            }
        }
        q = q->next;
    }

    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

static void repl_client_err(void *arg, err_t err) {
    (void)err;
    repl_client_t *client = (repl_client_t *)arg;
    if (client) {
        client->pcb = NULL;
        free(client);
    }
}

static err_t repl_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    (void)arg;
    if (err != ERR_OK || newpcb == NULL) {
        return ERR_VAL;
    }

    repl_client_t *client = calloc(1, sizeof(repl_client_t));
    if (!client) {
        tcp_close(newpcb);
        return ERR_MEM;
    }

    client->pcb = newpcb;
    client->buf_len = 0;

    tcp_arg(newpcb, client);
    tcp_recv(newpcb, repl_client_recv);
    tcp_err(newpcb, repl_client_err);

    const char *banner = "Bad Pico KB - type a line and press Enter to send as keypresses\r\n> ";
    tcp_write(newpcb, banner, (u16_t)strlen(banner), TCP_WRITE_FLAG_COPY);

    return ERR_OK;
}

void wifi_repl_init(wifi_repl_line_cb_t cb) {
    s_line_cb = cb;

    if (cyw43_arch_init()) {
        printf("wifi_repl: cyw43_arch_init failed\n");
        return;
    }

    cyw43_arch_enable_ap_mode(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);

    dhserv_init(&s_dhcp_config);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        printf("wifi_repl: tcp_new failed\n");
        return;
    }

    if (tcp_bind(pcb, IP_ANY_TYPE, REPL_PORT) != ERR_OK) {
        printf("wifi_repl: tcp_bind failed\n");
        return;
    }

    struct tcp_pcb *listen_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!listen_pcb) {
        printf("wifi_repl: tcp_listen failed\n");
        tcp_close(pcb);
        return;
    }

    tcp_accept(listen_pcb, repl_server_accept);

    printf("wifi_repl: AP \"%s\" up, REPL on port %d\n", WIFI_SSID, REPL_PORT);
}

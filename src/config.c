#define _GNU_SOURCE
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void trim(char *s) {
    if (!s) return;

    // left trim
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);

    // right trim
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n-1])) {
        s[n-1] = '\0';
        n--;
    }
}

static int parse_bool(const char *v, int *ok) {
    *ok = 1;
    if (!v) { *ok = 0; return 0; }

    if (!strcasecmp(v, "1") || !strcasecmp(v, "true") || !strcasecmp(v, "yes") || !strcasecmp(v, "on"))
        return 1;
    if (!strcasecmp(v, "0") || !strcasecmp(v, "false") || !strcasecmp(v, "no") || !strcasecmp(v, "off"))
        return 0;

    *ok = 0;
    return 0;
}

Config config_defaults(void) {
    Config c;
    c.colors = true;
    c.minimal = false;

    c.show_os = true;
    c.show_kernel = true;
    c.show_uptime = true;
    c.show_cpu = true;
    c.show_gpu = true;
    c.show_ram = true;
    c.show_disk = true;
    c.show_packages = true;
    c.show_shell = true;
    c.show_terminal = true;
    return c;
}

static void set_key(Config *c, const char *k, const char *v) {
    int ok = 0;
    int b = parse_bool(v, &ok);

    if (!strcasecmp(k, "colors") && ok) c->colors = b;
    else if (!strcasecmp(k, "minimal") && ok) c->minimal = b;
    else if (!strcasecmp(k, "show_os") && ok) c->show_os = b;
    else if (!strcasecmp(k, "show_kernel") && ok) c->show_kernel = b;
    else if (!strcasecmp(k, "show_uptime") && ok) c->show_uptime = b;
    else if (!strcasecmp(k, "show_cpu") && ok) c->show_cpu = b;
    else if (!strcasecmp(k, "show_gpu") && ok) c->show_gpu = b;
    else if (!strcasecmp(k, "show_ram") && ok) c->show_ram = b;
    else if (!strcasecmp(k, "show_disk") && ok) c->show_disk = b;
    else if (!strcasecmp(k, "show_packages") && ok) c->show_packages = b;
    else if (!strcasecmp(k, "show_shell") && ok) c->show_shell = b;
    else if (!strcasecmp(k, "show_terminal") && ok) c->show_terminal = b;
}

void config_load(Config *cfg) {
    const char *home = getenv("HOME");
    if (!home || !*home) return;

    char path[512];
    snprintf(path, sizeof(path), "%s/.config/newofetch/config.conf", home);

    FILE *f = fopen(path, "r");
    if (!f) return;

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        // strip newline
        line[strcspn(line, "\r\n")] = 0;

        // comments / empty
        char *hash = strchr(line, '#');
        if (hash) *hash = '\0';
        trim(line);
        if (!*line) continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';

        char *k = line;
        char *v = eq + 1;

        trim(k);
        trim(v);
        if (!*k) continue;

        set_key(cfg, k, v);
    }

    fclose(f);
}

static int arg_is(const char *a, const char *b) {
    return a && b && strcmp(a, b) == 0;
}

void config_apply_cli(Config *cfg, int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];

        if (arg_is(a, "--colors")) cfg->colors = true;
        else if (arg_is(a, "--no-colors")) cfg->colors = false;

        else if (arg_is(a, "--minimal")) cfg->minimal = true;

        else if (arg_is(a, "--no-os")) cfg->show_os = false;
        else if (arg_is(a, "--no-kernel")) cfg->show_kernel = false;
        else if (arg_is(a, "--no-uptime")) cfg->show_uptime = false;
        else if (arg_is(a, "--no-cpu")) cfg->show_cpu = false;
        else if (arg_is(a, "--no-gpu")) cfg->show_gpu = false;
        else if (arg_is(a, "--no-ram")) cfg->show_ram = false;
        else if (arg_is(a, "--no-disk")) cfg->show_disk = false;
        else if (arg_is(a, "--no-pkgs") || arg_is(a, "--no-packages")) cfg->show_packages = false;
        else if (arg_is(a, "--no-shell")) cfg->show_shell = false;
        else if (arg_is(a, "--no-terminal")) cfg->show_terminal = false;
    }

    if (cfg->minimal) {
        // Minimal = only the core essentials
        cfg->show_shell = false;
        cfg->show_terminal = false;
        cfg->show_disk = false;
        cfg->show_packages = false;
    }
}

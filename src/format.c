#include "format.h"
#include <stdio.h>
#include <string.h>

static const char *color_for_distro(const char *id) {
    if (!id) return "\033[1;36m"; // default cyan

    if (!strcmp(id, "ubuntu")) return "\033[1;33m";     // orange-ish
    if (!strcmp(id, "debian")) return "\033[1;35m";     // pink
    if (!strcmp(id, "arch")) return "\033[1;34m";       // blue
    if (!strcmp(id, "fedora")) return "\033[1;34m";     // blue
    if (!strcmp(id, "manjaro")) return "\033[1;32m";    // green
    if (!strcmp(id, "mint")) return "\033[1;32m";       // green
    if (!strcmp(id, "opensuse")) return "\033[1;32m";   // green

    return "\033[1;36m"; // fallback cyan
}

Colors colors_make(bool enabled, const char *distro_id) {
    Colors c;
    c.enabled = enabled;
    c.reset  = enabled ? "\033[0m" : "";
    c.value  = enabled ? "\033[0;37m" : "";

    if (!enabled) {
        c.label = "";
        c.accent = "";
        return c;
    }

    const char *distro_color = color_for_distro(distro_id);

    c.label  = distro_color;
    c.accent = distro_color;

    return c;
}

void print_kv(const Colors *c, const char *key, const char *value) {
    if (!value) value = "unknown";
    printf("%s%-9s%s: %s%s%s\n",
           c->label, key, c->reset,
           c->value, value, c->reset);
}

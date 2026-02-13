#pragma once
#include <stdbool.h>

typedef struct {
    bool enabled;
    const char *label;
    const char *value;
    const char *accent;
    const char *reset;
} Colors;

Colors colors_make(bool enabled, const char *distro_id);

void print_kv(const Colors *c, const char *key, const char *value);

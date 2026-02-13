#pragma once
#include <stdbool.h>

typedef struct {
    bool colors;
    bool minimal;

    bool show_os;
    bool show_kernel;
    bool show_uptime;
    bool show_cpu;
    bool show_gpu;
    bool show_ram;
    bool show_disk;
    bool show_packages;
    bool show_shell;
    bool show_terminal;
} Config;

Config config_defaults(void);
void config_load(Config *cfg);                 // loads ~/.config/newofetch/config.conf if exists
void config_apply_cli(Config *cfg, int argc, char **argv);

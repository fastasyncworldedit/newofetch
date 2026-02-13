#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "sysinfo.h"
#include "format.h"

static const char *safe_env(const char *k) {
    const char *v = getenv(k);
    return (v && *v) ? v : "unknown";
}

int main(int argc, char **argv) {
    Config cfg = config_defaults();
    config_load(&cfg);
    config_apply_cli(&cfg, argc, argv);

    char distro_id[128];
    si_distro_id(distro_id, sizeof(distro_id));

    Colors c = colors_make(cfg.colors, distro_id);


    char host[256], os[256], kernel[256], uptime[256], cpu[512], gpu[256], ram[256], disk[256], pkgs[256];

    si_hostname(host, sizeof(host));

    // Header
    printf("\n%s%s@%s%s\n", c.accent, safe_env("USER"), host, c.reset);

    if (cfg.show_os) {
        si_os_pretty(os, sizeof(os));
        print_kv(&c, "OS", os);
    }
    if (cfg.show_kernel) {
        si_kernel(kernel, sizeof(kernel));
        print_kv(&c, "Kernel", kernel);
    }
    if (cfg.show_uptime) {
        si_uptime(uptime, sizeof(uptime));
        print_kv(&c, "Uptime", uptime);
    }
    if (cfg.show_cpu) {
        si_cpu(cpu, sizeof(cpu));
        print_kv(&c, "CPU", cpu);
    }
    if (cfg.show_gpu) {
        si_gpu(gpu, sizeof(gpu));
        print_kv(&c, "GPU", gpu);
    }
    if (cfg.show_ram) {
        si_ram(ram, sizeof(ram));
        print_kv(&c, "RAM", ram);
    }
    if (cfg.show_disk) {
        si_disk_root(disk, sizeof(disk));
        print_kv(&c, "Disk (/)", disk);
    }
    if (cfg.show_packages) {
        si_packages(pkgs, sizeof(pkgs));
        print_kv(&c, "Packages", pkgs);
    }
    if (cfg.show_shell) {
        print_kv(&c, "Shell", safe_env("SHELL"));
    }
    if (cfg.show_terminal) {
        print_kv(&c, "Terminal", safe_env("TERM"));
    }

    printf("\n");
    return 0;
}

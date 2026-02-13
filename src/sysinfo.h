#pragma once
#include <stddef.h>

void si_hostname(char *out, size_t out_sz);
void si_os_pretty(char *out, size_t out_sz);
void si_kernel(char *out, size_t out_sz);
void si_uptime(char *out, size_t out_sz);
void si_cpu(char *out, size_t out_sz);
void si_ram(char *out, size_t out_sz);
void si_disk_root(char *out, size_t out_sz);
void si_gpu(char *out, size_t out_sz);
void si_packages(char *out, size_t out_sz);
void si_distro_id(char *out, size_t out_sz);

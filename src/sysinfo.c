#define _GNU_SOURCE
#include "sysinfo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <dirent.h>

#define BUF 4096

static void rstrip_newline(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n && (s[n-1] == '\n' || s[n-1] == '\r')) {
        s[n-1] = '\0';
        n--;
    }
}

static int read_first_matching_kv(const char *path, const char *key, char *out, size_t out_sz) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[BUF];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, key, strlen(key)) == 0 && line[strlen(key)] == '=') {
            char *val = line + strlen(key) + 1;
            rstrip_newline(val);

            if (val[0] == '"' || val[0] == '\'') {
                char q = val[0];
                val++;
                char *end = strrchr(val, q);
                if (end) *end = '\0';
            }

            snprintf(out, out_sz, "%s", val);
            fclose(f);
            return 0;
        }
    }

    fclose(f);
    return -1;
}

void si_hostname(char *out, size_t out_sz) {
    if (gethostname(out, out_sz) != 0)
        snprintf(out, out_sz, "unknown");
    out[out_sz - 1] = '\0';
}

void si_os_pretty(char *out, size_t out_sz) {
    if (read_first_matching_kv("/etc/os-release", "PRETTY_NAME", out, out_sz) == 0)
        return;
    snprintf(out, out_sz, "Linux");
}

void si_kernel(char *out, size_t out_sz) {
    struct utsname u;
    if (uname(&u) == 0)
        snprintf(out, out_sz, "%s %s", u.sysname, u.release);
    else
        snprintf(out, out_sz, "unknown");
}

void si_uptime(char *out, size_t out_sz) {
    FILE *f = fopen("/proc/uptime", "r");
    if (!f) {
        snprintf(out, out_sz, "unknown");
        return;
    }

    double up = 0.0;
    if (fscanf(f, "%lf", &up) != 1) {
        fclose(f);
        snprintf(out, out_sz, "unknown");
        return;
    }
    fclose(f);

    long secs = (long)up;
    long days = secs / 86400; secs %= 86400;
    long hrs  = secs / 3600;  secs %= 3600;
    long mins = secs / 60;

    if (days > 0) snprintf(out, out_sz, "%ldd %ldh %ldm", days, hrs, mins);
    else if (hrs > 0) snprintf(out, out_sz, "%ldh %ldm", hrs, mins);
    else snprintf(out, out_sz, "%ldm", mins);
}

void si_distro_id(char *out, size_t out_sz) {
    FILE *f = fopen("/etc/os-release", "r");
    if (!f) {
        snprintf(out, out_sz, "unknown");
        return;
    }

    char line[BUF];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "ID=", 3) == 0) {
            char *val = line + 3;
            rstrip_newline(val);

            if (val[0] == '"' || val[0] == '\'') {
                char q = val[0];
                val++;
                char *end = strrchr(val, q);
                if (end) *end = '\0';
            }

            snprintf(out, out_sz, "%s", val);
            fclose(f);
            return;
        }
    }

    fclose(f);
    snprintf(out, out_sz, "unknown");
}


void si_cpu(char *out, size_t out_sz) {
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (!f) { snprintf(out, out_sz, "unknown"); return; }

    char line[BUF];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "model name", 10) == 0) {
            char *colon = strchr(line, ':');
            if (!colon) break;
            colon++;
            while (*colon == ' ' || *colon == '\t') colon++;
            rstrip_newline(colon);
            snprintf(out, out_sz, "%s", colon);
            fclose(f);
            return;
        }
        if (strncmp(line, "Hardware", 8) == 0) { // ARM fallback
            char *colon = strchr(line, ':');
            if (!colon) break;
            colon++;
            while (*colon == ' ' || *colon == '\t') colon++;
            rstrip_newline(colon);
            snprintf(out, out_sz, "%s", colon);
            fclose(f);
            return;
        }
    }

    fclose(f);
    snprintf(out, out_sz, "unknown");
}

static long read_mem_kb(const char *key) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return -1;

    char line[BUF];
    long kb = -1;
    size_t klen = strlen(key);

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, key, klen) == 0) {
            char *p = line + klen;
            while (*p && (*p < '0' || *p > '9')) p++;
            kb = strtol(p, NULL, 10);
            break;
        }
    }

    fclose(f);
    return kb;
}

void si_ram(char *out, size_t out_sz) {
    long total = read_mem_kb("MemTotal");
    long avail = read_mem_kb("MemAvailable");
    if (total <= 0 || avail < 0) { snprintf(out, out_sz, "unknown"); return; }

    long used = total - avail;
    double total_gib = (double)total / 1024.0 / 1024.0;
    double used_gib  = (double)used  / 1024.0 / 1024.0;

    snprintf(out, out_sz, "%.2f GiB used / %.2f GiB total", used_gib, total_gib);
}

void si_disk_root(char *out, size_t out_sz) {
    struct statvfs vfs;
    if (statvfs("/", &vfs) != 0) { snprintf(out, out_sz, "unknown"); return; }

    unsigned long long total = (unsigned long long)vfs.f_frsize * (unsigned long long)vfs.f_blocks;
    unsigned long long freeb = (unsigned long long)vfs.f_frsize * (unsigned long long)vfs.f_bfree;
    unsigned long long used  = total - freeb;

    double total_gib = (double)total / 1024.0 / 1024.0 / 1024.0;
    double used_gib  = (double)used  / 1024.0 / 1024.0 / 1024.0;

    snprintf(out, out_sz, "%.2f GiB used / %.2f GiB total", used_gib, total_gib);
}

void si_gpu(char *out, size_t out_sz) {
    DIR *dir = opendir("/sys/class/drm");
    if (!dir) { snprintf(out, out_sz, "unknown"); return; }

    struct dirent *e;
    char driver[256] = {0};

    while ((e = readdir(dir))) {
        if (strncmp(e->d_name, "card", 4) != 0) continue;
        if (strchr(e->d_name, '-')) continue; // skip connectors

        char path[256];
        snprintf(path, sizeof(path), "/sys/class/drm/%s/device/uevent", e->d_name);

        FILE *f = fopen(path, "r");
        if (!f) continue;

        char line[BUF];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "DRIVER=", 7) == 0) {
                char *d = line + 7;
                rstrip_newline(d);
                snprintf(driver, sizeof(driver), "%s", d);
                break;
            }
        }
        fclose(f);

        if (driver[0]) break;
    }

    closedir(dir);

    if (!driver[0]) { snprintf(out, out_sz, "unknown"); return; }

    // small friendly mapping (still tiny)
    if (!strcmp(driver, "amdgpu")) snprintf(out, out_sz, "AMD (amdgpu)");
    else if (!strcmp(driver, "i915")) snprintf(out, out_sz, "Intel (i915)");
    else if (!strcmp(driver, "nouveau")) snprintf(out, out_sz, "NVIDIA (nouveau)");
    else if (!strcmp(driver, "nvidia")) snprintf(out, out_sz, "NVIDIA (nvidia)");
    else snprintf(out, out_sz, "%s", driver);
}

static int count_lines_prefix(const char *file, const char *prefix) {
    FILE *f = fopen(file, "r");
    if (!f) return -1;

    int count = 0;
    char line[BUF];
    size_t plen = strlen(prefix);

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, prefix, plen) == 0) count++;
    }

    fclose(f);
    return count;
}

static int count_dir_entries(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return -1;

    int count = 0;
    struct dirent *e;
    while ((e = readdir(dir))) {
        if (e->d_name[0] != '.') count++;
    }
    closedir(dir);
    return count;
}

void si_packages(char *out, size_t out_sz) {
    int c;

    c = count_lines_prefix("/var/lib/dpkg/status", "Package:");
    if (c > 0) { snprintf(out, out_sz, "%d (dpkg)", c); return; }

    c = count_dir_entries("/var/lib/pacman/local");
    if (c > 0) { snprintf(out, out_sz, "%d (pacman)", c); return; }

    // rpm detection by directory existence is rough but tiny; enough for RC2
    c = count_dir_entries("/var/lib/rpm");
    if (c > 0) { snprintf(out, out_sz, "rpm-based"); return; }

    snprintf(out, out_sz, "unknown");
}

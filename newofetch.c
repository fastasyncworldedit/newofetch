#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <dirent.h>

#define BUF 4096

/* ---------- helpers ---------- */

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

/* ---------- system info ---------- */

static void get_hostname(char *out, size_t out_sz) {
    if (gethostname(out, out_sz) != 0)
        snprintf(out, out_sz, "unknown");
}

static void get_os_pretty(char *out, size_t out_sz) {
    if (read_first_matching_kv("/etc/os-release", "PRETTY_NAME", out, out_sz) == 0)
        return;
    snprintf(out, out_sz, "Linux");
}

static void get_kernel(char *out, size_t out_sz) {
    struct utsname u;
    if (uname(&u) == 0)
        snprintf(out, out_sz, "%s %s", u.sysname, u.release);
    else
        snprintf(out, out_sz, "unknown");
}

static void get_uptime(char *out, size_t out_sz) {
    FILE *f = fopen("/proc/uptime", "r");
    if (!f) {
        snprintf(out, out_sz, "unknown");
        return;
    }

    double up;
    fscanf(f, "%lf", &up);
    fclose(f);

    long secs = (long)up;
    long days = secs / 86400; secs %= 86400;
    long hrs  = secs / 3600;  secs %= 3600;
    long mins = secs / 60;

    if (days > 0)
        snprintf(out, out_sz, "%ldd %ldh %ldm", days, hrs, mins);
    else if (hrs > 0)
        snprintf(out, out_sz, "%ldh %ldm", hrs, mins);
    else
        snprintf(out, out_sz, "%ldm", mins);
}

static void get_cpu(char *out, size_t out_sz) {
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (!f) {
        snprintf(out, out_sz, "unknown");
        return;
    }

    char line[BUF];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "model name", 10) == 0) {
            char *colon = strchr(line, ':');
            colon += 2;
            rstrip_newline(colon);
            snprintf(out, out_sz, "%s", colon);
            fclose(f);
            return;
        }
    }

    fclose(f);
    snprintf(out, out_sz, "unknown");
}

/* ---------- RAM ---------- */

static long read_mem_kb(const char *key) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return -1;

    char line[BUF];
    long kb = -1;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, key, strlen(key)) == 0) {
            sscanf(line, "%*s %ld", &kb);
            break;
        }
    }

    fclose(f);
    return kb;
}

static void get_ram(char *out, size_t out_sz) {
    long total = read_mem_kb("MemTotal");
    long avail = read_mem_kb("MemAvailable");
    long used = total - avail;

    snprintf(out, out_sz, "%.2f / %.2f GiB",
        used/1024.0/1024.0,
        total/1024.0/1024.0);
}

/* ---------- DISK ---------- */

static void get_disk_root(char *out, size_t out_sz) {
    struct statvfs vfs;
    if (statvfs("/", &vfs) != 0) {
        snprintf(out, out_sz, "unknown");
        return;
    }

    unsigned long long total = vfs.f_blocks * vfs.f_frsize;
    unsigned long long freeb = vfs.f_bfree * vfs.f_frsize;
    unsigned long long used = total - freeb;

    snprintf(out, out_sz, "%.2f / %.2f GiB",
        used/1024.0/1024.0/1024.0,
        total/1024.0/1024.0/1024.0);
}

/* ---------- GPU ---------- */

static void get_gpu(char *out, size_t out_sz) {
    DIR *dir = opendir("/sys/class/drm");
    if (!dir) {
        snprintf(out, out_sz, "unknown");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (strncmp(entry->d_name, "card", 4) == 0) {
            char path[256];
            snprintf(path, sizeof(path),
                     "/sys/class/drm/%s/device/uevent", entry->d_name);

            FILE *f = fopen(path, "r");
            if (!f) continue;

            char line[BUF];
            while (fgets(line, sizeof(line), f)) {
                if (strncmp(line, "DRIVER=", 7) == 0) {
                    char *drv = line + 7;
                    rstrip_newline(drv);
                    snprintf(out, out_sz, "%s", drv);
                    fclose(f);
                    closedir(dir);
                    return;
                }
            }
            fclose(f);
        }
    }

    closedir(dir);
    snprintf(out, out_sz, "unknown");
}

/* ---------- PACKAGE COUNT ---------- */

static int count_lines_with_prefix(const char *file, const char *prefix) {
    FILE *f = fopen(file, "r");
    if (!f) return -1;

    int count = 0;
    char line[BUF];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, prefix, strlen(prefix)) == 0)
            count++;
    }

    fclose(f);
    return count;
}

static int count_dir_entries(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return -1;

    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] != '.')
            count++;
    }

    closedir(dir);
    return count;
}

static void get_packages(char *out, size_t out_sz) {
    int count;

    // dpkg / apt
    count = count_lines_with_prefix("/var/lib/dpkg/status", "Package:");
    if (count > 0) {
        snprintf(out, out_sz, "%d (dpkg)", count);
        return;
    }

    // pacman
    count = count_dir_entries("/var/lib/pacman/local");
    if (count > 0) {
        snprintf(out, out_sz, "%d (pacman)", count);
        return;
    }

    // rpm / dnf
    count = count_dir_entries("/var/lib/rpm");
    if (count > 0) {
        snprintf(out, out_sz, "%d (rpm)", count);
        return;
    }

    snprintf(out, out_sz, "unknown");
}

/* ---------- MAIN ---------- */

int main() {
    char host[256], os[256], kernel[256], uptime[256], cpu[512], ram[256], disk[256], gpu[256], pkgs[256];

    get_hostname(host, sizeof(host));
    get_os_pretty(os, sizeof(os));
    get_kernel(kernel, sizeof(kernel));
    get_uptime(uptime, sizeof(uptime));
    get_cpu(cpu, sizeof(cpu));
    get_ram(ram, sizeof(ram));
    get_disk_root(disk, sizeof(disk));
    get_gpu(gpu, sizeof(gpu));
    get_packages(pkgs, sizeof(pkgs));

    printf("\n");
    printf("User: %s@%s\n", getenv("USER"), host);
    printf("OS: %s\n", os);
    printf("Kernel: %s\n", kernel);
    printf("Uptime: %s\n", uptime);
    printf("CPU: %s\n", cpu);
    printf("GPU: %s\n", gpu);
    printf("RAM: %s\n", ram);
    printf("Disk (/): %s\n", disk);
    printf("Packages: %s\n", pkgs);
    printf("Shell: %s\n", getenv("SHELL"));
    printf("Terminal: %s\n", getenv("TERM"));
    printf("\n");

    return 0;
}

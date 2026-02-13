# newofetch
### now maintained by @unfamiliardev
Minimal, fast system information tool written in C.  
A simple Linux CLI utility inspired by neofetch, focused on speed and zero dependencies.

## Features

- OS and kernel info
- Hostname and user
- Uptime
- CPU detection
- GPU detection (via `/sys`)
- RAM usage
- Disk usage
- Installed package count (dpkg / pacman / rpm)
- Shell and terminal detection

No external tools required. Pure C.

## Build

Requirements:
- Linux
- gcc

Compile:

```bash
gcc -O2 newofetch.c -o newofetch
````

Run:

```bash
./newofetch
```

## Example output

```
User: matt@heropc
OS: Arch Linux
Kernel: Linux 6.7.4
Uptime: 1d 3h 12m
CPU: Intel i5-2400
GPU: amdgpu
RAM: 3.21 / 7.78 GiB
Disk (/): 54.12 / 223.57 GiB
Packages: 812 (pacman)
Shell: /bin/bash
Terminal: xterm-256color
```

## Roadmap

* colors
* CLI flags
* distro logos
* config file
* package managers expansion
* performance improvements

## Why

This project exists as a lightweight alternative and as a learning project for low-level Linux system inspection in C.

## License

This project is licensed under the GNU General Public License v3.0 (GPL-3.0).

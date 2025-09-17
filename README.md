# simpleoverlay
<p float="left">
<img width=49% src="https://github.com/user-attachments/assets/72fba10a-405c-4fac-8029-e3acbd8ba651" />
<img width=49% src="https://github.com/user-attachments/assets/47d7aa6e-a660-45bc-9349-d32becd31b07" />
<img width=49% src="https://github.com/user-attachments/assets/9b54f96c-4ca5-47bc-90d2-d1a9e1543538" />
<img width=49% src="https://github.com/user-attachments/assets/4a802f9d-b56a-4fdd-90f3-c6225955df40" />
</p>

<p align="center">OpenGL performance overlay for Linux, macOS, *BSD and Illumos</p>

## Key features

- only build-time dependency is a C99 compliant compiler
- supports legacy OpenGL (1.1), making it compatible with a wide variety of old GPUs
- made with cross-platform support in mind, currently works on 7 different operating systems

## Configuration

simpleoverlay can be configured with the following environment variables

- SCALE
- HORIZONTAL_POS
- VERTICAL_POS
- BG_COLOR
- KEY_COLOR
- VALUE_COLOR
- METRICS
- NO_GRAPH
- FPS_ONLY

You can modify them in the `simpleoverlay` wrapper file

## Installing

```
git clone https://github.com/tranarchy/simpleoverlay
cd simpleoverlay
./build.sh
sudo ./build.sh install
```

## 32-bit support

For 32-bit game support (such as HL2) you will need to run

```
./build.sh multilib
sudo ./build.sh install_multilib
```

You will need a compiler that can compile for 32-bit, on Linux you can install `gcc-multilib`

You will also need to uncomment the line in `simpleoverlay` that adds the 32-bit file to LD_PRELOAD


## Usage

```
simpleoverlay glxgears
```

For Steam games

```
simpleoverlay %command%
```

## FreeBSD

To get CPU temp on FreeBSD you need to load either coretemp (for Intel) or amdtemp (for AMD)

```
sysrc -f /etc/rc.conf kld_list+=coretemp
service kld restart
```

<br>

> [!NOTE]
> Currently GPU information is limited to AMD, NVIDIA and Apple GPUs

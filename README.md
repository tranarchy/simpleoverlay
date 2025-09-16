# simpleoverlay
<p float="left">
<img width=49% src="https://github.com/user-attachments/assets/ffdd6714-a7b4-4689-8fb4-598e8ce1d9b9" />
<img width=49% src="https://github.com/user-attachments/assets/7f44352e-4a34-4d20-9dd6-d6b5c47f9cbd" />
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
> Currently GPU information is limited to AMD and Apple GPUs

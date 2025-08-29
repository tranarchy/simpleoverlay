# simpleoverlay
<p float="left">
<img width=49% src="https://github.com/user-attachments/assets/67ea83b1-2820-43c6-912a-5fa07afcddf5" />
<img width=49% src="https://github.com/user-attachments/assets/2dde7d71-bee5-4435-b491-e122a9052467" />
</p>

<p align="center">OpenGL performance overlay for Linux and *BSD</p>

## About

simpleoverlay is an OpenGL performance overlay for Linux and *BSD written in C

It supports the GLX and EGL OpenGL interface

## Configuration

simpleoverlay can be configured with the following environment variables

- SCALE
- KEY_COLOR
- VALUE_COLOR
- FPS_ONLY

You can modify them in the `simpleoverlay` wrapper file

## Build-time dependencies
- C compiler
- GNU make
- pkg-config
- libdrm-devel
- libglvnd-devel

## Installing

```
git clone https://github.com/tranarchy/simpleoverlay
cd simpleoverlay
make
sudo make install
```

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
kldload coretemp amdtemp
```

<br>

> [!NOTE]
> Currently GPU information is limited to AMD GPUs
>
> Other parts of the overlay will still work fine for non AMD GPU users

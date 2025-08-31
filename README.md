# simpleoverlay
<p float="left">
<img width=49% src="https://github.com/user-attachments/assets/73264ec0-33c2-45a9-a156-93366fc63114" />
<img width=49% src="https://github.com/user-attachments/assets/a3011d14-7c7e-4c8a-9a36-420ceed5b45a" />
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

To support 32-bit games (such as HL2) you will need to run

```
make multilib
sudo make install_multilib
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
kldload coretemp amdtemp
```

<br>

> [!NOTE]
> Currently GPU information is limited to AMD GPUs
>
> Other parts of the overlay will still work fine for non AMD GPU users

# simpleoverlay
<p float="left">
<img width=49% src="https://github.com/user-attachments/assets/96665846-9989-4bf7-a4ba-b6e8bda1ff72" />
<img width=49% src="https://github.com/user-attachments/assets/0c1bcac4-fa1c-44ab-aa32-97ec810f5040" />
</p>

<p align="center">OpenGL performance overlay for Linux, macOS and *BSD</p>

## About

simpleoverlay is an OpenGL performance overlay for Linux, macOS and *BSD written in C

It supports the CGL, GLX and EGL OpenGL interface

## Configuration

simpleoverlay can be configured with the following environment variables

- SCALE
- KEY_COLOR
- VALUE_COLOR
- FPS_ONLY

You can modify them in the `simpleoverlay` wrapper file

## Build-time dependencies
- C compiler
- make

## Installing

```
git clone https://github.com/tranarchy/simpleoverlay
cd simpleoverlay
make
sudo make install
```

If you are on macOS use `make macos`

## 32-bit support

For 32-bit game support (such as HL2) you will need to run

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
sysrc -f /etc/rc.conf kld_list+=coretemp
service kld restart
```

<br>

> [!NOTE]
> Currently GPU information is limited to AMD GPUs
>
> Other parts of the overlay will still work fine for non AMD GPU users

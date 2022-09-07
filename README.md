## Host Env
ubuntu22.04
``` zsh
sudo apt-get install ccache debian-archive-keyring debootstrap device-tree-compiler dwarves gcc-arm-linux-gnueabihf jq libbison-dev libc6-dev-armhf-cross libelf-dev libfl-dev liblz4-tool libpython2.7-dev libusb-1.0-0-dev pigz pixz pv swig pkg-config python3-distutils qemu-user-static u-boot-tools distcc uuid-dev lib32ncurses-dev lib32stdc++6 apt-cacher-ng aptly aria2 libfdt-dev libssl-dev
```
## SDK version
- kernel 5.16
- uboot 2021.10
## Compilation Notes
Compilation System
Run directly from the project home directory
``` bash
sudo ./build.sh
```

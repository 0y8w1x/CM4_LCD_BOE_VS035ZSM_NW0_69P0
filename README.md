# CM4_LCD_BOE_VS035ZSM_NW0_69P0
Raspberry Pi CM4 bindings for BOE VS035ZSM-NW0-69P0 1440x1600 LCD display

**Status:** WIP 

* adapter board for the rpi cm4 [done and working, not uploaded yet]
* driver + device tree for rpi linux [not tested]

# Overview
BOE VS035ZSM NW0 69P0 panel is a 1440x1600 4-lane DSI LCD display. To use it, 4-lane DSI interface is required, so it can be used with Raspberry Pi Compute Modules 3 and 4.

# Software installation
We need build rpi-5.10.y branch with BOE VS035ZSM NW0 69P0 panel module enabled. This is well documented at Raspberry Pi help pages [here](https://www.raspberrypi.org/documentation/linux/kernel/building.md) and [here](https://www.raspberrypi.org/documentation/linux/kernel/configuring.md)

NOTE: For cross-compiling, see information [here](https://www.raspberrypi.org/documentation/linux/kernel/building.md)

```
sudo apt install git bc bison flex libssl-dev make
sudo apt install libncurses5-dev
git clone --depth=1 --branch rpi-5.10.y https://github.com/raspberrypi/linux
cd linux
KERNEL=kernel7l
make bcm2711_defconfig
make menuconfig
```
Navigate to Device Drivers/Graphics support/Display panel, check BOE VS035ZSM NW0 69P0 panel and save

Now we need to build and install kernel:
```
make -j4 zImage modules dtbs
sudo make modules_install
sudo cp arch/arm/boot/dts/*.dtb /boot/
sudo cp arch/arm/boot/dts/overlays/*.dtb* /boot/overlays/
sudo cp arch/arm/boot/dts/overlays/README /boot/overlays/
sudo cp arch/arm/boot/zImage /boot/$KERNEL.img
```

Then compile and copy the Device Tree source to boot partition:
```
sudo dtc -@ -I dts -O dtb -o vc4-kms-dsi-vs035zsm-nw0-69p0.dtbo vc4-kms-dsi-vs035zsm-nw0-69p0-overlay.dts
sudo cp vc4-kms-dsi-vs035zsm-nw0-69p0.dtbo /boot/overlays/
```

Then edit /boot/config.txt to add:
```
lcd_ignore=1
dtoverlay=vc4-kms-v3d
dtoverlay=vc4-kms-dsi-vs035zsm-nw0-69p0
```
and reboot

After reboot screen should we working and have portrait orientation. To adjust, use Main Menu/Preferences/Screen configuration utility

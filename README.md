# qebspil
[qebspil] (Qualcomm Exit Boot Services Peripheral Image Loader) is an UEFI boot
driver that allows starting co-processors (such as DSPs) on Qualcomm platforms
late during the boot process (shortly before the bootloader exits UEFI using
`ExitBootServices()`). This can be used to start co-processors early,
independent of the selected operating system, and without disrupting the normal
UEFI boot flow. Possible use cases include (for example):

  - Start full DSP firmwares for operating system configurations that do not
    support DSP startup/firmware loading (e.g. EL2/virtualization-enabled
    Linux configurations on most Qualcomm compute/laptop platforms).

  - Make use of DSP services early during operating system startup.

**Note:** This is a spare time project not affiliated with Qualcomm or Linaro.

## Supported platforms
- Qualcomm Snapdragon 7c Compute Platform (SC7180)
- Qualcomm Snapdragon 8cx Gen 3 Compute Platform (SC8280XP)
- Qualcomm Snapdragon X Elite (X1E)

**Currently, only booting the full audio DSP (ADSP) firmware is supported.**
Booting the DSP may fail due to the lack of a RPMh driver in qebspil, which
is needed to ensure stable DSP startup. Improving reliability and adding
support for other DSPs such as the compute DSP (CDSP) is work-in-progress.

## Build
1. Make sure you have the `aarch64-linux-gnu-gcc` cross compiler installed.
   (If you want to build natively, omit `CROSS_COMPILE=` in the `make` command).

2. Make sure you have submodules checked out and up to date:

       $ git submodule update --init --recursive

3. Build qebspil using `make`:

       $ make CROSS_COMPILE=aarch64-linux-gnu-

4. The output file is located at `out/qebspilaa64.efi`.

## Usage
Currently, qebspil becomes active only when a device tree is installed by the
bootloader (e.g. when booting Linux using [systemd-boot] or [GRUB]). Only
co-processors (remoteprocs) enabled in the device tree will be started.

In addition, by default only remoteprocs with the `qcom,broken-reset`
property are started. This is intended to allow compatibility with standard
Linux configurations, where the remoteprocs are expected to be off during boot
and are later started by Linux during the boot process. To start all
remoteprocs even without this property, build qebspil with the
`QEBSPIL_ALWAYS_START=1` option in the `make` command line.

To use qebspil with Linux, additional patches are currently needed to allow
taking over the firmware started early by qebspil. They can be found in the
following branches:

  - **Linux 6.16:** https://git.codelinaro.org/stephan.gerhold/linux/-/tree/wip/x1e80100-6.16-el2
  - **Linux 6.17:** https://git.codelinaro.org/stephan.gerhold/linux/-/tree/wip/qcom-laptops-6.17-el2

## Installation
qebspil is an UEFI boot driver, it can be loaded manually from the EFI shell
or loaded automatically by the UEFI firmware and/or bootloader.

For initial testing, copy `qebspilaa64.efi` to an USB flash drive together with
the necessary firmware (see [Firmware](#firmware) below). Then load it from an
EFI shell as follows:

    # First navigate to USB flash drive using fsXY: (check e.g. using "ls")
    fsXY:\> load qebspilaa64.efi

Loading qebspil automatically is easiest when using [systemd-boot]: Copy
`qebspilaa64.efi` to your EFI System Partition (ESP) at
`/EFI/systemd/drivers/qebspilaa64.efi`. Note that the firmware must be also
copied to the ESP, to the top-level `/firmware` directory as described below.

### Firmware
qebspil needs access to the remoteproc firmware to start co-processors early.
The `firmware-name` is used from the device tree to allow creating generic
boot images (e.g. in combination with [dtbloader], UKIs or similar). All
firmware files should be placed in the same partition as `qebspilaa64.efi`
in a top-level `/firmware` directory, e.g.:

    /firmware/
    └── qcom/
        ├── sc8280xp/
        │   └── LENOVO/
        │       └── 21BX/
        │           ├── qcadsp8280.mbn
        │           ├── qccdsp8280.mbn
        │           └── qcslpi8280.mbn
        └── x1e80100/
            └── LENOVO/
                └── 21N1/
                    ├── adsp_dtbs.elf
                    ├── cdsp_dtbs.elf
                    ├── qcadsp8380.mbn
                    └── qccdsp8380.mbn

To find the necessary firmware files on a running system, you can use e.g.:

    $ find /sys/firmware/devicetree -name firmware-name -exec cat {} + | xargs -0n1

Alternatively, search for the `firmware-name` properties in the device tree
source code. The firmware files used by Linux are typically loaded from
`/lib/firmware/`. You may need to copy the firmware files from the Windows
partition if they are not part of upstream [linux-firmware].

## License
To maintain compatibility with the Linux kernel, this project is licensed under
the GNU General Public License v2.0 only (GPL-2.0-only). Contributions welcome!

qebspil includes code from the following third-party projects:

  - [gnu-efi](https://github.com/ncroxon/gnu-efi)
  - [dtc](https://git.kernel.org/pub/scm/utils/dtc/dtc.git)
  - Portions of [EDK2](https://github.com/tianocore/edk2)
  - Portions of Qualcomm's [ABL](https://git.codelinaro.org/clo/la/abl/tianocore/edk2)
    bootloader, to interface with the secure firmware
  - Portions of [lk2nd](https://github.com/msm8916-mainline/lk2nd)

The licensing for this code can be found in the respective source code files.

[qebspil]: https://github.com/stephan-gh/qebspil
[systemd-boot]: https://systemd.io/BOOT/
[GRUB]: https://www.gnu.org/software/grub/
[dtbloader]: https://github.com/TravMurav/dtbloader
[linux-firmware]: https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git

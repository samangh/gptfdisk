README for EFI version of GPT fdisk
===================================

GPT fdisk for EFI is a binary build of gdisk to run as a pre-boot EFI
application. It's OS-independent and may be used to check or recover
partition tables before installing or booting an OS. It may be used to
overcome boot problems caused by partition table damage or to prepare a
partition table prior to installing an OS.

Installing GPT fdisk for EFI
----------------------------

The contents of this archive are:

- COPYING -- The GNU GPL
- gdisk.html -- The gdisk man page, in HTML form
- gdisk_x64.efi -- The gdisk binary, built for EFI (x86-64 CPU)
- NEWS -- The GPT fdisk changelog
- README-efi.txt -- This file
- refind.cer -- The rEFInd public key, .cer (DER) form
- refind.crt -- The rEFInd public key, .crt form

The gdisk_x64.efi binary included here is built using the UEFI GPT fdisk
library (https://sourceforge.net/p/uefigptfdisk/), which is a beta-level
partial C++ library for UEFI. To use it, you must copy it to your EFI
System Partition (ESP) or some other EFI-accessible location. Under Linux,
the ESP is usually one of the first two or three partitions on /dev/sda.
Under OS X, it's usually the first partition on /dev/disk0 (that is,
/dev/disk0s1). Under Windows, you can mount it to S: by typing "mountvol S:
/S" in an Administrator command prompt. In any of these cases, the
recommended location for gdisk_x64.efi is the EFI/tools directory on the
ESP. In that location, my rEFInd boot manager will detect the gdisk binary
and create a menu option to launch it. If you don't use rEFInd, you can
launch the program using an EFI shell, register it as a boot program with
your firmware, or configure your boot manager (GRUB, gummiboot, etc.) to
launch it. Note that boot LOADERS, such as SYSLINUX and ELILO, can't launch
gdisk.

Alternatively, you can create a USB flash drive that will launch gdisk when
you boot from it. To do so, create a FAT filesystem on a partition on a USB
flash drive and copy gdisk_x64.efi to it as EFI/BOOT/bootx64.efi. (You'll
need to create the EFI/BOOT directory.) Some systems may require the FAT
filesystem to be flagged as an ESP (with a type code of EF00 in gdisk). You
can use your firmware's built-in boot manager to boot from the USB flash
drive. Some such boot managers present two options for booting USB flash
drives. If yours does this, select the option that includes the string
"UEFI" in the description.

The gdisk_x64.efi binary is signed with the rEFInd Secure Boot key. Thus,
if you're launching a rEFInd that I've compiled and distributed myself,
gdisk should launch, too. If you're *NOT* running rEFInd but ARE using
Shim, you'll need to add the refind.cer file to your MOK list by using the
MokManager utility. If you're using Secure Boot and you've signed rEFInd
yourself, you'll need to sign gdisk_x64.efi yourself, too. Note that the
rEFInd PPA distributes unsigned binaries and signs them with a local key
stored in /etc/refind/keys. To copy and sign the gdisk_x64.efi binary, you
should type (as root or using sudo):

sbsign --key /etc/refind.d/keys/refind_local.key \
  --cert /etc/refind.d/keys/refind.crt \
  --output /boot/efi/EFI/tooks/gdisk_x64.efi ./gdisk_x64.efi

This command assumes you have local rEFInd keys stored in the locations
created by the rEFInd installation script. Substitute your own keys if
you've built them in some other way. Some distributions don't provide the
sbsign binary, so you may need to build it yourself. See the following page
for details:

https://git.kernel.org/cgit/linux/kernel/git/jejb/sbsigntools.git/

Note that you do *NOT* need to sign gdisk if your computer doesn't use
Secure Boot or if you've disabled this feature.

Using gdisk for EFI
-------------------

The EFI version of gdisk is basically the same as using the Linux, OS X, or
other OS versions. One exception is that you do not specify a disk device
on the command line; gdisk for EFI instead displays a list of devices when
you launch and enables you to select one, as in:

List of hard disks found:
 1: Disk EFI_HANDLE(3EB5DD98): 108423424 sectors, 51.7 GiB
    Acpi(PNP0A03,0)/Pci(1|1)/Ata(Primary,Master)
 2: Disk EFI_HANDLE(3EB58289): 105456768 sectors, 50.3 GiB
    Acpi(PNP0A03,0)/Pci(D|0)?

Disk number (1-2): 2

Once you've selected your disk, it should operate in much the same way as
any other version of gdisk. (See the next section, though!) Some programs,
including my rEFInd boot manager, complain about the changed partition
table, even if you've made no changes. If you run into problems using other
programs or launching an OS immediately after running gdisk, reboot; that
should cause the firmware to re-load its partition table.

Caveats
-------

I've tested gdisk_x64.efi on several systems. It's worked fine for me on 4
of 6 computers (5 of 7, counting VirtualBox). Two systems gave me problems,
though:

* gdisk presented a never-ending list of options (as if receiving a
  never-ending string of "?" or other unrecognized command characters) on a
  2014 MacBook Air.
* A computer based on an Intel DG43NB motherboard rebooted as soon as I
  launched gdisk.

Both computers have relatively old EFIs. (Despite its newness, the Mac has
a 1.10 EFI, as do all Macs, to the best of my knowledge.) Most of the
computers that worked had 2.31 EFIs, although one had a 2.10 EFI.

The bottom line is that I can't guarantee that this binary will work on all
computers. It's conceivable that recompiling gdisk with the latest version
of the UEFI GPT fdisk library will help. Also, I haven't compiled a 32-bit
version, so if you have a 32-bit EFI, you'll have to compile it yourself or
do without.

References
----------

The following sites have useful additional information:

UEFI GPT fdisk:
https://sourceforge.net/projects/uefigptfdisk/

sbsigntools git repository:
https://git.kernel.org/cgit/linux/kernel/git/jejb/sbsigntools.git/

rEFInd:
http://www.rodsbooks.com/refind/

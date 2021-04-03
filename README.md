# VitaTools
(Maybe) useful tools for PSP2 Vita and Dolce

### storageFormat
- A tool that can format the desired storage to a desired filesystem.
- SD2Vita, USB, PSVSD, External and Internal memory card supported.
- Supported filesystems: FAT16, FAT32, TexFAT.
- PSVSD/USB need to be mounted before using this tool.
- Tested on firmwares: retail 3.60 and 3.65.

### recoVery
- A proper gui recovery for the reset image, intended for enso_ex.
- Can usb-mount every FAT16 and TexFAT partition (GC/XMC/EMMC).
- Support for EMMC flashing and dumping.
- Can load external sd0: helper kernel modules.
- Kernel modules required firmware: 3.65.

### mcfredir
- Patches SceSettings to format SD2Vita instead of the memory card.
- Redirects format popup at boot as well as the format option in settings.
- Put in taihen config under *NPXS10015 (settings) and *NPXS10016 (popup)
- Tested on firmwares: retail 3.60 and 3.65.

### vlog
- Prints debug output on screen.
- Remember to remove/change debug handlers before shell or recovery.
- Tested on firmware 3.65.

### pdbridge
- A small kernel-user bridge to manage the diag modules.
- Also sets some diag-checked dip switches.
- pd_test can be used to test pdbridge as well as PdDisplayOled.
- Tested on firmware 3.65 with 0syscall6.

### bgvpk
- background vpk downloader and installer
- based on download_enabler by TheFlow
- can be included in both hb apps and tai config
- Tested on firmware 3.65, should work on 3.60-3.73

### Notes
- All tools are licensed under MIT unless stated otherwise.
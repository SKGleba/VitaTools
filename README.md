# VitaTools
(Maybe) useful tools for PSP2 Vita and Dolce

### vita-bootanim
https://user-images.githubusercontent.com/30833773/125276948-220a9480-e311-11eb-823d-12ac39f43f14.mp4

- bootanimations plugin for enso
- convert gifs to rcf using the tool in /pc/ (linux-only atm)
  - ImageMagick and gzip required
  - run "mkanim -help" to get the list of available commands
  - max animation size: 108MiB
  - put boot.rcf in ur0:tai/
- faulty animations can be skipped by holding LTrigger
  - try setting a lower priority and optimizing the animation
- you can configure the boot animation in "settings->theme & background"
- Tested on firmware 3.65, should work on enso 3.60-3.73

### bgvpk
![bgvpk](https://user-images.githubusercontent.com/30833773/125277019-377fbe80-e311-11eb-9835-8a8a427213cd.png)

- background vpk downloader and installer
- based on download_enabler by TheFlow
- can be included in both hb apps and tai config
- Tested on firmware 3.65, should work on 3.60-3.73

### fakegc
![fakegc](https://user-images.githubusercontent.com/30833773/125277042-3ea6cc80-e311-11eb-83e7-6172d7b95c46.jpg)

- removes gc checks from the gro0: partition (can run any app)
- put the app files in gro0:app/TITLEID/ and copy param.sfo to gro0:gc/
  - retail games need to be decrypted first
- put fakegc.suprx under *main in taiHen's config.txt
- the provided kernel plugin will mount sd2vita to both gro0: and grw0:
  - compatible with YAMT's full version too
- you can redirect ux0:data/ to grw0:reData/ with [rePatch](https://github.com/SonicMastr/rePatch-reLoaded)
  - if the game crashes you will need to recompile it with the new path
- Tested on firmwares 3.60 and 3.65

### recoVery
![recoVery](https://user-images.githubusercontent.com/30833773/125277057-436b8080-e311-11eb-99b4-83c6bc066acb.jpg)

- A proper gui recovery for the reset image, intended for enso_ex.
- Can usb-mount every FAT16 and TexFAT partition (GC/XMC/EMMC).
- Support for EMMC flashing and dumping.
- Can load external sd0: helper kernel modules.
- Kernel modules required firmware: 3.65.

### vlog
https://user-images.githubusercontent.com/30833773/125277077-48303480-e311-11eb-9255-f23087712666.mp4

- Prints debug output on screen.
- Remember to remove/change debug handlers before shell or safemode.
- Tested on firmware 3.65.

### storageFormat
![storageFormat](https://user-images.githubusercontent.com/30833773/125276383-719c9080-e310-11eb-9ebb-e5cd7a39a08b.png)


- A tool that can format the desired storage to a desired filesystem.
- SD2Vita, USB, PSVSD, External and Internal memory card supported.
- Supported filesystems: FAT16, FAT32, TexFAT.
- PSVSD/USB need to be mounted before using this tool.
- Tested on firmwares: retail 3.60 and 3.65.

### mcfredir
- Patches SceSettings to format SD2Vita instead of the memory card.
- Redirects format popup at boot as well as the format option in settings.
- Put in taihen config under *NPXS10015 (settings) and *NPXS10016 (popup)
- Tested on firmwares: retail 3.60 and 3.65.

### pdbridge
- A small kernel-user bridge to manage the diag modules.
- Also sets some diag-checked dip switches.
- pd_test can be used to test pdbridge as well as PdDisplayOled.
- Tested on firmware 3.65 with 0syscall6.

### Notes
- All tools are licensed under MIT unless stated otherwise.

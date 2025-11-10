=== YumeKey 3 Release ===

YumeKey 3 (codename: Obsidian) disables activation requirements for Synthesizer V Studio Pro
and voicebanks.

You do not need to own Synthesizer V Studio Pro or any voicebanks.

For more information on YumeKey, see https://jinpwnsoft.re or email support@jinpwnsoft.re
YumeKey is and always will be free. If you paid for this software, then you've been scammed.

Setup instructions:

1. Copy "obsidian.exe" and "obsidian.dll" to the Synthesizer V Studio Pro install path,
   usually "C:\Program Files\Synthesizer V Studio Pro" or similar.

2. Optionally create "obsidian.ini" in that same path for configuration (see below).

3. Use "obsidian.exe" to open Synthesizer V Studio Pro and install any voice database you want.

Limitations:

* You should deactivate your purchased voicebanks before, to avoid losing activation code uses
  by accidental uninstallation.

* Tested with Synthesizer V Studio Pro 1.11.0 through 1.11.2.

* No VST support yet. Planned for later.

Configuration (ADVANCED):

You can use any of these options in obsidian.ini, or you can set them as environment variables
before launching Synthesizer V Studio.

; Base address of SynthV module (1 - use default; otherwise use specified)
OBSIDIAN_BASE_ADDR=1
; Debug mode (0 - nothing, 1 - enabled, 2 - verbose)
OBSIDIAN_DEBUG_MODE=0
; Console file, to save logs if debug mode is enabled.
OBSIDIAN_CONSOLE_FILE=C:\path\to\log\file.txt
; Block Internet access for SynthV.
OBSIDIAN_BLOCK_INTERNET=1
; Check SynthV version for compatibility.
OBSIDIAN_CHECK_SYNTHV_VERSION=1

If you were to set these in the environment, you can set them globally via Windows settings or
at a command prompt:

C:\Program Files\Synthesizer V Studio Pro> set OBSIDIAN_DEBUG_MODE=2

C:\Program Files\Synthesizer V Studio Pro> obsidian.exe

Environment variables override config file options.

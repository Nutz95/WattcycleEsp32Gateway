# JC 4.3 inch panel driver

ST7701 over MIPI DSI and GT911 touch for the 480×800 panel.
These sources are the manufacturer's Arduino demo drivers, kept in this repo
so the panel sketch builds without a path on the build machine.

`scripts/flash_p4_panel.ps1` copies `pins_config.h`, `lcd/`, and `touch/`
into the staged sketch.

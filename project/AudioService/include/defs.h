
#ifndef DEFS_H
#define DEFS_H

#include <NERvGear/version.h>


#define _TRANSLATE(_STR) _STR    // help poEdit to find translate string

#define NAME_STRING    _TRANSLATE("AudioDVServer")
#define DESCRIP_STRING _TRANSLATE("Plug-in for Audio Data Visualization.")
#define COMMENT_STRING _TRANSLATE("The SAO Utils Plug-in by Mashiro_Sorata")

#define VER_MAJOR    1
#define VER_MINOR    0
#define VER_SUBMINOR 0
#define VER_RESERVED 0

#define VER_REV                 VER_MAJOR, VER_MINOR, VER_SUBMINOR, VER_RESERVED
#define VER_STR NVG_DOT_VERSION(VER_MAJOR, VER_MINOR, VER_SUBMINOR, VER_RESERVED)
#define VER_NAME VER_STR " " NVG_PLATFORM_SUFFIX


#endif // DEFS_H

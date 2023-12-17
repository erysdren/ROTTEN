/*
Copyright (C) 1994-1995 Apogee Software, Ltd.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
//***************************************************************************
//
//    MODEXLIB.C - various utils palette funcs and modex stuff
//
//***************************************************************************

#ifndef _modexlib_public
#define _modexlib_public

#include "WinRott.h"
#include "rt_def.h"
#include "lumpy.h"

//***************************************************************************
//
//    Video (ModeX) Constants
//
//***************************************************************************

extern boolean StretchScreen;

extern int ylookup[800]; // Table of row offsets
extern int linewidth;
extern int screensize;
extern byte *displayofs;
extern byte *bufferofs;
extern byte *BackBuffer;
extern boolean graphicsmode;

void GraphicsMode(void);
void SetTextMode(void);
void VL_SetVGAPlaneMode(void);
void VL_ClearBuffer(byte *buf, byte color);
void VL_ClearVideo(byte color);
void VL_DePlaneVGA(void);
void VL_CopyPlanarPage(byte *src, byte *dest);
void VL_CopyPlanarPageToMemory(byte *src, byte *dest);
void XFlipPage(void);
void WaitVBL(void);
void TurnOffTextCursor(void);
void ToggleFullScreen(void);
void SetShowCursor(int);

void ToggleScreenStretch(void);
void SetScreenStretch(boolean to);

enum {
	ALIGN_TL, /* top left */
	ALIGN_T, /* center top */
	ALIGN_TR, /* top right */
	ALIGN_R, /* center right */
	ALIGN_BR, /* bottom right */
	ALIGN_B, /* center bottom */
	ALIGN_BL, /* bottom left */
	ALIGN_L, /* center left */
	ALIGN_C /* center */
};

void DrawPic(char *name, int x, int y);
void DrawPicScaled(char *name, int x, int y, int s);
void DrawPicSized(char *name, int x, int y, int w, int h);
void DrawPicAligned(char *name, int x, int y, int align);
void ShutdownPicCache(void);

typedef struct vidconfig_t {
	int WindowWidth;
	int WindowHeight;
	int ScreenBaseWidth;
	int ScreenBaseHeight;
	int ScreenWidth;
	int ScreenHeight;
	int ScreenScale;
	boolean ScreenStretch;
} vidconfig_t;

/* global video config */
extern vidconfig_t vidconfig;

#define VGAMAPMASK(a)
#define VGAREADMAP(a)
#define VGAWRITEMAP(a)

#endif

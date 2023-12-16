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

#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <stdlib.h>
#include <sys/stat.h>
#include "modexlib.h"
#include "rt_util.h"
#include "rt_net.h" // for GamePaused
#include "myprint.h"

static void StretchMemPicture();
// GLOBAL VARIABLES

boolean StretchScreen = 0; // bn�++
extern boolean iG_aimCross;
extern boolean sdl_fullscreen;
extern boolean sdl_mouse_grabbed;
extern int iG_X_center;
extern int iG_Y_center;
byte *iG_buf_center;

int linewidth;
int ylookup[800]; // Table of row offsets
byte *page1start;
byte *page2start;
byte *page3start;
int screensize;
byte *bufferofs;
byte *displayofs;
boolean graphicsmode = false;
byte *bufofsTopLimit;
byte *bufofsBottomLimit;

void DrawCenterAim();

/* global video config */
vidconfig_t vidconfig = {
	640, /* WindowWidth */
	480, /* WindowHeight */
	320, /* ScreenBaseWidth */
	200, /* ScreenBaseHeight */
	320, /* ScreenWidth */
	200, /* ScreenHeight */
	1,   /* ScreenScale */
	true /* ScreenStretch */
};

#include "SDL.h"

/*
====================
=
= GraphicsMode
=
====================
*/
static SDL_Surface *sdl_surface = NULL;
static SDL_Surface *unstretch_sdl_surface = NULL;

static SDL_Window *screen;
static SDL_Renderer *renderer;
static SDL_Surface *argbbuffer;
static SDL_Texture *texture;
static SDL_Rect blit_rect = { 0 };

SDL_Surface *VL_GetVideoSurface(void)
{
	return sdl_surface;
}

int VL_SaveBMP(const char *file)
{
	return SDL_SaveBMP(sdl_surface, file);
}

void SetShowCursor(int show)
{
	SDL_SetRelativeMouseMode(!show);
	SDL_GetRelativeMouseState(NULL, NULL);
	sdl_mouse_grabbed = !show;
}

void GraphicsMode(void)
{
	uint32_t flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
	uint32_t pixel_format;

	unsigned int rmask, gmask, bmask, amask;
	int bpp;

	if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
	{
		Error("Could not initialize SDL\n");
	}

	if (sdl_fullscreen)
	{
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}

	screen = SDL_CreateWindow(NULL, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, vidconfig.WindowWidth, vidconfig.WindowHeight, flags);
	SDL_SetWindowMinimumSize(screen, vidconfig.WindowWidth, vidconfig.WindowHeight);
	SDL_SetWindowTitle(screen, PACKAGE_STRING);

	renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_PRESENTVSYNC);
	if (!renderer)
	{
		renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_SOFTWARE);
	}

	/* enable screen stretch */
	SetScreenStretch(vidconfig.ScreenStretch);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);

	sdl_surface = SDL_CreateRGBSurface(0, vidconfig.ScreenWidth, vidconfig.ScreenHeight, 8, 0, 0, 0, 0);
	SDL_FillRect(sdl_surface, NULL, 0);

	pixel_format = SDL_GetWindowPixelFormat(screen);
	SDL_PixelFormatEnumToMasks(pixel_format, &bpp, &rmask, &gmask, &bmask, &amask);
	argbbuffer = SDL_CreateRGBSurface(0, vidconfig.ScreenWidth, vidconfig.ScreenHeight, bpp, rmask, gmask, bmask, amask);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	texture = SDL_CreateTexture(renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, vidconfig.ScreenWidth, vidconfig.ScreenHeight);

	blit_rect.w = vidconfig.ScreenWidth;
	blit_rect.h = vidconfig.ScreenHeight;

	SetShowCursor(!sdl_fullscreen);
}

void ToggleFullScreen(void)
{
	unsigned int flags = 0;

	sdl_fullscreen = !sdl_fullscreen;

	if (sdl_fullscreen)
	{
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}

	SDL_SetWindowFullscreen(screen, flags);
	SetShowCursor(!sdl_fullscreen);
}

/*
====================
=
= SetTextMode
=
====================
*/
void SetTextMode(void)
{
	if (SDL_WasInit(SDL_INIT_VIDEO) == SDL_INIT_VIDEO)
	{
		if (sdl_surface != NULL)
		{
			SDL_FreeSurface(sdl_surface);

			sdl_surface = NULL;
		}

		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}
}

/*
====================
=
= TurnOffTextCursor
=
====================
*/
void TurnOffTextCursor(void)
{
}

/*
====================
=
= WaitVBL
=
====================
*/
void WaitVBL(void)
{
	SDL_Delay(16667 / 1000);
}

/*
=======================
=
= VL_SetVGAPlaneMode
=
=======================
*/

void VL_SetVGAPlaneMode(void)
{
	int i, offset;

	GraphicsMode();

	//
	// set up lookup tables
	//
	// bna--   linewidth = 320;
	linewidth = vidconfig.ScreenWidth;

	offset = 0;

	for (i = 0; i < vidconfig.ScreenHeight; i++)
	{
		ylookup[i] = offset;
		offset += linewidth;
	}

	//    screensize=MAXSCREENHEIGHT*MAXSCREENWIDTH;
	screensize = vidconfig.ScreenHeight * vidconfig.ScreenWidth;

	page1start = sdl_surface->pixels;
	page2start = sdl_surface->pixels;
	page3start = sdl_surface->pixels;
	displayofs = page1start;
	bufferofs = page2start;

	iG_X_center = vidconfig.ScreenWidth / 2;
	iG_Y_center = (vidconfig.ScreenHeight / 2) + 10; //+10 = move aim down a bit

	//(iG_Y_center*vidconfig.ScreenWidth);//+iG_X_center;
	iG_buf_center = bufferofs + (screensize / 2);

	bufofsTopLimit = bufferofs + screensize - vidconfig.ScreenWidth;
	bufofsBottomLimit = bufferofs + vidconfig.ScreenWidth;

	// start stretched
	EnableScreenStretch();
	XFlipPage();
}

/*
=======================
=
= VL_CopyPlanarPage
=
=======================
*/
void VL_CopyPlanarPage(byte *src, byte *dest)
{
	memcpy(dest, src, screensize);
}

/*
=======================
=
= VL_CopyPlanarPageToMemory
=
=======================
*/
void VL_CopyPlanarPageToMemory(byte *src, byte *dest)
{
	memcpy(dest, src, screensize);
}

/*
=================
=
= VL_ClearBuffer
=
= Fill the entire video buffer with a given color
=
=================
*/

void VL_ClearBuffer(byte *buf, byte color)
{
	memset((byte *)buf, color, screensize);
}

/*
=================
=
= VL_ClearVideo
=
= Fill the entire video buffer with a given color
=
=================
*/

void VL_ClearVideo(byte color)
{
	memset(sdl_surface->pixels, color, vidconfig.ScreenWidth * vidconfig.ScreenHeight);
}

/*
=================
=
= VL_DePlaneVGA
=
=================
*/

void VL_DePlaneVGA(void)
{
}

/* C version of rt_vh_a.asm */

void VH_UpdateScreen(void)
{
	/* get current window size */
	SDL_GetWindowSize(screen, &vidconfig.WindowWidth, &vidconfig.WindowHeight);

	/* blit video buffer to screen */
	SDL_LowerBlit(VL_GetVideoSurface(), &blit_rect, argbbuffer, &blit_rect);
	SDL_UpdateTexture(texture, NULL, argbbuffer->pixels, argbbuffer->pitch);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

/*
=================
=
= XFlipPage
=
=================
*/

void XFlipPage(void)
{
	SDL_LowerBlit(sdl_surface, &blit_rect, argbbuffer, &blit_rect);
	SDL_UpdateTexture(texture, NULL, argbbuffer->pixels, argbbuffer->pitch);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void EnableScreenStretch(void)
{
	/* no-op */
}

void DisableScreenStretch(void)
{
	/* no-op */
}

// bna section -------------------------------------------
static void StretchMemPicture()
{
	SDL_Rect src;
	SDL_Rect dest;

	src.x = 0;
	src.y = 0;
	src.w = 320;
	src.h = 200;

	dest.x = 0;
	dest.y = 0;
	dest.w = vidconfig.ScreenWidth;
	dest.h = vidconfig.ScreenHeight;
	SDL_SoftStretch(unstretch_sdl_surface, &src, sdl_surface, &dest);
}

// bna function added start
extern boolean ingame;
int iG_playerTilt;

void DrawCenterAim()
{
	int x;

	int percenthealth = (locplayerstate->health * 10) /
						MaxHitpointsForCharacter(locplayerstate);
	int color = percenthealth < 3	? egacolor[RED]
				: percenthealth < 4 ? egacolor[YELLOW]
									: egacolor[GREEN];

	if (iG_aimCross && !GamePaused)
	{
		if ((ingame == true) && (vidconfig.ScreenWidth > 320))
		{
			if ((iG_playerTilt < 0) ||
				(iG_playerTilt > vidconfig.ScreenHeight / 2))
			{
				iG_playerTilt = -(2048 - iG_playerTilt);
			}
			if (vidconfig.ScreenWidth == 640)
			{
				x = iG_playerTilt;
				iG_playerTilt = x / 2;
			}
			iG_buf_center = bufferofs + ((iG_Y_center - iG_playerTilt) *
										 vidconfig.ScreenWidth); //+iG_X_center;

			for (x = iG_X_center - 10; x <= iG_X_center - 4; x++)
			{
				if ((iG_buf_center + x < bufofsTopLimit) &&
					(iG_buf_center + x > bufofsBottomLimit))
				{
					*(iG_buf_center + x) = color;
				}
			}
			for (x = iG_X_center + 4; x <= iG_X_center + 10; x++)
			{
				if ((iG_buf_center + x < bufofsTopLimit) &&
					(iG_buf_center + x > bufofsBottomLimit))
				{
					*(iG_buf_center + x) = color;
				}
			}
			for (x = 10; x >= 4; x--)
			{
				if (((iG_buf_center - (x * vidconfig.ScreenWidth) + iG_X_center) <
					 bufofsTopLimit) &&
					((iG_buf_center - (x * vidconfig.ScreenWidth) + iG_X_center) >
					 bufofsBottomLimit))
				{
					*(iG_buf_center - (x * vidconfig.ScreenWidth) + iG_X_center) =
						color;
				}
			}
			for (x = 4; x <= 10; x++)
			{
				if (((iG_buf_center + (x * vidconfig.ScreenWidth) + iG_X_center) <
					 bufofsTopLimit) &&
					((iG_buf_center + (x * vidconfig.ScreenWidth) + iG_X_center) >
					 bufofsBottomLimit))
				{
					*(iG_buf_center + (x * vidconfig.ScreenWidth) + iG_X_center) =
						color;
				}
			}
		}
	}
}
// bna function added end

// bna section -------------------------------------------

void ToggleScreenStretch(void)
{
	SetScreenStretch(!vidconfig.ScreenStretch);
}

void SetScreenStretch(boolean to)
{
	int width, height;

	vidconfig.ScreenStretch = to;

	if (vidconfig.ScreenStretch)
	{
		width = vidconfig.ScreenWidth;
		height = vidconfig.ScreenHeight * 1.2f;
	}
	else
	{
		width = vidconfig.ScreenWidth;
		height = vidconfig.ScreenHeight;
	}

	SDL_RenderSetLogicalSize(renderer, width, height);
}

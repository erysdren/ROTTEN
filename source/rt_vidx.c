/*
Copyright (C) 1994-1995 Apogee Software, Ltd.
Copyright (C) 2002-2015 Steven Fuller, Ryan C. Gordon, John Hall, Dan Olson
Copyright (C) 2023 Fabian Greffrath
Copyright (C) 2023 erysdren (it/she/they)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "rt_view.h"
#include "rt_vidx.h"

/* global variables */
SDL_Surface *VX_WorldCanvas = NULL;
SDL_Surface *VX_OverlayCanvas = NULL;
vxconfig_t VX_Config = {
	CANVAS_WIDTH,
	CANVAS_HEIGHT,
	CANVAS_SCALE_MIN,
	true
};

/* local variables */
static SDL_Window *Window = NULL;
static SDL_Renderer *Renderer = NULL;
static SDL_Texture *RenderTexture = NULL;
static SDL_Surface *RenderSurface = NULL;
static SDL_Rect WorldCanvasRect = {0, 0, 0, 0};
static SDL_Rect OverlayCanvasRect = {0, 0, 0, 0};

/* initialize video subsystem */
void VX_Init(void)
{
	unsigned int pixel_format, rmask, gmask, bmask, amask;
	int bpp;

	/* init sdl subsystems */
	SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

	/* create world canvas */
	VX_WorldCanvas = SDL_CreateRGBSurface(0, VX_Config.CanvasWidth * VX_Config.WorldCanvasScale, VX_Config.CanvasHeight * VX_Config.WorldCanvasScale, 8, 0, 0, 0, 0);

	/* create overlay canvas */
	VX_OverlayCanvas = SDL_CreateRGBSurface(0, VX_Config.CanvasWidth, VX_Config.CanvasHeight, 8, 0, 0, 0, 0);
	SDL_SetColorKey(VX_OverlayCanvas, SDL_TRUE, 255);

	/* create window */
	Window = SDL_CreateWindow(PACKAGE_STRING, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, VX_WorldCanvas->w, VX_WorldCanvas->h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_SetWindowMinimumSize(Window, VX_WorldCanvas->w, VX_WorldCanvas->h);

	/* create renderer */
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_PRESENTVSYNC);
	SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
	SDL_RenderClear(Renderer);
	SDL_RenderPresent(Renderer);

	/* setup stretch */
	if (VX_Config.WindowStretchAspect)
		SDL_RenderSetLogicalSize(Renderer, VX_WorldCanvas->w, VX_WorldCanvas->h * 1.2f);
	else
		SDL_RenderSetLogicalSize(Renderer, VX_WorldCanvas->w, VX_WorldCanvas->h);

	/* get pixel format from screen */
	pixel_format = SDL_GetWindowPixelFormat(Window);
	SDL_PixelFormatEnumToMasks(pixel_format, &bpp, &rmask, &gmask, &bmask, &amask);

	/* create render surface */
	RenderSurface = SDL_CreateRGBSurface(0, VX_WorldCanvas->w, VX_WorldCanvas->h, bpp, rmask, gmask, bmask, amask);

	/* create render texture */
	RenderTexture = SDL_CreateTexture(Renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, VX_WorldCanvas->w, VX_WorldCanvas->h);
}

/* shutdown video subsystem */
void VX_Shutdown(void)
{
	if (Window) SDL_DestroyWindow(Window);
	if (Renderer) SDL_DestroyRenderer(Renderer);
	if (RenderTexture) SDL_DestroyTexture(RenderTexture);
	if (RenderSurface) SDL_FreeSurface(RenderSurface);
	if (VX_WorldCanvas) SDL_FreeSurface(VX_WorldCanvas);
	if (VX_OverlayCanvas) SDL_FreeSurface(VX_OverlayCanvas);
}

/* flip all canvas changes to visible screen */
void VX_Flip(void)
{
	WorldCanvasRect.x = 0;
	WorldCanvasRect.y = 0;
	WorldCanvasRect.w = VX_WorldCanvas->w;
	WorldCanvasRect.h = VX_WorldCanvas->h;

	OverlayCanvasRect.x = 0;
	OverlayCanvasRect.y = 0;
	OverlayCanvasRect.w = VX_OverlayCanvas->w;
	OverlayCanvasRect.h = VX_OverlayCanvas->h;

	SDL_LowerBlit(VX_WorldCanvas, &WorldCanvasRect, RenderSurface, &WorldCanvasRect);
	SDL_LowerBlit(VX_OverlayCanvas, &OverlayCanvasRect, RenderSurface, &OverlayCanvasRect);
	SDL_UpdateTexture(RenderTexture, NULL, RenderSurface->pixels, RenderSurface->pitch);
	SDL_RenderClear(Renderer);
	SDL_RenderCopy(Renderer, RenderTexture, NULL, NULL);
	SDL_RenderPresent(Renderer);
}

/* set video palette */
void VX_SetPalette(uint8_t *palette)
{
	SDL_Color colors[256];
	int i;

	for (i = 0; i < 256; i++)
	{
		colors[i].r = gammatable[(gammaindex << 6) + (*palette++)] << 2;
		colors[i].g = gammatable[(gammaindex << 6) + (*palette++)] << 2;
		colors[i].b = gammatable[(gammaindex << 6) + (*palette++)] << 2;
	}

	SDL_SetPaletteColors(VX_OverlayCanvas->format->palette, colors, 0, 256);
	SDL_SetPaletteColors(VX_WorldCanvas->format->palette, colors, 0, 256);
}
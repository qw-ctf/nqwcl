/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// gl_vidnt.c -- NT GL vid component

#include "quakedef.h"
#include "winquake.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl_glext.h>

#define VID_ROW_SIZE	3
#define WARP_WIDTH		320
#define WARP_HEIGHT		200
#define MAXWIDTH		10000
#define MAXHEIGHT		10000
#define BASEWIDTH		1280
#define BASEHEIGHT		720

SDL_Window *window = NULL;
SDL_GLContext context;

const char *gl_vendor;
const char *gl_renderer;
const char *gl_version;
const char *gl_extensions;

bool		DDActive;
bool		scr_skipupdate;

static int		windowed_mouse;

uint32_t		WindowStyle, ExWindowStyle;


unsigned char	vid_curpal[256*3];
static bool fullsbardraw = false;

static float vid_gamma = 1.0;

glvert_t glv;

cvar_t	gl_ztrick = {"gl_ztrick","1"};

viddef_t	vid;				// global video state

unsigned short	d_8to16table[256];
unsigned	d_8to24table[256];
unsigned char d_15to8table[65536];

float		gldepthmin, gldepthmax;

void VID_MenuDraw (void);
void VID_MenuKey (int key);

char *VID_GetModeDescription (int mode);
void ClearAllStates (void);
void GL_Init (void);

SDL_FunctionPointer glArrayElementEXT;
SDL_FunctionPointer glColorPointerEXT;
SDL_FunctionPointer glTexCoordPointerEXT;
SDL_FunctionPointer glVertexPointerEXT;

typedef void (APIENTRY *lp3DFXFUNC) (int, int, int, int, int, const void*);
lp3DFXFUNC glColorTableEXT;
bool isPermedia = false;
bool gl_mtexable = false;

//====================================

cvar_t		vid_wait = {"vid_wait","0"};
cvar_t		vid_vsync = {"vid_nopageflip","0", true};
cvar_t		_vid_wait_override = {"_vid_wait_override", "0", true};
cvar_t		vid_config_x = {"vid_config_x","1280", true};
cvar_t		vid_config_y = {"vid_config_y","720", true};
cvar_t		vid_stretch_by_2 = {"vid_stretch_by_2","1", true};
cvar_t		_windowed_mouse = {"_windowed_mouse","1", true};

void VID_ToggleFullscreen(void) {
	bool windowed = !(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN);
	SDL_SetWindowFullscreen(window, windowed);
}

void VID_SetTitle(const char *title)
{
	SDL_SetWindowTitle(window, title);
}

void VID_Restore(void)
{
	if (!window || (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS)) {
		return;
	}

	SDL_RestoreWindow(window);
	SDL_RaiseWindow(window);
}

BINDTEXFUNCPTR bindTexFunc;

#define TEXTURE_EXT_STRING "GL_EXT_texture_object"


void CheckTextureExtensions (void)
{
	if (SDL_GL_ExtensionSupported("GL_EXT_texture_object")) {
		// load library and get procedure adresses for texture extension API
		if ((bindTexFunc = (BINDTEXFUNCPTR) SDL_GL_GetProcAddress("glBindTextureEXT")) == NULL) {
			Sys_Error ("GetProcAddress for BindTextureEXT failed");
		}
	}
}

void CheckArrayExtensions (void)
{
	char		*tmp;

	/* check for texture extension */
	tmp = (unsigned char *)glGetString(GL_EXTENSIONS);
	while (*tmp)
	{
		if (strncmp((const char*)tmp, "GL_EXT_vertex_array", strlen("GL_EXT_vertex_array")) == 0)
		{
			if (
((glArrayElementEXT = SDL_GL_GetProcAddress("glArrayElementEXT")) == NULL) ||
((glColorPointerEXT = SDL_GL_GetProcAddress("glColorPointerEXT")) == NULL) ||
((glTexCoordPointerEXT = SDL_GL_GetProcAddress("glTexCoordPointerEXT")) == NULL) ||
((glVertexPointerEXT = SDL_GL_GetProcAddress("glVertexPointerEXT")) == NULL) )
			{
				Sys_Error ("GetProcAddress for vertex extension failed");
				return;
			}
			return;
		}
		tmp++;
	}

	Sys_Error ("Vertex array extension not present");
}

int		texture_mode = GL_LINEAR;
int		texture_extension_number = 1;

void CheckMultiTextureExtensions(void)
{
	if (strstr(gl_extensions, "GL_SGIS_multitexture ") && !COM_CheckParm("-nomtex")) {
		Con_Printf("Multitexture extensions found.\n");
		qglMTexCoord2fSGIS = (void *) SDL_GL_GetProcAddress("glMTexCoord2fSGIS");
		qglSelectTextureSGIS = (void *) SDL_GL_GetProcAddress("glSelectTextureSGIS");
		gl_mtexable = true;
	}
}

/*
===============
GL_Init
===============
*/
void GL_Init (void)
{
	gl_vendor = glGetString (GL_VENDOR);
	Con_Printf ("GL_VENDOR: %s\n", gl_vendor);
	gl_renderer = glGetString (GL_RENDERER);
	Con_Printf ("GL_RENDERER: %s\n", gl_renderer);

	gl_version = glGetString (GL_VERSION);
	Con_Printf ("GL_VERSION: %s\n", gl_version);
	gl_extensions = glGetString (GL_EXTENSIONS);
	Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);

	CheckTextureExtensions ();
	CheckMultiTextureExtensions ();

	glClearColor (1,0,0,0);
	glCullFace(GL_FRONT);
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.666);

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel (GL_FLAT);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

}

/*
=================
GL_BeginRendering

=================
*/
void GL_BeginRendering (int *x, int *y, int *width, int *height)
{
	*x = *y = 0;
	*width = vid.width;
	*height = vid.height;
}


void GL_EndRendering (void)
{
	if (!scr_skipupdate || block_drawing)
	{
		SDL_GL_SwapWindow(window);
	}

	// handle the mouse state when windowed if that's changed
	if (!(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN))
	{
		if (!_windowed_mouse.value) {
			if (windowed_mouse)	{
				IN_DeactivateMouse ();
				IN_ShowMouse ();
				windowed_mouse = false;
			}
		} else {
			windowed_mouse = true;
			if (key_dest == key_game && !mouseactive && ActiveApp) {
				IN_ActivateMouse ();
				IN_HideMouse ();
			} else if (mouseactive && key_dest != key_game) {
				IN_DeactivateMouse ();
				IN_ShowMouse ();
			}
		}
	}
	if (fullsbardraw)
		Sbar_Changed();
}

void	VID_SetPalette (unsigned char *palette)
{
	byte	*pal;
	unsigned r,g,b;
	unsigned v;
	int     r1,g1,b1;
	int		j,k,l;
	unsigned short i;
	unsigned	*table;

	//
	// 8 8 8 encoding
	//
	pal = palette;
	table = d_8to24table;
	for (i=0 ; i<256 ; i++)
	{
		r = pal[0];
		g = pal[1];
		b = pal[2];
		pal += 3;
		v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
		*table++ = v;
	}
	d_8to24table[255] &= 0xffffff;	// 255 is transparent

	// JACK: 3D distance calcs - k is last closest, l is the distance.
	// FIXME: Precalculate this and cache to disk.
	for (i=0; i < (1<<15); i++) {
		/* Maps
			000000000000000
			000000000011111 = Red  = 0x1F
			000001111100000 = Blue = 0x03E0
			111110000000000 = Grn  = 0x7C00
		*/
		r = ((i & 0x1F) << 3)+4;
		g = ((i & 0x03E0) >> 2)+4;
		b = ((i & 0x7C00) >> 7)+4;
		pal = (unsigned char *)d_8to24table;
		for (v=0,k=0,l=10000*10000; v<256; v++,pal+=4) {
			r1 = r-pal[0];
			g1 = g-pal[1];
			b1 = b-pal[2];
			j = (r1*r1)+(g1*g1)+(b1*b1);
			if (j<l) {
				k=v;
				l=j;
			}
		}
		d_15to8table[i]=k;
	}
}

bool	gammaworks;

void VID_ShiftPalette (unsigned char *palette)
{
	VID_SetPalette (palette);
}


void VID_SetDefaultMode (void)
{
	IN_DeactivateMouse ();
}


void VID_Shutdown (void)
{
	if (window)
	{
		SDL_GL_DestroyContext(context);
		SDL_DestroyWindow(window);
	}
}


/*
===================================================================

MAIN WINDOW

===================================================================
*/

/*
================
ClearAllStates
================
*/
void ClearAllStates (void)
{
	int		i;
	
// send an up event for each key, to make sure the server clears them all
	for (i=0 ; i<256 ; i++)
	{
		Key_Event (i, false);
	}

	Key_ClearStates ();
	IN_ClearStates ();
}

/*
void AppActivate(BOOL fActive, BOOL minimize)
{
	MSG msg;
    HDC			hdc;
    int			i, t;
	static BOOL	sound_active;

	ActiveApp = fActive;
	Minimized = minimize;

// enable/disable sound on focus gain/loss
	if (!ActiveApp && sound_active)
	{
		S_BlockSound ();
		sound_active = false;
	}
	else if (ActiveApp && !sound_active)
	{
		S_UnblockSound ();
		sound_active = true;
	}

	if (fActive)
	{
		if (modestate == MS_FULLDIB)
		{
			IN_ActivateMouse ();
			IN_HideMouse ();
			if (vid_canalttab && vid_wassuspended) {
				vid_wassuspended = false;
				ChangeDisplaySettings (&gdevmode, CDS_FULLSCREEN);
				ShowWindow(mainwindow, SW_SHOWNORMAL);
			}
		}
		else if ((modestate == MS_WINDOWED) && _windowed_mouse.value && key_dest == key_game)
		{
			IN_ActivateMouse ();
			IN_HideMouse ();
		}
	}

	if (!fActive)
	{
		if (modestate == MS_FULLDIB)
		{
			IN_DeactivateMouse ();
			IN_ShowMouse ();
			if (vid_canalttab) { 
				ChangeDisplaySettings (NULL, 0);
				vid_wassuspended = true;
			}
		}
		else if ((modestate == MS_WINDOWED) && _windowed_mouse.value)
		{
			IN_DeactivateMouse ();
			IN_ShowMouse ();
		}
	}
}
*/

void VID_WindowResized(void) {
	SDL_GetWindowSizeInPixels(window, &vid.width, &vid.height);
	vid.conwidth = vid.width;
	vid.conheight = vid.height;
	vid.recalc_refdef = true;
	SCR_UpdateScreen();
	Draw_AdjustConsoleBackground();
}

/*
=================
VID_GetModeDescription
=================
*/
char *VID_GetModeDescription (int mode)
{
	static char	modestring[100];
	int w, h;

	if (!SDL_GetWindowSize(window, &w, &h)) {
		Sys_Error("Could not query window size");
	}

	SDL_snprintf (modestring, sizeof(modestring), "Desktop resolution (%dx%d)", w, h);

	return modestring;
}

static void Check_Gamma (unsigned char *pal)
{
	float	f, inf;
	unsigned char	palette[768];
	int		i;

	if ((i = COM_CheckParm("-gamma")) == 0) {
		if ((gl_renderer && strstr(gl_renderer, "Voodoo")) ||
			(gl_vendor && strstr(gl_vendor, "3Dfx")))
			vid_gamma = 1;
		else
			vid_gamma = 0.7; // default to 0.7 on non-3dfx hardware
	} else
		vid_gamma = SDL_atof(com_argv[i+1]);

	for (i=0 ; i<768 ; i++)
	{
		f = pow ( (pal[i]+1)/256.0 , vid_gamma );
		inf = f*255 + 0.5;
		if (inf < 0)
			inf = 0;
		if (inf > 255)
			inf = 255;
		palette[i] = inf;
	}

	memcpy (pal, palette, sizeof(palette));
}

/*
===================
VID_Init
===================
*/
void	VID_Init (unsigned char *palette)
{
	int		i;
	int		width, height;

	Cvar_RegisterVariable (&vid_wait);
	Cvar_RegisterVariable (&vid_vsync);
	Cvar_RegisterVariable (&_vid_wait_override);
	Cvar_RegisterVariable (&vid_config_x);
	Cvar_RegisterVariable (&vid_config_y);
	Cvar_RegisterVariable (&vid_stretch_by_2);
	Cvar_RegisterVariable (&_windowed_mouse);
	Cvar_RegisterVariable (&gl_ztrick);

	if (COM_CheckParm("-width"))
	{
		width = SDL_atoi(com_argv[COM_CheckParm("-width")+1]);
	}
	else
	{
		width = 1280;
	}

	if (COM_CheckParm("-height"))
	{
		height = SDL_atoi(com_argv[COM_CheckParm("-height")+1]);
	}
	else
	{
		height = 720;
	}

	vid.width = width;
	vid.height = height;

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

	window = SDL_CreateWindow("nqwcl", width, height, window_flags);
	if (!window)
	{
		Sys_Error("Could not create window: %s\n", SDL_GetError());
	}

	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	context = SDL_GL_CreateContext(window);
	if (!context)
	{
		Sys_Error("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
	}

	SDL_GL_MakeCurrent(window, context);
	SDL_GL_SetSwapInterval((int) vid_vsync.value);
	SDL_ShowWindow(window);

	if ((i = COM_CheckParm("-conwidth")) != 0)
		vid.conwidth = SDL_atoi(com_argv[i+1]);
	else
		vid.conwidth = 640;

	vid.conwidth &= 0xfff8; // make it a multiple of eight

	if (vid.conwidth < 320)
		vid.conwidth = 320;

	// pick a conheight that matches with correct aspect
	vid.conheight = vid.conwidth*3 / 4;

	if ((i = COM_CheckParm("-conheight")) != 0)
		vid.conheight = SDL_atoi(com_argv[i+1]);
	if (vid.conheight < 200)
		vid.conheight = 200;

	vid.maxwarpwidth = WARP_WIDTH;
	vid.maxwarpheight = WARP_HEIGHT;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));

	Check_Gamma(palette);
	VID_SetPalette (palette);

	vid.conwidth = vid.width;
	vid.conheight = vid.height;

	GL_Init ();

	vid_menudrawfn = VID_MenuDraw;
	vid_menukeyfn = VID_MenuKey;

	if (COM_CheckParm("-fullsbar"))
		fullsbardraw = true;
}


//========================================================
// Video menu stuff
//========================================================

extern void M_Menu_Options_f (void);
extern void M_Print (int cx, int cy, char *str);
extern void M_PrintWhite (int cx, int cy, char *str);
extern void M_DrawCharacter (int cx, int line, int num);
extern void M_DrawTransPic (int x, int y, qpic_t *pic);
extern void M_DrawPic (int x, int y, qpic_t *pic);

typedef struct
{
	int		modenum;
	char	*desc;
	int		iscur;
} modedesc_t;

#define MAX_COLUMN_SIZE		9
#define MODE_AREA_HEIGHT	(MAX_COLUMN_SIZE + 2)
#define MAX_MODEDESCS		(MAX_COLUMN_SIZE*3)

static modedesc_t	modedescs[MAX_MODEDESCS];

/*
================
VID_MenuDraw
================
*/
void VID_MenuDraw (void)
{
	SDL_PixelFormat format;
	qpic_t		*p;
	int			i, column, row, w, h, bpp;
	char modestring[64];

	p = Draw_CachePic ("gfx/vidmodes.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	M_Print (2*8, 36+0*8, "Fullscreen Modes (WIDTHxHEIGHTxBPP)");

	column = 8;
	row = 36+2*8;

	SDL_GetWindowSize(window, &w, &h);
	format = SDL_GetWindowPixelFormat(window);
	bpp = SDL_BITSPERPIXEL(format);

	SDL_snprintf(modestring, sizeof(modestring), "%dx%dx%d", w, h, bpp);
	M_PrintWhite (column, row, modedescs[i].desc);

	M_Print (3*8, 36 + MODE_AREA_HEIGHT * 8 + 8*2,
			 "Video modes must be set from the");
	M_Print (3*8, 36 + MODE_AREA_HEIGHT * 8 + 8*3,
			 "command line with -width <width>");
	M_Print (3*8, 36 + MODE_AREA_HEIGHT * 8 + 8*4,
			 "and -bpp <bits-per-pixel>");
	M_Print (3*8, 36 + MODE_AREA_HEIGHT * 8 + 8*6,
			 "Select windowed mode with -window");
}


/*
================
VID_MenuKey
================
*/
void VID_MenuKey (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		S_LocalSound ("misc/menu1.wav");
		M_Menu_Options_f ();
		break;

	default:
		break;
	}
}

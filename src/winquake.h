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
// winquake.h: Win32-specific Quake header file

#ifdef _WIN32 

#include <SDL3/SDL.h>
#define WM_MOUSEWHEEL                   0x020A


extern	HINSTANCE	global_hInstance;
extern	int			global_nCmdShow;


extern bool			DDActive;

extern uint32_t gSndBufSize;
//#define SNDBUFSIZE 65536


typedef enum {MS_WINDOWED, MS_FULLSCREEN, MS_FULLDIB, MS_UNINIT} modestate_t;

extern modestate_t	modestate;

extern HWND			mainwindow;
extern bool		ActiveApp, Minimized;

extern bool	WinNT;

void IN_ShowMouse (void);
void IN_DeactivateMouse (void);
void IN_HideMouse (void);
void IN_ActivateMouse (void);
void IN_MouseEvent (void);
void IN_Accumulate (float dx, float dy);

extern bool	winsock_lib_initialized;

extern int		window_center_x, window_center_y;

extern bool	mouseinitialized;
extern HWND		hwnd_dialog;

extern HANDLE	hinput, houtput;

void IN_UpdateClipCursor (void);
void CenterWindow(HWND hWndCenter, int width, int height, BOOL lefttopjustify);

void S_BlockSound (void);
void S_UnblockSound (void);

void VID_SetDefaultMode (void);

#endif

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
// in_win.c -- windows 95 mouse and joystick code
// 02/21/97 JCB Added extended DirectInput code to support external controllers.

#include "quakedef.h"
#include "winquake.h"

#define mlook_active	(freelook.value || (in_mlook.state&1))

extern SDL_Window *window;

qboolean ActiveApp = true;
qboolean Minimized = false;

int			mouse_buttons;
int			mouse_oldbuttonstate;
POINT		current_pos;
int			mouse_x, mouse_y, old_mouse_x, old_mouse_y;

float mx_accum, my_accum;

static qboolean	mouseparmsvalid, mouseactivatetoggle;
static qboolean	mouseshowtoggle = 1;

qboolean	mouseactive;

static float maxpitch = 80.0f;
static float minpitch = -70.0f;

static const int buttonremap[] =
{
	K_MOUSE1,
	K_MOUSE3,
	K_MOUSE2,
};

static int ScancodeToQuakeKey(SDL_Scancode scancode)
{
	switch (scancode)
	{
	case SDL_SCANCODE_TAB: return K_TAB;
	case SDL_SCANCODE_RETURN: return K_ENTER;
	case SDL_SCANCODE_RETURN2: return K_ENTER;
	case SDL_SCANCODE_ESCAPE: return K_ESCAPE;
	case SDL_SCANCODE_SPACE: return K_SPACE;

	case SDL_SCANCODE_A: return 'a';
	case SDL_SCANCODE_B: return 'b';
	case SDL_SCANCODE_C: return 'c';
	case SDL_SCANCODE_D: return 'd';
	case SDL_SCANCODE_E: return 'e';
	case SDL_SCANCODE_F: return 'f';
	case SDL_SCANCODE_G: return 'g';
	case SDL_SCANCODE_H: return 'h';
	case SDL_SCANCODE_I: return 'i';
	case SDL_SCANCODE_J: return 'j';
	case SDL_SCANCODE_K: return 'k';
	case SDL_SCANCODE_L: return 'l';
	case SDL_SCANCODE_M: return 'm';
	case SDL_SCANCODE_N: return 'n';
	case SDL_SCANCODE_O: return 'o';
	case SDL_SCANCODE_P: return 'p';
	case SDL_SCANCODE_Q: return 'q';
	case SDL_SCANCODE_R: return 'r';
	case SDL_SCANCODE_S: return 's';
	case SDL_SCANCODE_T: return 't';
	case SDL_SCANCODE_U: return 'u';
	case SDL_SCANCODE_V: return 'v';
	case SDL_SCANCODE_W: return 'w';
	case SDL_SCANCODE_X: return 'x';
	case SDL_SCANCODE_Y: return 'y';
	case SDL_SCANCODE_Z: return 'z';

	case SDL_SCANCODE_1: return '1';
	case SDL_SCANCODE_2: return '2';
	case SDL_SCANCODE_3: return '3';
	case SDL_SCANCODE_4: return '4';
	case SDL_SCANCODE_5: return '5';
	case SDL_SCANCODE_6: return '6';
	case SDL_SCANCODE_7: return '7';
	case SDL_SCANCODE_8: return '8';
	case SDL_SCANCODE_9: return '9';
	case SDL_SCANCODE_0: return '0';

	case SDL_SCANCODE_MINUS: return '-';
	case SDL_SCANCODE_EQUALS: return '=';
	case SDL_SCANCODE_LEFTBRACKET: return '[';
	case SDL_SCANCODE_RIGHTBRACKET: return ']';
	case SDL_SCANCODE_BACKSLASH: return '\\';
	case SDL_SCANCODE_NONUSHASH: return '#';
	case SDL_SCANCODE_SEMICOLON: return ';';
	case SDL_SCANCODE_APOSTROPHE: return '\'';
	case SDL_SCANCODE_GRAVE: return '`';
	case SDL_SCANCODE_COMMA: return ',';
	case SDL_SCANCODE_PERIOD: return '.';
	case SDL_SCANCODE_SLASH: return '/';
	case SDL_SCANCODE_NONUSBACKSLASH: return '\\';

	case SDL_SCANCODE_BACKSPACE: return K_BACKSPACE;
	case SDL_SCANCODE_UP: return K_UPARROW;
	case SDL_SCANCODE_DOWN: return K_DOWNARROW;
	case SDL_SCANCODE_LEFT: return K_LEFTARROW;
	case SDL_SCANCODE_RIGHT: return K_RIGHTARROW;

	case SDL_SCANCODE_LALT: return K_ALT;
	case SDL_SCANCODE_RALT: return K_ALT;
	case SDL_SCANCODE_LCTRL: return K_CTRL;
	case SDL_SCANCODE_RCTRL: return K_CTRL;
	case SDL_SCANCODE_LSHIFT: return K_SHIFT;
	case SDL_SCANCODE_RSHIFT: return K_SHIFT;

	case SDL_SCANCODE_F1: return K_F1;
	case SDL_SCANCODE_F2: return K_F2;
	case SDL_SCANCODE_F3: return K_F3;
	case SDL_SCANCODE_F4: return K_F4;
	case SDL_SCANCODE_F5: return K_F5;
	case SDL_SCANCODE_F6: return K_F6;
	case SDL_SCANCODE_F7: return K_F7;
	case SDL_SCANCODE_F8: return K_F8;
	case SDL_SCANCODE_F9: return K_F9;
	case SDL_SCANCODE_F10: return K_F10;
	case SDL_SCANCODE_F11: return K_F11;
	case SDL_SCANCODE_F12: return K_F12;
	case SDL_SCANCODE_INSERT: return K_INS;
	case SDL_SCANCODE_DELETE: return K_DEL;
	case SDL_SCANCODE_PAGEDOWN: return K_PGDN;
	case SDL_SCANCODE_PAGEUP: return K_PGUP;
	case SDL_SCANCODE_HOME: return K_HOME;
	case SDL_SCANCODE_END: return K_END;
	default: return 0;
	}
}


/*
===========
Force_CenterView_f
===========
*/
void Force_CenterView_f (void)
{
	cl.viewangles[PITCH] = 0;
}


/*
===========
IN_UpdateClipCursor
===========
*/
void IN_UpdateClipCursor (void)
{
/*
	if (mouseinitialized && mouseactive && !dinput)
	{
		ClipCursor (&window_rect);
	}
	*/
}


/*
===========
IN_ShowMouse
===========
*/
void IN_ShowMouse (void)
{
	if (!mouseshowtoggle)
	{
		SDL_ShowCursor();
		mouseshowtoggle = 1;
	}
}


/*
===========
IN_HideMouse
===========
*/
void IN_HideMouse (void)
{
	if (mouseshowtoggle)
	{
		SDL_HideCursor();
		mouseshowtoggle = 0;
	}
}


/*
===========
IN_ActivateMouse
===========
*/
void IN_ActivateMouse (void)
{
	mouseactivatetoggle = true;

	SDL_SetWindowRelativeMouseMode(window, true);

	mouseactive = true;
}

/*
===========
IN_DeactivateMouse
===========
*/
void IN_DeactivateMouse (void)
{
	SDL_SetWindowRelativeMouseMode(window, false);

	mouseactivatetoggle = false;
	mouseactive = false;
}

/*
===========
IN_StartupMouse
===========
*/
void IN_StartupMouse (void)
{
	mouse_buttons = 3;

// if a fullscreen video mode was set before the mouse was initialized,
// set the mouse state appropriately
	if (mouseactivatetoggle)
		IN_ActivateMouse ();
}

void KeyDown (kbutton_t *b);

/*
===========
IN_Init
===========
*/
void IN_Init (void)
{
	Cmd_AddCommand ("force_centerview", Force_CenterView_f);

	KeyDown(&in_mlook);
	IN_StartupMouse ();
}

/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown (void)
{
	IN_DeactivateMouse ();
	IN_ShowMouse ();
}


/*
===========
IN_MouseMove
===========
*/
void IN_MouseMove (usercmd_t *cmd)
{
	static float old_mouse_x = 0, old_mouse_y = 0;

	//
	// Do not move the player if we're in HUD editor or menu mode.
	// And don't apply ingame sensitivity, since that will make movements jerky.
	//
	if (key_dest == key_game)
	{
		// Normal game mode.
		float mouse_x, mouse_y;

		mouse_x = mx_accum;
		mouse_y = my_accum;

		old_mouse_x = mx_accum;
		old_mouse_y = my_accum;

		mouse_x *= sensitivity.value;
		mouse_y *= sensitivity.value;

		// add mouse X/Y movement to cmd
		if ((in_strafe.state & 1) || (lookstrafe.value && mlook_active))
			cmd->sidemove += m_side.value * mouse_x;
		else if (!cl.paused || cls.demoplayback || cl.spectator)
			cl.viewangles[YAW] -= m_yaw.value * mouse_x;

		if (mlook_active)
			V_StopPitchDrift ();

		if (mlook_active && !(in_strafe.state & 1))
		{
			if (!cl.paused || cls.demoplayback || cl.spectator) {
				cl.viewangles[PITCH] += m_pitch.value * mouse_y;
			}
			if (cl.viewangles[PITCH] > maxpitch)
				cl.viewangles[PITCH] = maxpitch;
			if (cl.viewangles[PITCH] < minpitch)
				cl.viewangles[PITCH] = minpitch;
		} else {
			cmd->forwardmove -= m_forward.value * mouse_y;
		}
	}
	else {
		old_mouse_x = mx_accum * cursor_sensitivity.value;
		old_mouse_y = my_accum * cursor_sensitivity.value;
	}

	mx_accum = my_accum = 0; // clear for next update
}


/*
===========
IN_Move
===========
*/
void IN_Move (usercmd_t *cmd)
{
	if (ActiveApp && !Minimized)
	{
		IN_MouseMove (cmd);
	}
}


/*
===========
IN_Accumulate
===========
*/
void IN_Accumulate (float dx, float dy)
{
	if (mouseactive)
	{
		mx_accum += dx;
		my_accum += dy;
	}
}


/*
===================
IN_ClearStates
===================
*/
void IN_ClearStates (void)
{
	if (mouseactive)
	{
		mx_accum = 0;
		my_accum = 0;
		mouse_oldbuttonstate = 0;
	}
}

void IN_SendKeyEvents(void)
{
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_EVENT_MOUSE_MOTION:
				IN_Accumulate(event.motion.xrel, event.motion.yrel);
				break;
			case SDL_EVENT_KEY_UP:
			case SDL_EVENT_KEY_DOWN:
				Key_Event (ScancodeToQuakeKey(event.key.scancode), event.key.down);
			break;
			case SDL_EVENT_MOUSE_BUTTON_UP:
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				if (event.button.button < 3) {
					Key_Event (buttonremap[event.button.button], event.button.down);
				}
			break;
			case SDL_EVENT_MOUSE_WHEEL:
				if (event.wheel.y > 0) {
					Key_Event(K_MWHEELUP, true);
					Key_Event(K_MWHEELUP, false);
				}
				else if (event.wheel.y < 0) {
					Key_Event(K_MWHEELDOWN, true);
					Key_Event(K_MWHEELDOWN, false);
				}
			break;
			case SDL_EVENT_WINDOW_RESIZED:
				VID_WindowResized();
			break;
			case SDL_EVENT_WINDOW_MINIMIZED:
				Minimized = true;
				break;
			case SDL_EVENT_WINDOW_RESTORED:
				Minimized = false;
				break;
			case SDL_EVENT_WINDOW_FOCUS_LOST:
				ActiveApp = false;
				IN_DeactivateMouse();
			break;
			case SDL_EVENT_WINDOW_FOCUS_GAINED:
				ActiveApp = true;
				IN_ActivateMouse();
			break;
			case SDL_EVENT_QUIT:
				Sys_Quit();
			break;
			default:
				break;
		}
	}

}

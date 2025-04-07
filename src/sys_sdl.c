#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "quakedef.h"
#include "winquake.h"
#include "fcntl.h"
#include <limits.h>
#include <stdio.h>

#define MINIMUM_WIN_MEMORY	0x0c00000
#define MAXIMUM_WIN_MEMORY	0xfffffff

extern SDL_Window *window;

double Sys_DoubleTime (void)
{
	static SDL_Time starttime;
	static bool first = true;
	SDL_Time now;

	if (!SDL_GetCurrentTime(&now))
	{
		Sys_Error("Failed to get time: %s\n", SDL_GetError());
	}

	if (first) {
		first = false;
		starttime = now;
		return 0.0;
	}

	return (double)(now - starttime) / SDL_NS_PER_SECOND;
}

void Sys_mkdir (char *path)
{
	SDL_CreateDirectory (path);
}


void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	Host_Shutdown ();

	va_start (argptr, error);
	SDL_vsnprintf(text, sizeof(text), error, argptr);
	va_end (argptr);

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", text, NULL);

	exit (1);
}

void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;

	va_start (argptr,fmt);
	vprintf (fmt, argptr);
	va_end (argptr);
}

void Sys_DebugLog(char *file, char *fmt, ...)
{
	va_list argptr;
	SDL_IOStream *stream;

	stream = SDL_IOFromFile(file, "wb+");

	va_start(argptr, fmt);
	SDL_IOvprintf(stream, fmt, argptr);
	va_end(argptr);

	SDL_CloseIO(stream);
};

void Sys_Quit (void)
{
	CL_Disconnect ();
	Host_Shutdown();

	exit (0);
}

int	Sys_FileTime (char *path)
{
	FILE	*f;
	int		t, retval;

	f = fopen(path, "rb");

	if (f)
	{
		fclose(f);
		retval = 1;
	}
	else
	{
		retval = -1;
	}

	return retval;
}

void Sys_SendKeyEvents (void)
{
	IN_SendKeyEvents();
}

int main(int argc, char **argv)
{
   MSG				msg;
	quakeparms_t	parms;
	double			time, oldtime, newtime;
	char			*cwd;
	int				t;
	RECT			rect;

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
	{
		printf("Error: SDL_Init(): %s\n", SDL_GetError());
		return -1;
	}

	cwd = SDL_GetCurrentDirectory();

	parms.basedir = cwd;
	parms.cachedir = NULL;

	COM_InitArgv (argc, argv);

	parms.argc = com_argc;
	parms.argv = com_argv;

// take the greater of all the available memory or half the total memory,
// but at least 8 Mb and no more than 16 Mb, unless they explicitly
// request otherwise
	parms.memsize = SDL_GetSystemRAM() * 1024 * 1024;

	if (parms.memsize < MINIMUM_WIN_MEMORY)
		parms.memsize = MINIMUM_WIN_MEMORY;

	if (parms.memsize > MAXIMUM_WIN_MEMORY)
		parms.memsize = MAXIMUM_WIN_MEMORY;

	if (COM_CheckParm ("-heapsize"))
	{
		t = COM_CheckParm("-heapsize") + 1;

		if (t < com_argc)
			parms.memsize = Q_atoi (com_argv[t]) * 1024;
	}

	parms.membase = malloc (parms.memsize);

	if (!parms.membase)
		Sys_Error ("Not enough memory free; check disk space\n");

// because sound is off until we become active
	S_BlockSound ();

	Sys_Printf ("Host_Init\n");
	Host_Init (&parms);

	oldtime = (float) Sys_DoubleTime ();

    /* main window message loop */
	while (1)
	{
		newtime = Sys_DoubleTime ();
		time = newtime - oldtime;
		Host_Frame (time);
		oldtime = newtime;
	}

	SDL_free(cwd);

    /* return success of application */
    return TRUE;
}
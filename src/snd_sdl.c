/*
 * snd_sdl.c - SDL audio driver for Hexen II: Hammer of Thyrion (uHexen2)
 * based on implementations found in the quakeforge and ioquake3 projects.
 *
 * Copyright (C) 1999-2005 Id Software, Inc.
 * Copyright (C) 2005-2012 O.Sezer <sezero@users.sourceforge.net>
 * Copyright (C) 2010-2014 QuakeSpasm developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "quakedef.h"

static int buffersize;

static SDL_AudioStream *stream = NULL;

static void SDLCALL paint_audio(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
	int pos, tobufend;
	int len1, len2;

	if (!shm)
	{ /* shouldn't happen, but just in case */
		//SDL_ClearAudioStream(stream);
		return;
	}

	pos = (shm->samplepos * (shm->samplebits / 8));
	if (pos >= buffersize)
		shm->samplepos = pos = 0;

	tobufend = buffersize - pos; /* bytes to buffer's end. */
	len1 = additional_amount;
	len2 = 0;

	if (len1 > tobufend)
	{
		len1 = tobufend;
		len2 = additional_amount - len1;
	}

	SDL_PutAudioStreamData(stream, shm->buffer + pos, len1);

	if (len2 <= 0)
	{
		shm->samplepos += (len1 / (shm->samplebits / 8));
	}
	else
	{ /* wraparound? */
		SDL_PutAudioStreamData(stream, shm->buffer, len2);
		shm->samplepos = (len2 / (shm->samplebits / 8));
	}

	if (shm->samplepos >= buffersize)
		shm->samplepos = 0;
}

bool SNDDMA_Init (dma_t *dma)
{
	SDL_AudioSpec desired, obtained;
	int			  tmp, val;
	char		  drivername[128];
	int samples;

	if (SDL_InitSubSystem (SDL_INIT_AUDIO) < 0)
	{
		Con_Printf ("Couldn't init SDL audio: %s\n", SDL_GetError ());
		return false;
	}

	desired.freq = 44100;
	desired.format = SDL_AUDIO_S16;
	desired.channels = 2;

	stream = SDL_OpenAudioDeviceStream (SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired, paint_audio, NULL);
	if (!stream) {
		Con_Printf ("Couldn't open SDL audio: %s\n", SDL_GetError ());
		SDL_QuitSubSystem (SDL_INIT_AUDIO);
		return false;
	}

	if (!SDL_GetAudioDeviceFormat(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &obtained, &samples)) {
		Con_Printf ("Couldn't determine SDL audio format: %s\n", SDL_GetError ());
		SDL_DestroyAudioStream (stream);
		SDL_QuitSubSystem (SDL_INIT_AUDIO);
		return false;
	}

	memset ((void *)dma, 0, sizeof (dma_t));
	shm = dma;

	/* Fill the audio DMA information block */
	/* Since we passed NULL as the 'obtained' spec to SDL_OpenAudio(),
	 * SDL will convert to hardware format for us if needed, hence we
	 * directly use the desired values here. */
	shm->samplebits = SDL_AUDIO_BITSIZE(desired.format);
	shm->speed = desired.freq;
	shm->channels = desired.channels;
	tmp = (samples * desired.channels) * 10;
	if (tmp & (tmp - 1))
	{ /* make it a power of two */
		val = 1;
		while (val < tmp)
			val <<= 1;

		tmp = val;
	}
	shm->samples = tmp;
	shm->samplepos = 0;
	shm->submission_chunk = 1;

	Con_Printf ("SDL audio spec  : %d Hz, %d samples, %d channels\n", desired.freq, samples, desired.channels);
	{
		SDL_AudioDeviceID device_id = SDL_GetAudioStreamDevice(stream);
		const char *driver = SDL_GetCurrentAudioDriver ();
		const char *device = SDL_GetAudioDeviceName (device_id);
		SDL_snprintf (drivername, sizeof (drivername), "%s - %s", driver != NULL ? driver : "(UNKNOWN)", device != NULL ? device : "(UNKNOWN)");
	}
	buffersize = shm->samples * (shm->samplebits / 8);
	Con_Printf ("SDL audio driver: %s, %d bytes buffer\n", drivername, buffersize);

	shm->buffer = (unsigned char *)malloc (buffersize);
	if (!shm->buffer)
	{
		SDL_DestroyAudioStream (stream);
		SDL_QuitSubSystem (SDL_INIT_AUDIO);
		shm = NULL;
		Con_Printf ("Failed allocating memory for SDL audio\n");
		return false;
	}

	SDL_ResumeAudioStreamDevice(stream);

	return true;
}

int SNDDMA_GetDMAPos (void)
{
	return shm->samplepos;
}

void SNDDMA_Shutdown (void)
{
	if (shm)
	{
		Con_Printf ("Shutting down SDL sound\n");
		SDL_DestroyAudioStream (stream);
		SDL_QuitSubSystem (SDL_INIT_AUDIO);
		if (shm->buffer)
			free (shm->buffer);
		shm->buffer = NULL;
		shm = NULL;
	}
}

void SNDDMA_LockBuffer (void)
{
	SDL_LockAudioStream(stream);
}

void SNDDMA_Submit (void)
{
	SDL_UnlockAudioStream(stream);
}

void SNDDMA_BlockSound (void)
{
	SDL_PauseAudioStreamDevice(stream);
}

void SNDDMA_UnblockSound (void)
{
	SDL_ResumeAudioStreamDevice(stream);
}

#include <SDL2/SDL.h>

int main(int argc, char ** argv)
{
	SDL_Init(SDL_INIT_AUDIO);

	// load WAV file

	SDL_AudioSpec wavSpec;
	Uint32 wavLength;
	Uint8 *wavBuffer;

	SDL_LoadWAV("res/gun.wav", &wavSpec, &wavBuffer, &wavLength);
	
	// open audio device

	SDL_AudioDeviceID deviceId = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL, 0);

	// play audio

	int success = SDL_QueueAudio(deviceId, wavBuffer, wavLength);
	SDL_PauseAudioDevice(deviceId, 0);

	// keep window open enough to hear the sound

	SDL_Delay(3000);

	// clean up

	SDL_CloseAudioDevice(deviceId);
	SDL_FreeWAV(wavBuffer);
	SDL_Quit();

	return 0;
}

#include "Audio.hpp"

bool audio_init(AudioContext &audio_context) {
  audio_context.device = alcOpenDevice(nullptr);
  audio_context.context = alcCreateContext(audio_context.device, nullptr);
  audio_make_current(audio_context);
  return true;
}

bool audio_make_current(AudioContext &audio_context) {
  alcMakeContextCurrent(audio_context.context);
  return true;
}

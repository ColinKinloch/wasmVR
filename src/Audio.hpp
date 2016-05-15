#pragma once

#include <AL/al.h>
#include <AL/alc.h>

struct AudioContext {
  ALCdevice *device;
  ALCcontext *context;
};

bool audio_init(AudioContext &context);
bool audio_make_current(AudioContext &context);

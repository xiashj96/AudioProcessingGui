#pragma once
#include "fmod.hpp"

typedef struct FMOD_DSP_PARAMETER_WAVEFORM
{
	float *buffer[2];	// all buffer data de-interleaved, support up to 2 channels
	int   length_samples; // number of samples in every channel
	int   numchannels; // number of channels
} FMOD_DSP_PARAMETER_WAVEFORM;

FMOD_DSP_DESCRIPTION getWaveformDSPDesc();
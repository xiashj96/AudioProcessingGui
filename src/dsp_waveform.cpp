#include "dsp_waveform.h"
#include <memory>

FMOD_RESULT F_CALLBACK WaveformDSPCallback(FMOD_DSP_STATE *dsp_state, float *inbuffer, float *outbuffer, unsigned int length, int inchannels, int *outchannels)
{
	FMOD_DSP_PARAMETER_WAVEFORM *data = (FMOD_DSP_PARAMETER_WAVEFORM *)dsp_state->plugindata;

	/*
	This loop assumes inchannels = outchannels, which it will be if the DSP is created with '0'
	as the number of channels in FMOD_DSP_DESCRIPTION.
	Specifying an actual channel count will mean you have to take care of any number of channels coming in,
	but outputting the number of channels specified. Generally it is best to keep the channel
	count at 0 for maximum compatibility.
	*/
	for (unsigned int samp = 0; samp < length; samp++)
	{
		/*
		Feel free to unroll this.
		*/
		for (int chan = 0; chan < *outchannels; chan++)
		{
			data->buffer[chan][samp] =
				outbuffer[(samp * inchannels) + chan] = inbuffer[(samp * inchannels) + chan];
		}
	}

	data->numchannels = inchannels;

	return FMOD_OK;
}

/*
Callback called when DSP is created.   This implementation creates a structure which is attached to the dsp state's 'plugindata' member.
*/
FMOD_RESULT F_CALLBACK WaveformDSPCreateCallback(FMOD_DSP_STATE *dsp_state)
{
	unsigned int blocksize;
	FMOD_RESULT result;

	result = dsp_state->functions->getblocksize(dsp_state, &blocksize);

	FMOD_DSP_PARAMETER_WAVEFORM *data = new FMOD_DSP_PARAMETER_WAVEFORM;
	if (!data)
	{
		return FMOD_ERR_MEMORY;
	}
	dsp_state->plugindata = data;
	data->length_samples = blocksize;

	data->buffer[0] = new float[blocksize];
	data->buffer[1] = new float[blocksize];

	if (!data->buffer)
	{
		return FMOD_ERR_MEMORY;
	}

	return FMOD_OK;
}

/*
Callback called when DSP is destroyed.   The memory allocated in the create callback can be freed here.
*/
FMOD_RESULT F_CALLBACK WaveformDSPReleaseCallback(FMOD_DSP_STATE *dsp_state)
{
	if (dsp_state->plugindata)
	{
		FMOD_DSP_PARAMETER_WAVEFORM *data = (FMOD_DSP_PARAMETER_WAVEFORM *)dsp_state->plugindata;

		if (data->buffer)
		{
			delete[] data->buffer[0];
			delete[] data->buffer[1];
		}

		delete data;
	}

	return FMOD_OK;
}

/*
Callback called when DSP::getParameterData is called.   This returns a pointer to the raw floating point PCM data.
We have set up 'parameter 0' to be the data parameter, so it checks to make sure the passed in index is 0, and nothing else.
*/
FMOD_RESULT F_CALLBACK WaveformDSPGetParameterDataCallback(FMOD_DSP_STATE *dsp_state, int index, void **data, unsigned int *length, char *)
{
	if (index == 0)
	{
		unsigned int blocksize;
		FMOD_RESULT result;
		FMOD_DSP_PARAMETER_WAVEFORM *mydata = (FMOD_DSP_PARAMETER_WAVEFORM *)dsp_state->plugindata;

		result = dsp_state->functions->getblocksize(dsp_state, &blocksize);

		*data = (void *)mydata;
		*length = blocksize * 2 * sizeof(float);

		return FMOD_OK;
	}

	return FMOD_ERR_INVALID_PARAM;
}


FMOD_DSP_DESCRIPTION getWaveformDSPDesc()
{
	FMOD_DSP_DESCRIPTION dspdesc;
	memset(&dspdesc, 0, sizeof(dspdesc));

	FMOD_DSP_PARAMETER_DESC wavedata_desc;
	FMOD_DSP_PARAMETER_DESC *paramdesc[1] =
	{
		&wavedata_desc,
	};

	FMOD_DSP_INIT_PARAMDESC_DATA(wavedata_desc, "WAVEFORM_DATA", "", "WAVEFORM_DATA", FMOD_DSP_PARAMETER_DATA_TYPE_USER);

	strncpy(dspdesc.name, "Waveform DSP unit", sizeof(dspdesc.name));
	dspdesc.version = 0x00010000;
	dspdesc.numinputbuffers = 1;
	dspdesc.numoutputbuffers = 1;
	dspdesc.read = WaveformDSPCallback;
	dspdesc.create = WaveformDSPCreateCallback;
	dspdesc.release = WaveformDSPReleaseCallback;
	dspdesc.getparameterdata = WaveformDSPGetParameterDataCallback;
	dspdesc.numparameters = 1;
	dspdesc.paramdesc = paramdesc;

	return dspdesc;
}
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/audio/audio.h"
#include "cinder/Log.h"

#include "fmod.hpp"
#include "fmod_errors.h"
#include "dsp_waveform.h"

#include "Plot.h"
#include "utility.h"

#include "CinderImGui.h"


using namespace ci;
using namespace ci::app;
using namespace std;

class AudioProcessingGuiApp : public App {
public:
	void setup() override;
	void mouseDown(MouseEvent event) override;
	void update() override;
	void draw() override;
	void cleanup() override;

	const int fftSize = 1024;

	// timer
	Timer timer;

	vector<string> musicFiles;
	void GetMusicFiles();

	// FMOD sound system
	FMOD::System* fmodSystem = nullptr;
	FMOD::Sound* music = nullptr;
	FMOD::ChannelGroup* masterGroup = nullptr;
	FMOD::Channel* musicChannel = nullptr;

	// DSPs
	FMOD::DSP* threeEq;
	FMOD::DSP* distortion;
	FMOD::DSP* flange;
	FMOD::DSP* chorus;
	FMOD::DSP* fft;
	FMOD::DSP* waveform;

	bool threeEqBypass;
	bool distortionBypass;
	bool flangeBypass;
	bool chorusBypass;

	FMOD_DSP_PARAMETER_FFT *fftResult;
	FMOD_DSP_PARAMETER_WAVEFORM *waveformResult;

	vector<float> spectrum;
	vector<float> waveformL;
	vector<float> waveformR;

	// ui
	void MusicControlWindow();
	void UpdateParameters();
	// parameters
	float sVolume = 1;		//[0,1]
	float sPlayRate = 1;		//[0,3]
	float sPan = 0;			//[-1,1]

	float sLowVol = 1;		//[0.001,1]
	float sMidVol = 1;		//[0.001,1]
	float sHighVol = 1;		//[0.001,1]

	float sDistortionLevel = 0;	//[0,1]

	float sFlangeMix = 0;	//[0,100]
	float sFlangeRate = 0.1;	//[0,20]

	float sChorusMix = 0;	//[0,100]
	float sChorosRate = 0.8;	//[0,20]
	float sChorusDepth = 3;	//[0,100]
	
};

void AudioProcessingGuiApp::MusicControlWindow()
{
	ui::ScopedWindow window("Music Controls");

	ui::SliderFloat("Main Volume", &sVolume, 0, 1);
	ui::SliderFloat("Play Rate", &sPlayRate, 0, 3);
	ui::SliderFloat("Pan", &sPan, -1, 1);
	if (ui::CollapsingHeader("Three Band Equilizer"))
	{
		threeEqBypass = false;
		ui::SliderFloat("Low Volume", &sLowVol, 0.001, 1);
		ui::SliderFloat("Mid Volume", &sMidVol, 0.001, 1);
		ui::SliderFloat("High Volume", &sHighVol, 0.001, 1);
	}
	else {
		threeEqBypass = true;
	}
	if (ui::CollapsingHeader("Distortion"))
	{
		distortionBypass = false;
		ui::SliderFloat("Distortion", &sDistortionLevel, 0, 1);
	}
	else {
		distortionBypass = true;
	}
	if (ui::CollapsingHeader("Flange"))
	{
		flangeBypass = false;
		ui::SliderFloat("Flange Mix", &sFlangeMix, 0, 100);
		ui::SliderFloat("Flange Rate", &sFlangeRate, 0, 20);
	}
	else {
		flangeBypass = true;
	}
	if (ui::CollapsingHeader("Chorus"))
	{
		chorusBypass = false;
		ui::SliderFloat("Chorus Mix", &sChorusMix, 0, 100);
		ui::SliderFloat("Chorus Rate", &sChorosRate, 0, 20);
		ui::SliderFloat("Chorus Depth", &sChorusDepth, 0, 100);
	}
	else {
		chorusBypass = true;
	}
	if (ui::CollapsingHeader("Change Music"))
	{
		for (const auto& music_file : musicFiles)
		{
			if (ui::Button(music_file.c_str()))
			{
				musicChannel->stop();
				music->release();
				auto music_path = getAssetPath(music_file).string();
				fmodSystem->createSound(music_path.c_str(), FMOD_LOOP_OFF | FMOD_2D | FMOD_CREATESTREAM, nullptr, &music);
				fmodSystem->playSound(music, nullptr, false, &musicChannel);
			}
		}
	}
}

void AudioProcessingGuiApp::UpdateParameters()
{
	musicChannel->setVolume(sVolume);
	musicChannel->setPitch(sPlayRate);
	musicChannel->setPan(sPan);

	threeEq->setBypass(threeEqBypass);
	threeEq->setParameterFloat(FMOD_DSP_THREE_EQ_LOWGAIN, linear2dB(sLowVol));
	threeEq->setParameterFloat(FMOD_DSP_THREE_EQ_MIDGAIN, linear2dB(sMidVol));
	threeEq->setParameterFloat(FMOD_DSP_THREE_EQ_HIGHGAIN, linear2dB(sHighVol));

	distortion->setBypass(distortionBypass);
	distortion->setParameterFloat(FMOD_DSP_DISTORTION_LEVEL, sDistortionLevel);

	flange->setBypass(flangeBypass);
	flange->setParameterFloat(FMOD_DSP_FLANGE_MIX, sFlangeMix);
	flange->setParameterFloat(FMOD_DSP_FLANGE_RATE, sFlangeRate);

	chorus->setBypass(chorusBypass);
	chorus->setParameterFloat(FMOD_DSP_CHORUS_MIX, sChorusMix);
	chorus->setParameterFloat(FMOD_DSP_CHORUS_RATE, sChorosRate);
	chorus->setParameterFloat(FMOD_DSP_CHORUS_DEPTH, sChorusDepth);
}

void AudioProcessingGuiApp::setup()
{
	timer.start();

	// initialize Fmod
	FMOD::System_Create(&fmodSystem);
	fmodSystem->init(1, FMOD_INIT_NORMAL, nullptr); // we only need 1 music play at a time
	fmodSystem->getMasterChannelGroup(&masterGroup);

	GetMusicFiles();

	// stream music file
	auto music_path = getAssetPath(musicFiles[0]).string();
	fmodSystem->createSound(music_path.c_str(), FMOD_LOOP_OFF | FMOD_2D | FMOD_CREATESTREAM, nullptr, &music);
	fmodSystem->playSound(music, nullptr, true, &musicChannel); // pause at start

	// create and connect DSP effect
	// 1. 3-band eq
	fmodSystem->createDSPByType(FMOD_DSP_TYPE_THREE_EQ, &threeEq);
	masterGroup->addDSP(0, threeEq);

	// 2. distortion
	fmodSystem->createDSPByType(FMOD_DSP_TYPE_DISTORTION, &distortion);
	masterGroup->addDSP(0, distortion);
	distortion->setParameterFloat(FMOD_DSP_DISTORTION_LEVEL, sDistortionLevel);

	// 3. flange
	fmodSystem->createDSPByType(FMOD_DSP_TYPE_FLANGE, &flange);
	masterGroup->addDSP(0, flange);
	flange->setParameterFloat(FMOD_DSP_FLANGE_MIX, sFlangeMix); // at first there is no flange

	// 4. chorus
	fmodSystem->createDSPByType(FMOD_DSP_TYPE_CHORUS, &chorus);
	masterGroup->addDSP(0, chorus);
	chorus->setParameterFloat(FMOD_DSP_CHORUS_MIX, sChorusMix); // at first there is no chorus

	// 5.fft
	fmodSystem->createDSPByType(FMOD_DSP_TYPE_FFT, &fft);
	masterGroup->addDSP(0, fft);
	fft->setParameterInt(FMOD_DSP_FFT_WINDOWSIZE, fftSize);
	fft->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void **)&fftResult, nullptr, nullptr, 0);

	// 6. custom waveform DSP
	auto waveformDesc = getWaveformDSPDesc();
	fmodSystem->createDSP(&waveformDesc, &waveform);
	masterGroup->addDSP(0, waveform);
	waveform->getParameterData(0, (void**)&waveformResult, nullptr, nullptr, 0);

	musicChannel->setPaused(false);

	ui::initialize();
}

void AudioProcessingGuiApp::mouseDown(MouseEvent event)
{

}

void AudioProcessingGuiApp::update()
{
	FMOD_RESULT result;

	// if music is playing, get fft and waveform
	bool isPlaying;
	musicChannel->isPlaying(&isPlaying);
	if (isPlaying && timer.getSeconds() > 0.5) // wait 0.5s on start, otherwise will crash
	{
		spectrum.clear();
		for (int i = 0; i < fftSize / 4; ++i) // fftResult->spectrum has 2 channels, average to get spectrum
		{
			// result should belong to [0,1]
			spectrum.push_back((fftResult->spectrum[0][i] + fftResult->spectrum[1][i]) / 2);
		}

		waveformL.clear();
		waveformR.clear();
		for (int i = 0; i < waveformResult->length_samples; ++i)
		{
			waveformL.push_back(waveformResult->buffer[0][i]);
			waveformR.push_back(waveformResult->buffer[1][i]);
		}
	}

	MusicControlWindow();

	// update Fmod
	UpdateParameters();
	result = fmodSystem->update();
	if (result != FMOD_OK)
	{
		CI_LOG_E(FMOD_ErrorString(result));
	}
}

void AudioProcessingGuiApp::draw()
{
	gl::clear(Color(0, 0, 0));

	plotLine(
		waveformL,
		{ 0.f,0.f, (float)getWindow()->getWidth(), (float)getWindow()->getHeight() / 4.f },
		-1.f, 1.f);
	plotLine(
		waveformR,
		{ 0.f,(float)getWindow()->getHeight() / 4.f, (float)getWindow()->getWidth(), (float)getWindow()->getHeight() / 2.f },
		-1.f, 1.f);
	plotHistogram(
		spectrum,
		{ 0.f,(float)getWindow()->getHeight() / 2.f, (float)getWindow()->getWidth(), (float)getWindow()->getHeight() },
		0.f, 0.3f);

	// fps
	gl::drawString("FPS: " + std::to_string((int)getAverageFps()), { getWindowWidth() - 100, getWindowHeight() - 100 });
}

void AudioProcessingGuiApp::cleanup()
{
	music->release();
	fmodSystem->release();
}

void AudioProcessingGuiApp::GetMusicFiles()
{
	namespace fs = experimental::filesystem;
	fs::directory_iterator asset_folder{ getAssetDirectories()[0] };
	
	for (auto file : asset_folder)
	{
		if (file.path().extension() == ".mp3")
		{
			//console() << file.path().filename();
			musicFiles.push_back(file.path().filename().string());
		}
	}
}

CINDER_APP(AudioProcessingGuiApp, RendererGl{ RendererGl::Options().version(4,3).msaa(4) }, [](App::Settings *settings) {
	settings->setWindowSize({ 1280, 720 });
	settings->setTitle("Audio Processing");
	settings->setResizable(true);
	settings->setConsoleWindowEnabled(true);
});
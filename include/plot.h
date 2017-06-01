#pragma once
#include <vector>
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"

void plotLine(
	const std::vector<float>& data, 
	ci::Rectf frame, 
	float yMin, 
	float yMax, 
	bool drawFrame = false);

void plotHistogram(
	const std::vector<float>& data, 
	ci::Rectf frame,
	float yMin, 
	float yMax, 
	bool drawFrame = false);
#include "Plot.h"
using namespace ci;
using namespace std;

void plotLine(
	const vector<float>& data,
	Rectf frame,
	float yMin,
	float yMax,
	bool drawFrame)
{
	// generate polyline
	PolyLine2 line;
	float step_x = frame.getWidth() / (data.size() - 1);
	float x = 0;
	for (float data_point : data)
	{
		float y = ci::lmap(data_point, yMin, yMax, 0.f, 1.f) * frame.getHeight();
		line.push_back({ x, y });
		x += step_x;
	}

	// draw stuff
	if (drawFrame)
	{
		gl::drawStrokedRect(frame);
	}
	gl::pushMatrices();
	gl::translate({ frame.x1 ,frame.getHeight() + frame.y1 });
	gl::scale({ 1, -1 });
	gl::draw(line);
	gl::popMatrices();
}

void plotHistogram(
	const std::vector<float>& data,
	ci::Rectf frame,
	float yMin,
	float yMax,
	bool drawFrame)
{
	// draw stuff
	if (drawFrame)
	{
		gl::drawStrokedRect(frame);
	}
	gl::pushMatrices();
	gl::translate({ frame.x1 ,frame.getHeight() + frame.y1 });
	gl::scale({ 1, -1 });
	// histogram is just little rectangular bars
	float step_x = frame.getWidth() / data.size();
	float x = 0;
	for (float data_point : data)
	{
		float y = ci::lmap(data_point, yMin, yMax, 0.f, 1.f) * frame.getHeight();
		gl::drawSolidRect({ x,0,x + step_x,y });
		x += step_x;
	}
	gl::popMatrices();
}
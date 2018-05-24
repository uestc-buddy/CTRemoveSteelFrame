#pragma once
#include <string>
#include <opencv2/opencv.hpp>

/*
*	本类处理和去除脑部CT中出现的钢圈
*/

class CRemoveSteelFrame
{
public:
	CRemoveSteelFrame();
	CRemoveSteelFrame(std::string filepath);
	~CRemoveSteelFrame();

private:
	cv::Mat src_img;

public:
	bool CRemoveSteelFrame::RemoveFrame();
	unsigned char CRemoveSteelFrame::GetMeanValue(const cv::Mat& src_img);
	void getSizeContours(std::vector<std::vector<cv::Point>> &contours);
};


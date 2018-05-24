#include "stdafx.h"
#include "RemoveSteelFrame.h"

#include <iostream>
#include <string>
#include <exception>

using std::cout;
using std::endl;

CRemoveSteelFrame::CRemoveSteelFrame()
{
}


CRemoveSteelFrame::~CRemoveSteelFrame()
{
}

//************************************************************************
// 函数名称:    	CRemoveSteelFrame
// 访问权限:    	public 
// 创建日期:		2016/12/01
// 创 建 人:		NPC
// 函数说明:		加载图像的构造函数
// 函数参数: 	std::string filepath	文件路径
// 返 回 值:   	
//************************************************************************
CRemoveSteelFrame::CRemoveSteelFrame(std::string filepath)
{
	try
	{
		this->src_img = cv::imread(filepath, 0);
	}
	catch (std::exception *ex)
	{
		cout << ex << endl; 
	}
}

bool CRemoveSteelFrame::RemoveFrame()
{
	if (!src_img.data)
	{
		cout << "no image data" << endl;
		return false;
	}

	int rows(src_img.rows);
	int cols(src_img.cols);

	cv::imshow("origin_img", src_img);
	unsigned char meanvalue = this->GetMeanValue(src_img);	//计算图像的均值

	cv::Mat threshold_img;									//求取图像的二值化图像
	cv::threshold(src_img, threshold_img, meanvalue, 255, CV_THRESH_BINARY);
	cv::imshow("threshold_img", threshold_img);

	cv::Mat morphology_img;									//形态学变化
	cv::Mat element = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3,3));
	cv::erode(threshold_img, morphology_img, element);
	cv::dilate(morphology_img, morphology_img, element);
	cv::imshow("morphology_img", morphology_img);

	std::vector<std::vector<cv::Point>> contours;
	//CV_CHAIN_APPROX_NONE  获取每个轮廓每个像素点  
	cv::findContours(morphology_img, contours, CV_RETR_EXTERNAL/*CV_RETR_CCOMP*/, CV_CHAIN_APPROX_NONE, cvPoint(0, 0));
	getSizeContours(contours);				//去除点数过小的轮廓
	cout << contours.size() << endl;

	//所有的轮廓的图像
	cv::Mat all_contours(morphology_img.size(), CV_8UC1, cv::Scalar::all(0));
	cv::drawContours(all_contours, contours, -1, cv::Scalar(255), -1);   // -1 表示所有轮廓
	cv::imshow("all_contours", all_contours);

	//收集单个轮廓的图像
	std::vector<cv::Mat> contours_Vec;		//存储轮廓图像，
	int start_x(100000), pos_1(-1), end_x(-1), pos_2(-1);

	for (int i=0; i<contours.size(); i++)
	{
		cv::Mat temp(morphology_img.size(), CV_8UC1, cv::Scalar::all(0));
		cv::drawContours(temp, contours, i, cv::Scalar(255), -1);   // -1 表示所有轮廓
		contours_Vec.push_back(temp);

		bool stop_bit = false;
		unsigned char* data = nullptr;
		for (int k = (rows / 2); k < rows; k++)
		{
			if (stop_bit)
			{
				break;
			}

			data = temp.ptr<unsigned char>(k);
			int count(0);
			int start_x_temp(-1);
			for (int j = 0; j <= cols; j++)
			{
				if ((data[j] == 255))
				{
					start_x_temp = j;
					count++;
				}	
				else if (data[j] == 0)
				{
					if ((count >= 5) && (count <= 12))
					{
						if (start_x_temp < start_x)
						{
							start_x = start_x_temp;
							if (start_x <= 65)
							{
								pos_1 = i;
								stop_bit = true;
								break;
							}
						}
						if (j > end_x)
						{
							end_x = j;
							if (end_x >= (cols - 65))
							{
								pos_2 = i;
								stop_bit = true;
								break;
							}
						}
						stop_bit = true;
						break;
					}
					count = 0;
				}
			}
		}
	}

	//叠加之后的图像
	cv::Mat merge_img(morphology_img.size(), CV_8UC1, cv::Scalar::all(0));
	if ((pos_2 != -1) && (pos_1 != -1))
	{
		if (contours[pos_1].size() > contours[pos_2].size()+200)
		{
			merge_img = contours_Vec[pos_1].clone();
		}
		else if (contours[pos_2].size() > contours[pos_1].size()+200)
		{
			merge_img = contours_Vec[pos_2].clone();
		}
		else
		{
			addWeighted(contours_Vec[pos_1], 1.0, contours_Vec[pos_2], 1.0, 0.0, merge_img);
		}
	}
	else if ((pos_1 != -1) && (pos_2 == -1))
	{
		merge_img = contours_Vec[pos_1].clone();
	}
	else if ((pos_2 != -1) && (pos_1 == -1))
	{
		merge_img = contours_Vec[pos_2].clone();
	}
	cv::imshow("merge_img", merge_img);

	//在去除钢架之前将图像进行膨胀，保证能够将钢架完全去除
	cv::Mat absdiff_img(morphology_img.size(), CV_8UC1, cv::Scalar::all(0));
	cv::dilate(merge_img, merge_img, element, cv::Point(-1, -1), 3);
	unsigned char* src_data = nullptr;		//数据指针
	unsigned char* merge_data = nullptr;
	for (int i=0; i<rows; i++)
	{
		src_data = src_img.ptr<unsigned char>(i);
		merge_data = merge_img.ptr<unsigned char>(i);
		for (int j = 0; j < cols; j++)
		{
			int temp_value(src_data[j] - merge_data[j]);
			temp_value = temp_value<0?0:temp_value;
			src_data[j] = (unsigned char)temp_value;
		}
	}
	cv::imshow("absdiff_img", src_img);

	return true;
}

//************************************************************************
// 函数名称:    GetMeanValue
// 访问权限:    public 
// 创建日期:    2016/11/18
// 创 建 人:	    NPC
// 函数说明:    获取图像的平均灰度值
// 函数参数:    cv::Mat & src_img	输入图像
// 返 回 值:    double
//************************************************************************
unsigned char CRemoveSteelFrame::GetMeanValue(const cv::Mat& src_img)
{
	if (CV_8UC1 != src_img.type())
	{
		return -1.0;
	}

	int rows(src_img.rows);	//height
	int cols(src_img.cols);	//width
	unsigned char* data = nullptr;
	double PixelValueSum(0.0);	//总共的像素值

	for (int i = 0; i < rows; i++)
	{
		data = const_cast<unsigned char*>(src_img.ptr<unsigned char>(i));
		for (int j = 0; j < cols; j++)
		{
			PixelValueSum += (double)data[j];
		}	//计算图像的总共像素值
	}

	double result(PixelValueSum / static_cast<double>(rows*cols));	//计算图像的均值

	return (unsigned char)result;
}

// 移除过小或过大的轮廓  
void CRemoveSteelFrame::getSizeContours(std::vector<std::vector<cv::Point>> &contours)
{
	int cmin = 100;   // 最小轮廓长度  
	int cmax = 1000;   // 最大轮廓长度  
	std::vector<std::vector<cv::Point>>::const_iterator itc = contours.begin();
	while (itc != contours.end())
	{
		if ((itc->size()) < cmin /*|| (itc->size()) > cmax*/)
		{
			itc = contours.erase(itc);
		}
		else ++itc;
	}
}

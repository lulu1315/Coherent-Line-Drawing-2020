#include <stdio.h>
#include <opencv2/opencv.hpp>

using namespace std;

#define M_PI 3.14159265358979323846

class PP {
public:
	PP(cv::Size);
	void ETF(cv::Mat &flowfield, cv::Mat &dis ,int s,int noiseupsample);
    void ETF_image(cv::Mat &input, cv::Mat &flowfield, cv::Mat &dis ,int s);
	void FlowField(cv::Mat &flowfield, cv::Mat &dis);
	void AntiAlias(cv::Mat &src, cv::Mat &dis);
};

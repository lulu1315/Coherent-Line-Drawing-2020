#include <cstdio>
#include <opencv2/opencv.hpp>

#include "postProcessing.h"

using namespace std;
using namespace cv;
using std::cout;
using std::endl;

int main(int argc, char **argv)
{
     bool withpreview=0;
    //minimal usage
    if (argc == 1)
        {
        string usagetxt ="Usage : lic_image-cli inputframe flowfield outputframe iterations";
        string usagetxt1="        lic_image-cli /path/to/input/ima.0001.png /path/to/input/tangent.0001.exr /path/to/result/lic.0001.png 10";
        //also save the Line integral convolution in a jpg image
        cout << usagetxt << "\n" << usagetxt1 << endl;
        return 0;
        }
        
    char *inputfile = argv[1];
    char *flowfile = argv[2];
    char *outputfile  = argv[3];
    int s= atoi(argv[4]);
    
    Mat inputimage,flowfield,licresult;
    inputimage = imread(inputfile, cv::IMREAD_COLOR); //unchanged
    flowfield = imread(flowfile, -1); //unchanged
    cv::cvtColor(flowfield, flowfield, cv::COLOR_RGB2BGR);
    licresult = Mat::zeros(cv::Size(flowfield.cols, flowfield.rows), CV_8UC3);
    PP lic(cv::Size(flowfield.cols, flowfield.rows));
    lic.ETF_image(inputimage,flowfield, licresult,s);
    if (withpreview) {
        cv::namedWindow("etf");
        cv::imshow("etf", licresult);
        cv::waitKey(0);
    }
    //cv::cvtColor(licresult, licresult, CV_BGR2RGB);
    licresult.convertTo(licresult, CV_8UC3, 1.0 * 255.0);
    cv::imwrite(outputfile, licresult);
    printf("writing file : %s\n",outputfile);
}

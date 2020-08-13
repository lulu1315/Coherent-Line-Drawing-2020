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
        string usagetxt ="Usage : lic-cli flowfield outputframe iterations noiseupsample";
        string usagetxt1="        lic-cli /path/to/input/tangent.0001.exr /path/to/result/lic.0001.png 10 2";
        //also save the Line integral convolution in a jpg image
        cout << usagetxt << "\n" << usagetxt1 << endl;
        return 0;
        }
        
    char *inputfile = argv[1];
    char *outputfile  = argv[2];
    int s= atoi(argv[3]);
    int noiseupsample= atoi(argv[4]);
    
    Mat flowfield,licresult;
    flowfield = imread(inputfile, -1); //unchanged
    cv::cvtColor(flowfield, flowfield, CV_RGB2BGR);
    licresult = Mat::zeros(cv::Size(flowfield.cols, flowfield.rows), CV_8UC1);
    PP lic(cv::Size(flowfield.cols, flowfield.rows));
    lic.ETF(flowfield, licresult,s,noiseupsample);
    if (withpreview) {
        cv::namedWindow("etf");
        cv::imshow("etf", licresult);
        cv::waitKey(0);
    }
    //cv::cvtColor(licresult, licresult, CV_BGR2RGB);
    licresult.convertTo(licresult, CV_8UC1, 1.0 * 255.0);
    cv::imwrite(outputfile, licresult);
    printf("writing file : %s\n",outputfile);
}

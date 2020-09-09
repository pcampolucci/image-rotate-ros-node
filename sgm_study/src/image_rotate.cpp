#include "opencv2/opencv.hpp"

int main()
{
    // ============== ROTATE IMAGE ==============

    cv::Mat src = cv::imread("im.png", CV_LOAD_IMAGE_UNCHANGED);
    double angle = -20;

    // get rotation matrix for rotating the image around its center in pixel coordinates
    cv::Point2f center((src.cols-1)/2.0, (src.rows-1)/2.0);
    cv::Mat rot = cv::getRotationMatrix2D(center, angle, 1.0);
    // determine bounding rectangle, center not relevant
    cv::Rect2f bbox = cv::RotatedRect(cv::Point2f(), src.size(), angle).boundingRect2f();
    // adjust transformation matrix
    rot.at<double>(0,2) += bbox.width/2.0 - src.cols/2.0;
    rot.at<double>(1,2) += bbox.height/2.0 - src.rows/2.0;

    cv::Mat dst;
    cv::warpAffine(src, dst, rot, bbox.size());

    // ============== CHANGE BLACK TO WHITE IN CONTOUR ==============

    cv::Mat gray;
    cv::cvtColor(dst,gray,CV_BGR2GRAY);

    cv::Mat mask;
    // compute inverse thresholding
    double grayThres = cv::threshold(gray, mask, 0, 255, CV_THRESH_BINARY_INV);

    // color all masked pixel white
    dst.setTo(cv::Scalar(255,255,255), mask);

    // ============= SHOW IMAGE / SAVE IMAGE ==============

    cv::imshow("input", dst);
    cv::waitKey(0);
    // cv::imwrite("rotated_im_2.png", dst);

    return 0;
}
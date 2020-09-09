#include <ros/ros.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/image_encodings.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <dynamic_reconfigure/server.h>
#include <image_rotate/rotate_nodeConfig.h>
#include "opencv2/opencv.hpp"
#include <math.h>

#define PCL_NO_PRECOMPILE

using namespace std;
using namespace ros;
using namespace sensor_msgs;
namespace enc = sensor_msgs::image_encodings;

// dynamic parameter
double angle_;
int shift_a_;
int shift_b_;
int epi_1_;
int epi_2_;
int epi_3_;

void callback(image_rotate::rotate_nodeConfig &config, uint32_t level){
  angle_ = config.angle;
  shift_a_ = config.shift_a;
  shift_b_ = config.shift_b;
  epi_1_ = config.epi_1;
  epi_2_ = config.epi_2;
  epi_3_ = config.epi_3;
  ROS_INFO("New angle: %f", angle_);
  ROS_INFO("New shift a: %i", shift_a_);
  ROS_INFO("New shift b: %i", shift_b_);

}

class ImageRotate {
public:

    NodeHandle nodeHandle;

    // specify topic to listen and publish
    Subscriber image_subscriber_quad_a;
    Subscriber image_subscriber_quad_b;
    Publisher image_publisher_quad_a;
    Publisher image_publisher_quad_b;

    ImageRotate() {
        image_publisher_quad_a = nodeHandle.advertise<Image>("/camera/quad1/image_rotated", 1);
        image_publisher_quad_b = nodeHandle.advertise<Image>("/camera/quad3/image_rotated", 1);
    }

    void start() {
        image_subscriber_quad_a = nodeHandle.subscribe("/camera/quad1/image_rect_color", 1, &ImageRotate::Rotate, this);
        image_subscriber_quad_b = nodeHandle.subscribe("/camera/quad3/image_rect_color", 1, &ImageRotate::Rotate, this);
    }

    cv::Mat translateImg(cv::Mat &img, int shift){

        // constructs a larger image to fit both the image and the border
        cv::Mat dst(img.rows + shift, img.cols, img.depth());
        // form a border in-place (top, bottom, l, r)
        cv::copyMakeBorder(img, dst, 1, shift, 1, 1, CV_HAL_BORDER_CONSTANT, cv::Scalar(255,255,255));
        // apply translation
        cv::Mat trans_mat = (cv::Mat_<double>(2,3) << 1, 0, 0, 0, 1, shift);
        warpAffine(dst, dst, trans_mat, dst.size());
        return dst;
        
    }

    void Rotate(const Image::ConstPtr& image) {

        // ============== CONVERT IMAGE TO MAT ==============
        string id = image->header.frame_id;
        ROS_INFO("received message %s", id.c_str());
        cv::Mat src;
        
        try {
            cv_bridge::CvImagePtr cv_ptr; 
            cv_ptr = cv_bridge::toCvCopy(image, sensor_msgs::image_encodings::RGB8); 
            src = cv_ptr -> image;
            }
        catch (cv_bridge::Exception& e) {
            ROS_ERROR("cv_bridge exception: %s", e.what());
            return;
        }

        // ============== ROTATE IMAGE ==============
        // get rotation matrix for rotating the image around its center in pixel coordinates
        cv::Point2f center((src.cols-1)/2.0, (src.rows-1)/2.0);
        cv::Mat rot = cv::getRotationMatrix2D(center, angle_, 1.0);
        // determine bounding rectangle, center not relevant
        cv::Rect2f bbox = cv::RotatedRect(cv::Point2f(), src.size(), angle_).boundingRect2f();
        // adjust transformation matrix
        rot.at<double>(0,2) += bbox.width/2.0 - src.cols/2.0;
        rot.at<double>(1,2) += bbox.height/2.0 - src.rows/2.0;

        cv::Mat dst;
        cv::warpAffine(src, dst, rot, bbox.size());

        // ============== SHIFT IMAGE VERTICALLY ==============
        // the shift happens for the image from the lower camera, so only quad3
        if (id == "quad1") {
            dst = translateImg(dst, shift_a_);
        } else {
            dst = translateImg(dst, shift_b_);
        }
        

        // ============== CHANGE BLACK TO WHITE IN CONTOUR ==============
        cv::Mat gray;
        cv::cvtColor(dst, gray, CV_BGR2GRAY);

        cv::Mat mask;
        // compute inverse thresholding
        double grayThres = cv::threshold(gray, mask, 0, 255, CV_THRESH_BINARY_INV);

        // color all masked pixel white
        dst.setTo(cv::Scalar(255,255,255), mask);

        // ============== ADD HYPERPOLAR LINES ==============
        cv::Point p1(0, epi_1_), q1(dst.cols, epi_1_);
        cv::Point p2(0, epi_2_), q2(dst.cols, epi_2_);
        cv::Point p3(0, epi_3_), q3(dst.cols, epi_3_);
        cv::line(dst, p1, q1, cv::Scalar(0,255,0), 5);
        cv::line(dst, p2, q2, cv::Scalar(255,0,0), 5);
        cv::line(dst, p3, q3, cv::Scalar(0,0,255), 5);

        // ============== CHANGE BACK TO IMAGE FOR ROS ==============
        cv_bridge::CvImage out_msg;
        out_msg.header   = image->header; // Same timestamp and tf frame as input image
        out_msg.encoding = sensor_msgs::image_encodings::RGB8;
        out_msg.image    = dst; // Your cv::Mat
        

        // ============== PUBLISH TOPIC ==============
        if (id == "quad1") {
            image_publisher_quad_a.publish(out_msg.toImageMsg());
        } else {
            image_publisher_quad_b.publish(out_msg.toImageMsg());
        }
    }

};

int main(int argc, char **argv) {

  // Init the node
  ROS_INFO("Rotating Images");
  init(argc, argv, "rotate");

  // Starting Dynamic reconfigure server
  dynamic_reconfigure::Server<image_rotate::rotate_nodeConfig> server;
  dynamic_reconfigure::Server<image_rotate::rotate_nodeConfig>::CallbackType f;

  f = boost::bind(&callback, _1, _2);
  server.setCallback(f);

  // Create a TrackFinder object
  ImageRotate rotate_image;

  // Start the feedback loop
  rotate_image.start();

  // Keep listening till Ctrl+C is pressed
  spin();

  // Exit succesfully
  return 0;
}

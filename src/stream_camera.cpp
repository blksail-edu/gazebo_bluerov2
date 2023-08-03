// build with $ g++ image_cv_gst.cpp -o cv_gst_udp `pkg-config --cflags --libs gz-transport12 gz-msgs9 opencv4`
//
//  TEST WITH GST IN TERMINAL:
//  $ gst-launch-1.0 -ve udpsrc port=5000 ! tsparse ! tsdemux ! h264parse ! avdec_h264 ! videoconvert ! autovideosink sync=false

#include <stdio.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <gz/msgs.hh>
#include <gz/transport.hh>
#include <mutex>

std::mutex mtx; // Mutex for safe access to the frame
cv::Mat frame;  // Global frame to use in the callback and main function

void cb(const gz::msgs::Image &_msg)
{
  // create image using opencv in RGB format
  cv::Mat img_rgb = cv::Mat(_msg.height(), _msg.width(), CV_8UC3, (void *)_msg.data().c_str());
  // convert from RGB to BGR
  cv::cvtColor(img_rgb, frame, cv::COLOR_RGB2BGR);
  // print img information
  std::cout << "img size: " << frame.size() << std::endl;
  // print image to screen
  // cv::imshow("image", frame);
  // cv::waitKey(1);
}

int main(int argc, char **argv)
{

  // Create a Node for communication.
  gz::transport::Node node;

  // Subscribe to the 'camera' topic
  node.Subscribe("camera_9012", cb);

  // second part of sender pipeline
  cv::VideoWriter writer;
  writer.open("appsrc ! videoconvert ! x264enc noise-reduction=10000 tune=zerolatency byte-stream=true threads=4 ! mpegtsmux ! udpsink host=0.0.0.0 port=5000", 0, (double)30, cv::Size(320, 240), true);
  if (!writer.isOpened())
  {
    printf("=ERR= can't create video writer\n");
    return -1;
  }

  int key;

  while (true)
  {
    mtx.lock();
    if (!frame.empty())
    {
      std::cout << "writing frame" << frame.size() << std::endl;
      writer << frame;
      // set frame empty
      frame = cv::Mat();
    }
    mtx.unlock();

    key = cv::waitKey(30);
    if (key == 27) // 'Esc' key to stop
      break;
  }

  gz::transport::waitForShutdown();

  return 0;
}
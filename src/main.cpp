#include "camera.hpp"
#include "multi_camera.hpp"
#include <com_client.hpp>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <lsl_c.h>

using namespace cv;
const int max_value_H = 360 / 2;
const int max_value = 255;
int low_H = 90, low_S = 171, low_V = 91;
int high_H = 95, high_S = 222, high_V = 167;
int dilate_size = 20;
std::string win_name = "Win";

static void
on_low_H_thresh_trackbar(int, void *)
{
    low_H = min(high_H - 1, low_H);
    setTrackbarPos("Low H", win_name, low_H);
}
static void
on_high_H_thresh_trackbar(int, void *)
{
    high_H = max(high_H, low_H + 1);
    setTrackbarPos("High H", win_name, high_H);
}
static void
on_low_S_thresh_trackbar(int, void *)
{
    low_S = min(high_S - 1, low_S);
    setTrackbarPos("Low S", win_name, low_S);
}
static void
on_high_S_thresh_trackbar(int, void *)
{
    high_S = max(high_S, low_S + 1);
    setTrackbarPos("High S", win_name, high_S);
}
static void
on_low_V_thresh_trackbar(int, void *)
{
    low_V = min(high_V - 1, low_V);
    setTrackbarPos("Low V", win_name, low_V);
}
static void
on_high_V_thresh_trackbar(int, void *)
{
    high_V = max(high_V, low_V + 1);
    setTrackbarPos("High V", win_name, high_V);
}

cv::Mat hsv, thresh, inverted, dilated;
cv::Ptr<cv::SimpleBlobDetector> detector;
void
get_keypoints(Mat &frame, std::vector<cv::KeyPoint> &keypoints)
{
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, cv::Scalar(low_H, low_S, low_V),
                cv::Scalar(high_H, high_S, high_V), thresh);
    cv::dilate(thresh, dilated,
               cv::getStructuringElement(cv::MORPH_ELLIPSE,
                                         cv::Size(dilate_size, dilate_size)));
    bitwise_not(dilated, inverted);
    detector->detect(inverted, keypoints);
}

int
main(int argc, char **argv)
{
    namedWindow(win_name);
    // Trackbars to set thresholds for HSV values
    createTrackbar("Low H", win_name, &low_H, max_value_H,
                   on_low_H_thresh_trackbar);
    createTrackbar("High H", win_name, &high_H, max_value_H,
                   on_high_H_thresh_trackbar);
    createTrackbar("Low S", win_name, &low_S, max_value,
                   on_low_S_thresh_trackbar);
    createTrackbar("High S", win_name, &high_S, max_value,
                   on_high_S_thresh_trackbar);
    createTrackbar("Low V", win_name, &low_V, max_value,
                   on_low_V_thresh_trackbar);
    createTrackbar("High V", win_name, &high_V, max_value,
                   on_high_V_thresh_trackbar);
    createTrackbar("dilate", win_name, &dilate_size, 200,
                   on_high_V_thresh_trackbar);

    cv::SimpleBlobDetector::Params params;
    params.minThreshold = 1;
    params.maxThreshold = 2000;
    params.filterByArea = true;
    params.minArea = 150;
    params.maxArea = 10000;
    params.filterByCircularity = false;
    params.minCircularity = 0.1;
    params.filterByConvexity = false;
    params.minConvexity = 0.87;
    params.filterByInertia = false;
    params.minInertiaRatio = 0.01;
    detector = cv::SimpleBlobDetector::create(params);

    MultiCam cameras;
    int n = argc - 3;
    std::cout << "Number of cameras: " << n << std::endl;
    int width = 3264;
    int height = 2448;
    double coef = 640. / width;

    for(int i = 0; i < n; i++)
        cameras.addCamera(atoi(argv[i + 3]), width, height);

    /*** FISRT IMAGE RECORDING PHASE ****/
    std::string img_name = argv[1];
    int n_images = atoi(argv[2]);
    std::cout << "Press any key to record images" << std::endl;

    cv::Mat hdframe[2];
    cv::Mat hdframe_resized[2];
    cv::Mat frame[2];
    for(int i = 0; i < n_images; i++)
    {
        while(1)
        {
            for(int j = 0; j < 2; j++)
            {
                hdframe[j] = cameras.getFrame(j);
                cv::resize(hdframe[j], hdframe_resized[j], cv::Size(0, 0), coef,
                           coef);
                cv::imshow("cam" + std::to_string(j), hdframe_resized[j]);
            }
            if(cv::waitKey(10) >= 0)
                break;
        }
        for(int j = 0; j < 2; j++)
            cv::imwrite(img_name + "_" + std::to_string(i) + "_c" +
                            std::to_string(j) + ".png",
                        hdframe[j]);
        std::cout << "Image recorded! " << i + 1 << "/" << n_images
                  << std::endl;
    }
    /*** -------------------------- ****/
    for(int j = 0; j < 2; j++)
      {
      cv::resize(hdframe[j], hdframe_resized[j], cv::Size(0, 0), coef, coef);
      cameras.m_cameras[j]->set_resolution(640, 480);
      }

    
    int nb_ch = 4;
    float data[nb_ch];
    lsl_streaminfo info =
      lsl_create_streaminfo("positions", "positions", nb_ch,
                                  0, cft_float32, "camerasuid");
    lsl_outlet outlet = lsl_create_outlet(info, 0, 360);
    for(;;)
    {
      for(int j = 0; j < 2; j++)
      {
        frame[j] = cameras.getFrame(j);
        std::vector<cv::KeyPoint> keypoints;
        keypoints.push_back(cv::KeyPoint(-1, -1, 20));
        get_keypoints(frame[j], keypoints);

        cv::Mat im_with_keypoints;
        drawKeypoints(frame[j], keypoints, im_with_keypoints,
                      cv::Scalar(0, 0, 255),
                      cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
        cv::imshow("cam" + std::to_string(j), im_with_keypoints);

        //send to client
        data[j*2] = keypoints[0].pt.x/coef;
        data[j*2+1] = keypoints[0].pt.y/coef;
        std::cout << data[j*2] << "\t" << data[j*2+1] << "\t";
      }
      cv::imshow(win_name, inverted);
      std::cout << "       \xd" <<std::flush;
      lsl_push_sample_f(outlet, data);

        if(cv::waitKey(5) >= 0)
            break;
    }

    return 0;
}

#include "camera.hpp"
#include "multi_camera.hpp"
#include <iostream>
#include <lsl_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
//setprecision
#include <iomanip>

using namespace cv;
const int max_value_H = 360 / 2;
const int max_value = 255;
int low_H = 107, low_S = 114, low_V = 106;
int high_H = 153, high_S = 255, high_V = 220;
int dilate_size = 9;
std::string win_name = "Win";
int border = 60;

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
    //remove border
    cv::rectangle(thresh, cv::Point(0, 0),
                  cv::Point(thresh.cols - 1, thresh.rows - 1),
                  cv::Scalar(0, 0, 0), border);
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

    if(argc < 2)
    {
        std::cout << "Usage: ./main <cam1_index> <cam2_index> ..." << std::endl;
        return 0;
    }

    MultiCam cameras(2);
    int n = argc - 1;
    std::cout << "Number of cameras: " << n << std::endl;
    int width = 3264;
    int height = 2448;
    double coef = 640. / width;
    cv::Mat frame[n];

    for(int i = 0; i < n; i++)
        cameras.addCamera(atoi(argv[i + 1]), width, height);

    for(int j = 0; j < n; j++)
    {
        cameras.get_cam(j)->set_resolution(640, 480);
        cameras.get_cam(j)->set_wrap("wrap" + std::to_string(j) + ".xml");
    }

    int nb_ch = 3 * n;
    float data[nb_ch];
    lsl_streaminfo info = lsl_create_streaminfo("positions", "positions", nb_ch,
                                                0, cft_float32, "camerasuid");
    lsl_outlet outlet = lsl_create_outlet(info, 0, 360);
    for(;;)
    {
        std::cout << "\xd                                                      "
                     "                       \xd";
        for(int j = 0; j < n; j++)
        {
            frame[j] = cameras.getFrame(j, true);
            std::vector<cv::KeyPoint> keypoints;
            keypoints.push_back(cv::KeyPoint(-1, -1, 20));
            get_keypoints(frame[j], keypoints);

            cv::Mat im_with_keypoints;
            drawKeypoints(frame[j], keypoints, im_with_keypoints,
                          cv::Scalar(0, 0, 255),
                          cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
            cv::imshow("cam" + std::to_string(j), im_with_keypoints);

            //check if 2 keypoints are detected with max distance of 100px
            if(keypoints.size() > 1)
            {
                double dx = keypoints[0].pt.x - keypoints[1].pt.x;
                double dy = keypoints[0].pt.y - keypoints[1].pt.y;
                double dist = sqrt(dx * dx + dy * dy);
                double angle = atan2(dy, dx) * 180 / M_PI;
                if(dist < 60)
                {
                    //send to client
                    data[j * 2] = (keypoints[0].pt.x - border) /
                                  (frame[j].cols - border * 2);
                    data[j * 2 + 1] = (keypoints[0].pt.y - border) /
                                      (frame[j].rows - border * 2);
                    //print with 3 decimals
                    std::cout << std::fixed << std::setprecision(3);
                    std::cout << data[j * 2] << "\t\t" << data[j * 2 + 1]
                              << "\t\t" << angle << "\t\t" << dist << std::endl;
                }
            }
        }
        cv::imshow(win_name, inverted);
        std::cout << std::flush;
        lsl_push_sample_f(outlet, data);

        //press q to quit
        if(cv::waitKey(1) == 'q')
            break;
    }

    return 0;
}

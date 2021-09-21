#include <iostream>
#include <cstdlib> // 亂數相關函數
#include <ctime>   // 時間相關函數
#include <time.h>
#include <windows.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/photo.hpp>

using namespace cv;
using namespace std;

vector<Point> pointList;
int           PtoP = 104;
int           z1 = 20;
void          video();
Mat           creatImg(int x, int y, int z);

int main(int argc, char **argv) {
  video();
  return 0;
}

Mat creatImg(int x, int y, int z) {
  srand((unsigned int)time(NULL));

  Mat img = Mat::zeros(x, y, z);
  int i, j, k = 0;
  int random = rand() % 5;

  // 0 是綠色, 1 是紅色
  int test[5][16] = {{0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0},
                     {0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
                     {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
                     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
                     {1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1}};

  for (j = 90; j <= y - 160; j += PtoP) {
    for (i = 110; i < x; i += PtoP) {
      // circle (影像, 圓心座標, 半徑, 顏色, 線條寬度)
      if (test[random][k] == 1) {
        circle(img, Point(i, j), 50, Scalar(0, 0, 255), 2);
      } else {
        circle(img, Point(i, j), 50, Scalar(0, 255, 0), 2);
      }

      circle(img, Point(i, j), z1, Scalar(125, 125, 125), 2);
      pointList.push_back(Point(i, j));
      k++;
    }
  }

  return img;
}

void video() {
  VideoCapture cap(0);
  Mat          img, imgHSV;

  vector<std::vector<cv::Point>> OriginalContours;
  vector<Vec4i>                  hierarchy;
  vector<vector<Point>>          FinalContours;
  vector<Point>                  hull;

  // 藍色
  int iLowH = 100;
  int iHighH = 140;
  int iLowS = 40;
  int iHighS = 255;
  int iLowV = 40;
  int iHighV = 255;

  /*// 綠色
  int iLowH = 78;
  int iHighH = 99;
  int iLowS = 43;
  int iHighS = 255;
  int iLowV = 46;
  int iHighV = 255;*/

  cap >> img;

  Mat test = creatImg(img.rows, img.cols, img.type());
  Mat out1 = Mat::zeros(img.rows, img.cols, img.type());
  Mat out2 = Mat::zeros(img.rows, img.cols, img.type());
  Mat backGround = Mat::zeros(img.rows, img.cols, img.type());
  Mat out;
  Mat d;

  int flag, flag2;
  int successful = 0;
  int X_position = 0, Y_position = 0;

  while (1) {
    cap >> img;
    if (img.empty())
      break;

    // 左右反轉
    flip(img, img, 1);

    // 轉為 HSV
    cvtColor(img, imgHSV, COLOR_BGR2HSV);

    Mat imgThresholded;
    inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV),
            imgThresholded);

    // 獲取自定義核 (第一個參數:內核的形狀，第二個參數: 內核的尺寸)
    Mat element = getStructuringElement(MORPH_RECT, Size(5, 5));

    // 開操作
    morphologyEx(imgThresholded, imgThresholded, MORPH_OPEN, element);

    // 閉操作 (連接一些連通域)
    morphologyEx(imgThresholded, imgThresholded, MORPH_CLOSE, element);

    vector<vector<Point>> contours;
    // 找輪廓
    findContours(imgThresholded, contours, cv::RETR_EXTERNAL,
                 CHAIN_APPROX_NONE);
    vector<vector<Point>>::iterator cit = contours.begin();
    while (cit != contours.end()) {
      // 過濾面積太小的區塊
      if (contourArea(*cit) < 300) {
        cit = contours.erase(cit);
      } else
        cit++;
    }

    // 區塊
    Mat dst = img.clone();

    vector<vector<Point>> hull(contours.size());
    cvtColor(imgThresholded, imgThresholded, cv::COLOR_GRAY2RGB);

    // 畫出區塊
    for (int i = 0; i < contours.size(); i++) {
      convexHull(contours[i], hull[i]);
    }

    // 中心點
    vector<Moments> mu(contours.size());
    for (int i = 0; i < contours.size(); i++) {
      mu[i] = moments(contours[i], false);
    }

    Point mc(contours.size());
    for (int i = 0; i < contours.size(); i++) {
      mc = Point2f(mu[i].m10 / mu[i].m00, mu[i].m01 / mu[i].m00);
    }
    circle(dst, mc, 10, Scalar(255, 155, 62), -1);

    // 判斷
    for (int n = 0; n < pointList.size(); n++) {
      Point myPoint = pointList[n];
      successful = 1;
      if (test.at<Vec3b>(myPoint.y + 51, myPoint.x)[1] != 255) {
        successful = 0;
        break;
      }
    }

    if (successful == 1)
      break;

    // 顏色轉換
    for (int n = 0; n < pointList.size(); n++) {
      Point myPoint = pointList[n];
      flag = 0;
      flag2 = 0;

      if ((mc.x & mc.y) == 0)
        break;

      // 判斷是否在同一個圈
      if ((X_position & Y_position) != 0) {
        if ((mc.x >= X_position - z1) & (mc.x <= X_position + z1)) {
          if ((mc.y >= Y_position - z1) & (mc.y <= Y_position + z1)) {
            break;
          } else {
            X_position = 0;
            Y_position = 0;
          }
        } else {
          X_position = 0;
          Y_position = 0;
        }
      }

      if ((mc.x >= myPoint.x - z1) & (mc.x <= myPoint.x + z1)) {
        if ((mc.y >= myPoint.y - z1) & (mc.y <= myPoint.y + z1)) {
          X_position = myPoint.x;
          Y_position = myPoint.y;

          int x = myPoint.x;
          int y = myPoint.y;

          // X 軸轉換
          if (x == 110) {
            if (test.at<Vec3b>(y, x + 51)[2] == 255) {
              circle(out1, myPoint, 50, Scalar(0, 0, 255), 2);

              if (test.at<Vec3b>(y, x + PtoP - 1)[2] == 255) {
                circle(out1, Point(x + PtoP, y), 50, Scalar(0, 0, 255), 2);
                flag = 1;
              } else {
                circle(out1, Point(x + PtoP, y), 50, Scalar(0, 255, 0), 2);
              }

              cv::absdiff(test, out1, out2);
              test = out2.clone();
              cv::absdiff(out1, out1, out1);

              circle(out1, myPoint, 50, Scalar(0, 255, 0), 2);

              if (flag == 1) {
                circle(out1, Point(x + PtoP, y), 50, Scalar(0, 255, 0), 2);
              } else {
                circle(out1, Point(x + PtoP, y), 50, Scalar(0, 0, 255), 2);
              }
            } else {
              circle(out1, myPoint, 50, Scalar(0, 255, 0), 2);

              if (test.at<Vec3b>(y, x + PtoP - 1)[2] == 255) {
                circle(out1, Point(x + PtoP, y), 50, Scalar(0, 0, 255), 2);
                flag = 1;
              } else {
                circle(out1, Point(x + PtoP, y), 50, Scalar(0, 255, 0), 2);
              }

              cv::absdiff(test, out1, out2);
              test = out2.clone();
              cv::absdiff(out1, out1, out1);

              circle(out1, myPoint, 50, Scalar(0, 0, 255), 2);

              if (flag == 1) {
                circle(out1, Point(x + PtoP, y), 50, Scalar(0, 255, 0), 2);
              } else {
                circle(out1, Point(x + PtoP, y), 50, Scalar(0, 0, 255), 2);
              }
            }
          } else {
            if (x + 100 < img.rows) {
              if (test.at<Vec3b>(y, x + 51)[2] == 255) {
                circle(out1, myPoint, 50, Scalar(0, 0, 255), 2);

                if (test.at<Vec3b>(y, x + PtoP - 1)[2] == 255) {
                  circle(out1, Point(x + PtoP, y), 50, Scalar(0, 0, 255), 2);
                  flag = 1;
                } else {
                  circle(out1, Point(x + PtoP, y), 50, Scalar(0, 255, 0), 2);
                }

                if (test.at<Vec3b>(y, x - PtoP - 1)[2] == 255) {
                  circle(out1, Point(x - PtoP, y), 50, Scalar(0, 0, 255), 2);
                  flag2 = 1;
                } else {
                  circle(out1, Point(x - PtoP, y), 50, Scalar(0, 255, 0), 2);
                }

                cv::absdiff(test, out1, out2);
                test = out2.clone();
                cv::absdiff(out1, out1, out1);

                circle(out1, myPoint, 50, Scalar(0, 255, 0), 2);

                if (flag == 1) {
                  circle(out1, Point(x + PtoP, y), 50, Scalar(0, 255, 0), 2);
                } else {
                  circle(out1, Point(x + PtoP, y), 50, Scalar(0, 0, 255), 2);
                }

                if (flag2 == 1) {
                  circle(out1, Point(x - PtoP, y), 50, Scalar(0, 255, 0), 2);
                } else {
                  circle(out1, Point(x - PtoP, y), 50, Scalar(0, 0, 255), 2);
                }
              } else {
                circle(out1, myPoint, 50, Scalar(0, 255, 0), 2);

                if (test.at<Vec3b>(y, x + PtoP - 1)[2] == 255) {
                  circle(out1, Point(x + PtoP, y), 50, Scalar(0, 0, 255), 2);
                  flag = 1;
                } else {
                  circle(out1, Point(x + PtoP, y), 50, Scalar(0, 255, 0), 2);
                }

                if (test.at<Vec3b>(y, x - PtoP - 1)[2] == 255) {
                  circle(out1, Point(x - PtoP, y), 50, Scalar(0, 0, 255), 2);
                  flag2 = 1;
                } else {
                  circle(out1, Point(x - PtoP, y), 50, Scalar(0, 255, 0), 2);
                }

                cv::absdiff(test, out1, out2);
                test = out2.clone();
                cv::absdiff(out1, out1, out1);

                circle(out1, myPoint, 50, Scalar(0, 0, 255), 2);

                if (flag == 1) {
                  circle(out1, Point(x + PtoP, y), 50, Scalar(0, 255, 0), 2);
                } else {
                  circle(out1, Point(x + PtoP, y), 50, Scalar(0, 0, 255), 2);
                }
                if (flag2 == 1) {
                  circle(out1, Point(x - PtoP, y), 50, Scalar(0, 255, 0), 2);
                } else {
                  circle(out1, Point(x - PtoP, y), 50, Scalar(0, 0, 255), 2);
                }
              }
            } else {
              if (test.at<Vec3b>(y, x + 51)[2] == 255) {
                circle(out1, myPoint, 50, Scalar(0, 0, 255), 2);

                if (test.at<Vec3b>(y, x - PtoP - 1)[2] == 255) {
                  circle(out1, Point(x - PtoP, y), 50, Scalar(0, 0, 255), 2);
                  flag = 1;
                } else {
                  circle(out1, Point(x - PtoP, y), 50, Scalar(0, 255, 0), 2);
                }

                cv::absdiff(test, out1, out2);
                test = out2.clone();
                cv::absdiff(out1, out1, out1);

                circle(out1, myPoint, 50, Scalar(0, 255, 0), 2);
                if (flag == 1) {
                  circle(out1, Point(x - PtoP, y), 50, Scalar(0, 255, 0), 2);
                } else {
                  circle(out1, Point(x - PtoP, y), 50, Scalar(0, 0, 255), 2);
                }

              } else {
                circle(out1, myPoint, 50, Scalar(0, 255, 0), 2);

                if (test.at<Vec3b>(y, x - PtoP - 1)[2] == 255) {
                  circle(out1, Point(x - PtoP, y), 50, Scalar(0, 0, 255), 2);
                  flag = 1;
                } else {
                  circle(out1, Point(x - PtoP, y), 50, Scalar(0, 255, 0), 2);
                }

                cv::absdiff(test, out1, out2);
                test = out2.clone();
                cv::absdiff(out1, out1, out1);

                circle(out1, myPoint, 50, Scalar(0, 0, 255), 2);

                if (flag == 1) {
                  circle(out1, Point(x - PtoP, y), 50, Scalar(0, 255, 0), 2);
                } else {
                  circle(out1, Point(x - PtoP, y), 50, Scalar(0, 0, 255), 2);
                }
              }
            }
          }

          // y 軸轉換
          if (y == 90) {
            if (test.at<Vec3b>(y + PtoP - 1, x)[2] == 255) {
              circle(out1, Point(x, y + PtoP), 50, Scalar(0, 0, 255), 2);
              flag = 1;
            } else {
              circle(out1, Point(x, y + PtoP), 50, Scalar(0, 255, 0), 2);
            }

            cv::absdiff(test, out1, out2);
            test = out2.clone();
            cv::absdiff(out1, out1, out1);

            if (flag == 1) {
              circle(out1, Point(x, y + PtoP), 50, Scalar(0, 255, 0), 2);
            } else {
              circle(out1, Point(x, y + PtoP), 50, Scalar(0, 0, 255), 2);
            }
          } else {
            if (y + 100 < img.rows) {
              if (test.at<Vec3b>(y + PtoP - 1, x)[2] == 255) {
                circle(out1, Point(x, y + PtoP), 50, Scalar(0, 0, 255), 2);
                flag = 1;
              } else {
                circle(out1, Point(x, y + PtoP), 50, Scalar(0, 255, 0), 2);
              }
              if (test.at<Vec3b>(y - PtoP - 1, x)[2] == 255) {
                circle(out1, Point(x, y - PtoP), 50, Scalar(0, 0, 255), 2);
                flag2 = 1;
              } else {
                circle(out1, Point(x, y - PtoP), 50, Scalar(0, 255, 0), 2);
              }

              cv::absdiff(test, out1, out2);
              test = out2.clone();
              cv::absdiff(out1, out1, out1);

              if (flag == 1) {
                circle(out1, Point(x, y + PtoP), 50, Scalar(0, 255, 0), 2);
              } else {
                circle(out1, Point(x, y + PtoP), 50, Scalar(0, 0, 255), 2);
              }
              if (flag2 == 1) {
                circle(out1, Point(x, y - PtoP), 50, Scalar(0, 255, 0), 2);
              } else {
                circle(out1, Point(x, y - PtoP), 50, Scalar(0, 0, 255), 2);
              }
            } else {
              if (test.at<Vec3b>(y - PtoP - 1, x)[2] == 255) {
                circle(out1, Point(x, y - PtoP), 50, Scalar(0, 0, 255), 2);
                flag = 1;
              } else {
                circle(out1, Point(x, y - PtoP), 50, Scalar(0, 255, 0), 2);
              }

              cv::absdiff(test, out1, out2);
              test = out2.clone();
              cv::absdiff(out1, out1, out1);

              if (flag == 1) {
                circle(out1, Point(x, y - PtoP), 50, Scalar(0, 255, 0), 2);
              } else {
                circle(out1, Point(x, y - PtoP), 50, Scalar(0, 0, 255), 2);
              }
            }
          }
          break;
        }
      }
    }

    cv::absdiff(test, out1, out2);
    test = out2.clone();
    cv::absdiff(out1, out1, out1);

    // 遊戲畫面與視訊畫面結合
    addWeighted(test, 10.0, dst, 0.5, 1, out);

    imshow("finish", out);

    // 按 esc 結束程式
    char c = (char)waitKey(25);
    if (c == 27)
      break;
  }

  destroyAllWindows();
  Mat fin = imread("D:\\ux-opencv\\Congratulation.jpg");
  imshow("Congratulation", fin);
  waitKey(0);
  cv::destroyAllWindows();
  return;
}
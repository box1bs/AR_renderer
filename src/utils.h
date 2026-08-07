#ifndef RECREN_UTILS_H
#define RECREN_UTILS_H

#include <glad/glad.h>
#include <opencv2/opencv.hpp>
#include <opencv2/calib3d.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>

GLuint compileShader(GLenum type, const char* src);
unsigned int CreateVAO(const float* verts, const unsigned int* iverts, unsigned int size, unsigned int isize);
std::string FRead(const char* path);
GLFWwindow* InitWindow(int width, int height);
void BuildCameraMatrix(double height, double width, cv::Mat& cameraMatrix, cv::Mat& distCoeffs);
glm::mat4 CvPoseToGlmView(const cv::Mat& rvec, const cv::Mat& tvec);
glm::mat4 CvMatToGlmProjection(const cv::Mat& cameraMatrix, float width, float height, float near, float far);
std::vector<cv::Point2f> RotateQuadBaseLike(const std::vector<cv::Point2f>& points, double cx, double cy);
std::vector<cv::Point2f> MatchToPrevious(const std::vector<cv::Point2f>& current, std::vector<cv::Point2f>& prev);

#endif //RECREN_UTILS_H
#include <fstream>

#include "utils.h"

GLFWwindow* InitWindow(const int width, const int height) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    return glfwCreateWindow(width, height, "window", nullptr, nullptr);
}

std::string FRead(const char *path) {
    const std::ifstream file(path);
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::vector<cv::Point2f> RotateQuadBaseLike(const std::vector<cv::Point2f>& points, const double cx, const double cy) {
    std::vector<cv::Point2f> vec(4);
    for (const auto& pt : points) {
        if (pt.x < cx && pt.y < cy) {
            vec[3] = pt; // левый верх
        } else if (pt.x > cx && pt.y < cy) {
            vec[2] = pt; // правый верх
        } else if (pt.x > cx && pt.y > cy) {
            vec[1] = pt; // правый низ
        } else { // pt.x < cx && pt.y > cy
            vec[0] = pt; // левый низ
        }
    }
    return vec;
}

std::vector<cv::Point2f> MatchToPrevious(const std::vector<cv::Point2f>& current, std::vector<cv::Point2f>& prev) {
    if (prev.empty()) {
        prev = current;
        return prev;
    }

    std::vector<cv::Point2f> result(4);
    std::vector used(4, false);

    for (int i = 0; i < 4; ++i) {
        double bestDist = std::numeric_limits<double>::max();
        int bestIdx = -1;
        for (int j = 0; j < 4; ++j) {
            if (used[j]) continue;
            if (const double dist = cv::norm(prev[i] - current[j]); dist < bestDist) {
                bestDist = dist;
                bestIdx = j;
            }
        }
        result[i] = current[bestIdx];
        used[bestIdx] = true;
    }

    prev = result;
    return result;
}

glm::mat4 CvMatToGlmProjection(const cv::Mat& cameraMatrix, const float width, const float height, const float near, const float far) {
    const auto fx = static_cast<float>(cameraMatrix.at<double>(0, 0)); // fov x
    const auto fy = static_cast<float>(cameraMatrix.at<double>(1, 1)); // fov y
    const auto cx = static_cast<float>(cameraMatrix.at<double>(0,2)); // center x
    const auto cy = static_cast<float>(cameraMatrix.at<double>(1,2)); // center y

    glm::mat4 proj(0.f);
    proj[0][0] = 2.f * fx / width;
    proj[1][1] = 2.f * fy / height;
    proj[2][0] = 1.f - 2.f * cx / width;
    proj[2][1] = 2.f * cy / height - 1.f;
    proj[2][2] = -(far + near) / (far - near);
    proj[2][3] = -1.f;
    proj[3][2] = -2.f * far * near / (far - near);

    return proj;
}

void BuildCameraMatrix(const double height, const double width, cv::Mat& cameraMatrix, cv::Mat& distCoeffs) {
    const double fx = width; // width / (2 * tg(X_FOV / 2)) но кому не похуй

    cameraMatrix = (cv::Mat_<double>(3, 3) <<
        fx, 0, width / 2.0,
        0, fx, height / 2.0,
        0, 0, 1
    );

    distCoeffs = cv::Mat::zeros(4, 1, CV_64F);
}

glm::mat4 CvPoseToGlmView(const cv::Mat& rvec, const cv::Mat& tvec) {
    cv::Mat R;
    cv::Rodrigues(rvec, R);

    glm::mat4 view;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            view[j][i] = static_cast<float>(R.at<double>(i, j));
        }
        view[3][i] = static_cast<float>(tvec.at<double>(i, 0));
    }

    const glm::mat4 flipYZ = glm::scale(glm::mat4(1.f), glm::vec3(1, -1, -1));
    return flipYZ * view;
}

GLuint compileShader(const GLenum type, const char* src) {
    const GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}
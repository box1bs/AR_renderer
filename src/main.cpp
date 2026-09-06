#include <ranges>
#include <fstream>

#include "renderer.h"

void initTrackWindow(const std::string& trackWindowName);

int main() {
    std::string vShaderText;
    std::string fShaderText;

    try {
        vShaderText = FRead("../shaders/background.vert");
        fShaderText = FRead("../shaders/background.frag");
    } catch (std::ifstream::failure& e) {
        std::cout << "oh, fuck: " << e.what() << std::endl;
        return 1;
    }

    cv::VideoCapture cap("../testvids/3.mp4");
    if (!cap.isOpened()) {
        std::cout << "frame isn't captured" << std::endl;
        return 1;
    }

    const int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    const int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    double windowDiagProduct = cv::norm(cv::Point(0, 0) - cv::Point(width, height)) * cv::norm(cv::Point(0, height) - cv::Point(width, 0));
    GLFWwindow* window = InitWindow(width, height);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));

    // вершины фона
    constexpr float vertices[] = {
        -1, -1,  0, 1,
         1, -1,  1, 1,
         1,  1,  1, 0,
        -1,  1,  0, 0,
    };
    // индексы вершин для построения треугольников
    constexpr unsigned int indices[] = { 0,1,2, 2,3,0 };

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), static_cast<void *>(nullptr));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), reinterpret_cast<void *>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    unsigned int prog = glCreateProgram();
    unsigned int vert = compileShader(GL_VERTEX_SHADER, vShaderText.c_str());
    unsigned int frag = compileShader(GL_FRAGMENT_SHADER, fShaderText.c_str());
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert);
    glDeleteShader(frag);

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    const std::string trackWindowName = "trackWindow";
    initTrackWindow(trackWindowName);

    std::string modelVShaderText;
    std::string modelFShaderText;
    try {
        modelVShaderText = FRead("../shaders/model.vert");
        modelFShaderText = FRead("../shaders/model.frag");
    } catch (std::ifstream::failure& e) {
        std::cout << "file reading failed: " << e.what() << std::endl;
        return 1;
    }

    auto fwidth = static_cast<float>(width), fheight = static_cast<float>(height);

    Renderer renderer("../models/2.obj");
    auto mesh = renderer.GetMesh();

    std::vector<Renderer::Vertex> flatVerts;
    for (const auto &verts: mesh | std::views::keys)
        flatVerts.insert(flatVerts.end(), verts.begin(), verts.end());

    unsigned int modelProg = glCreateProgram();
    unsigned int modelVert = compileShader(GL_VERTEX_SHADER, modelVShaderText.c_str());
    unsigned int modelFrag = compileShader(GL_FRAGMENT_SHADER, modelFShaderText.c_str());
    unsigned int modelVAO, modelVBO;
    glGenVertexArrays(1, &modelVAO);
    glGenBuffers(1, &modelVBO);

    glAttachShader(modelProg, modelVert);
    glAttachShader(modelProg, modelFrag);
    glLinkProgram(modelProg);
    glDeleteShader(modelVert);
    glDeleteShader(modelFrag);

    glBindVertexArray(modelVAO);
    glBindBuffer(GL_ARRAY_BUFFER, modelVBO);
    glBufferData(GL_ARRAY_BUFFER,  static_cast<GLsizeiptr>(flatVerts.size() * sizeof(Renderer::Vertex)), flatVerts.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Renderer::Vertex), reinterpret_cast<void *>(offsetof(Renderer::Vertex, position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Renderer::Vertex), reinterpret_cast<void *>(offsetof(Renderer::Vertex, normal)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Renderer::Vertex), reinterpret_cast<void *>(offsetof(Renderer::Vertex, tex)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    std::vector<cv::Point2f> baseFig = { // прямоугольник 3:2
        {-1.5, -1}, // LB
        {1.5, -1}, // RB
        {1.5, 1}, // RT
        {-1.5, 1}, // LT
    };
    double avgSideSize = 0.0;
    for (int i = 0; i < 4; ++i) {
        avgSideSize += cv::norm(baseFig[i] - baseFig[(i + 1) % 4]);
    }
    avgSideSize /= 4.0;
    // const float avgSideSize = 2.5f;

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    const glm::mat4 model = renderer.TransformMatrix(avgSideSize);

    const GLint modelLoc = glGetUniformLocation(modelProg, "model");
    const GLint viewLoc = glGetUniformLocation(modelProg, "view");
    const GLint projectionLoc = glGetUniformLocation(modelProg, "projection");
    const GLint lightPosLoc = glGetUniformLocation(modelProg, "lightWorldPos");

    const GLint texLoc = glGetUniformLocation(modelProg, "tex");
    const GLint KaLoc = glGetUniformLocation(modelProg, "Ka");
    const GLint KsLoc = glGetUniformLocation(modelProg, "Ks");
    const GLint lightColorLoc = glGetUniformLocation(modelProg, "lightColor");
    const GLint NsLoc = glGetUniformLocation(modelProg, "Ns");

    const Renderer::mtlTex* prev = nullptr;
    GLuint texID;
    std::vector<cv::Point2f> prevPoints;
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        cv::Mat frame;
        if (!cap.read(frame)) break;

        // обработка изображения
        cv::Mat blur;
        cv::GaussianBlur(frame, blur, cv::Size(3, 3), 0);
        cv::Mat gray;
        cv::cvtColor(blur, gray, cv::COLOR_BGR2GRAY);
        cv::Mat edges;
        cv::Canny(gray, edges,
            cv::getTrackbarPos("threshold1", trackWindowName),
            cv::getTrackbarPos("threshold2", trackWindowName)
        );
        cv::Mat dil;
        cv::dilate(edges, dil, cv::Mat::ones(5, 5, CV_8U));

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(dil, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        double bestv = 0;
        std::vector<cv::Point> besti;

        for (const auto & contour : contours) {
            if (const double area = std::fabs(cv::contourArea(contour)); area > 1500 && area > bestv) {
                std::vector<cv::Point> approx;
                cv::approxPolyDP(contour, approx, 0.02 * cv::arcLength(contour, true), true);
                if (approx.size() != 4 || !cv::isContourConvex(approx)) {
                    continue;
                }
                besti = approx;
                bestv = area;
            }
        }

        std::vector<cv::Point2f> figurePoints;
        if (!besti.empty()) {
            cv::drawContours(frame,
                std::vector<std::vector<cv::Point>>{besti},
                -1,
                cv::Scalar(200, 200, 0),
                2
            );

            std::ranges::transform(besti, std::back_inserter(figurePoints), [](const auto& point){ return cv::Point2f(point.x, point.y); });
            figurePoints = MatchToPrevious(figurePoints, prevPoints);
        }

        cv::putText(frame,
            "Angles: " + std::to_string(besti.size()), // 4 или не 4
            cv::Point(30, 30),
            cv::FONT_HERSHEY_SIMPLEX,
            1,
            cv::Scalar(0, 255, 0),
            2,
            cv::LINE_AA
        );
        cv::imshow("frame", frame);

        cv::Mat rgbFrame;
        cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);

        // смена текстуры
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
            rgbFrame.cols, rgbFrame.rows, 0,
            GL_RGB, GL_UNSIGNED_BYTE, rgbFrame.data);

        glUseProgram(prog);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        if (!besti.empty() || !prevPoints.empty()) {
            if (besti.empty()) figurePoints = prevPoints;
            cv::Mat cameraMatrix, distCoeffs;
            BuildCameraMatrix(fheight, fwidth, cameraMatrix, distCoeffs);

            std::vector<cv::Point3f> objectPoints3D;
            objectPoints3D.reserve(baseFig.size());
            std::ranges::transform(baseFig,
                std::back_inserter(objectPoints3D),
                [](const cv::Point2f& p){ return cv::Point3f(p.x, 0.0, -p.y); } // перпендикулярно относительно фрейма
            );

            const glm::mat4 projection = CvMatToGlmProjection(cameraMatrix, fwidth, fheight,
                static_cast<float>(cv::getTrackbarPos("near", trackWindowName)) / 10.f,
                static_cast<float>(cv::getTrackbarPos("far", trackWindowName)) / 10.f
            );

            cv::Mat rvec, tvec;
            if (!cv::solvePnP(objectPoints3D, figurePoints, cameraMatrix, distCoeffs, rvec, tvec)) {
                std::cout << "projection error" << std::endl;
                continue;
            }
            const glm::mat4 view = CvPoseToGlmView(rvec, tvec);

            glUseProgram(modelProg);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
            glUniform3f(lightPosLoc, tvec.at<double>(0, 0), tvec.at<double>(1, 0), tvec.at<double>(2, 0));
            glBindVertexArray(modelVAO);

            GLsizei c = 0;
            for (const auto&[verts, mtl] : mesh) {
                if (prev == nullptr || prev != &mtl) {
                    prev = &mtl;
                    texID = mtl.loadMatTexture();
                }
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, texID);
                glUniform1i(texLoc, 1);
                glUniform3fv(KaLoc, 1, glm::value_ptr(mtl.Ka));
                glUniform3fv(KsLoc, 1, glm::value_ptr(mtl.Ks));
                glUniform3f(lightColorLoc, 1.f, 1.f, 1.f);
                glUniform1f(NsLoc, mtl.Ns);

                glDrawArrays(GL_TRIANGLES, c, static_cast<GLsizei>(verts.size()));
                c += static_cast<GLsizei>(verts.size());
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteTextures(1, &texture);
    glDeleteProgram(prog);
    glfwTerminate();
}

void initTrackWindow(const std::string& trackWindowName) {
    cv::namedWindow(trackWindowName);
    cv::createTrackbar("threshold1", trackWindowName, nullptr, 255, nullptr);
    cv::setTrackbarPos("threshold1", trackWindowName, 128);
    cv::createTrackbar("threshold2", trackWindowName, nullptr, 255, nullptr);
    cv::setTrackbarPos("threshold2", trackWindowName, 128);
    cv::createTrackbar("near", trackWindowName, nullptr, 20, nullptr);
    cv::setTrackbarPos("near", trackWindowName, 10);
    cv::createTrackbar("far", trackWindowName, nullptr, 2000, nullptr);
    cv::setTrackbarPos("far", trackWindowName, 1000);
}
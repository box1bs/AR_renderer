#include "utils.h"
#include "renderer.h"

#include <fstream>
#include <iostream>
#include <ranges>

Renderer::Renderer(const char *path) {
    loadObjModel(path);
}

GLuint Renderer::mtlTex::loadMatTexture() const {
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (!map_Kd.empty()) {
        cv::Mat img = cv::imread(map_Kd, cv::IMREAD_COLOR);
        cv::flip(img, img, 0);
        cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.cols, img.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, img.data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        // const glm::vec3 color = glm::clamp(Kd + Ka + Ks, 0.0f, 1.0f);
        const glm::vec3 color = Kd;
        const unsigned char pixel[3] = {
            static_cast<unsigned char>(color.r * 255),
            static_cast<unsigned char>(color.g * 255),
            static_cast<unsigned char>(color.b * 255)
        };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    }
    return texID;
}

std::vector<std::pair<std::vector<Renderer::Vertex>, Renderer::mtlTex>> Renderer::GetMesh() {
    std::vector<std::pair<std::vector<Vertex>, mtlTex>> m;
    // std::cout << materialList.size() << std::endl;
    for (size_t i = 0; i < materialList.size(); ++i) {
        const size_t start = materialList[i].second;
        const size_t end = (i + 1 < materialList.size()) ? materialList[i + 1].second : faceIndex.size();

        std::vector<Vertex> verts;
        verts.reserve(end - start);
        for (size_t j = start; j < end; ++j) {
            verts.emplace_back(vertices[faceIndex[j]], normales[normalIndex[j]], texture[textureIndex[j]]);
        }
        m.emplace_back(std::move(verts), materialBatch[materialList[i].first]);
    }
    if (m.empty()) { // if vertices isn't empty but materials is
        std::vector<Vertex> verts;
        for (size_t j = 0; j < textureIndex.size(); ++j) {
            verts.emplace_back(vertices[faceIndex[j]], normales[normalIndex[j]], texture[textureIndex[j]]);
        }
        m.emplace_back(std::move(verts), mtlTex{.Kd = glm::vec3(1.f)});
    }
    return m;
}

[[nodiscard]] glm::mat4 Renderer::TransformMatrix(const float& avgSideLen) const {
    const glm::vec3 center = (bboxMax + bboxMin) * 0.5f;
    glm::mat4 transformMatrix(1.0f);
    // transformMatrix = glm::scale(transformMatrix, glm::vec3(diagRelation));
    transformMatrix = glm::scale(transformMatrix, glm::vec3(avgSideLen / glm::length(bboxMax - bboxMin)));
    return glm::translate(transformMatrix, -center);
}

void Renderer::loadObjModel(const char* path) {
    clear();
    if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        std::cerr << "file isn't exist: " << path << std::endl;
    }
    double xmin = std::numeric_limits<double>::max(), xmax = std::numeric_limits<double>::lowest();
    double ymin = std::numeric_limits<double>::max(), ymax = std::numeric_limits<double>::lowest();
    double zmin = std::numeric_limits<double>::max(), zmax = std::numeric_limits<double>::lowest();
    std::ifstream objFile(path);

    if (!objFile) {
        std::cout << "file isn't open: " << path << std::endl;
        exit(1);
    }

    std::string matname, line, pref;
    while (std::getline(objFile, line)) {
        pref = line.substr(0, 7);
        if (pref == "usemtl ") {
            matname = line.substr(7);
            materialList.emplace_back(matname, faceIndex.size());
            continue;
        }

        if (pref == "mtllib ") {
            auto spath = static_cast<std::string>(path);
            const size_t pos = spath.find_last_of('/');
            const std::string& rel = line.substr(7);
            if (pos != std::string::npos) {
                spath.replace(pos+1, std::string::npos, rel);
                loadMtl(spath.c_str());
            } else {
                loadMtl(rel.c_str());
            }
            continue;
        }

        if (const auto& subs = pref.substr(0, 2); subs == "v ") {
            std::istringstream v(line.substr(2));
            double x, y, z;
            v>>x>>y>>z;
            xmin = std::min(xmin, x); xmax = std::max(xmax, x);
            ymin = std::min(ymin, y); ymax = std::max(ymax, y);
            zmin = std::min(zmin, z); zmax = std::max(zmax, z);
            vertices.emplace_back(x, y, z);

        } else if (subs == "vt") {
            std::istringstream v(line.substr(3));
            double U, V;
            v>>U>>V;
            texture.emplace_back(U, V);

        } else if (subs == "vn") {
            std::istringstream v(line.substr(3));
            double x, y, z;
            v>>x>>y>>z;
            normales.emplace_back(x, y, z);

        } else if (subs == "f ") {
            int a, b, c, A, B, C, xn, yn, zn;
            const char* ch = line.c_str();
            sscanf_s(ch, "f %i/%i/%i %i/%i/%i %i/%i/%i",&a,&A,&xn,&b,&B,&yn,&c,&C,&zn);
            a--;b--;c--;
            A--;B--;C--;
            xn--;yn--;zn--;

            faceIndex.push_back(a);
            faceIndex.push_back(b);
            faceIndex.push_back(c);

            textureIndex.push_back(A);
            textureIndex.push_back(B);
            textureIndex.push_back(C);

            normalIndex.push_back(xn);
            normalIndex.push_back(yn);
            normalIndex.push_back(zn);
        }
    }
    bboxMax = glm::vec3(xmax, ymax, zmax);
    bboxMin = glm::vec3(xmin, ymin, zmin);
}

void Renderer::loadMtl(const char *path) {
    if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        std::cerr << "file isn't exist: " << path << std::endl;
    }
    mtlTex* tex = nullptr;

    std::ifstream mtlFile(path);
    if (!mtlFile) {
        std::cout << "file isn't open: " << path << std::endl;
        exit(1);
    }

    std::string line;
    while (std::getline(mtlFile, line)) {
        if (line.starts_with("newmtl ")) {
            auto [it, inserted] = materialBatch.emplace(line.substr(7), mtlTex{});
            tex = &it->second;
        } else if (line.starts_with("map_Kd ")) {
            std::istringstream ss(line.substr(7));
            ss >> tex->map_Kd;
        } else if (line.starts_with("Ns ")) {
            std::istringstream ss(line.substr(3));
            ss >> tex->Ns;
        } else if (line.starts_with("Ka ")) {
            std::istringstream ss(line.substr(3));
            double r, g, b;
            ss>>r>>g>>b;
            tex->Ka = glm::vec3(r, g, b);
        } else if (line.starts_with("Kd ")) {
            std::istringstream ss(line.substr(3));
            double r, g, b;
            ss>>r>>g>>b;
            tex->Kd = glm::vec3(r, g, b);
        } else if (line.starts_with("Ks ")) {
            std::istringstream ss(line.substr(3));
            double r, g, b;
            ss>>r>>g>>b;
            tex->Ks = glm::vec3(r, g, b);
        } else if (line.starts_with("Ke ")) {
            std::istringstream ss(line.substr(3));
            double r, g, b;
            ss>>r>>g>>b;
            tex->Ke = glm::vec3(r, g, b);
        } else if (line.starts_with("Ni ")) {
            std::istringstream ss(line.substr(3));
            ss >> tex->Ni;
        } else if (line.starts_with("d ")) {
            std::istringstream ss(line.substr(2));
            ss >> tex->d;
        } else if (line.starts_with("illum ")) {
            std::istringstream ss(line.substr(6));
            ss >> tex->illum;
        }
    }
}

void Renderer::clear() {
    faceIndex = {};
    textureIndex = {};
    vertices = {};
    texture = {};
    materialList = {};
}
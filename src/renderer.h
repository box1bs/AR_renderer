#ifndef RECREN_RENDERER_H
#define RECREN_RENDERER_H
#include "utils.h"
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include <GL/gl.h>

class Renderer {
    glm::vec3 bboxMin = {};
    glm::vec3 bboxMax = {};
    std::vector<int> faceIndex;
    std::vector<int> textureIndex;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texture;

public:
    struct Vertex {
        glm::vec3 position;
        glm::vec2 tex;
    };

    struct mtlTex {
        float Ns; // shininess
        glm::vec3 Ka; // ambient color
        glm::vec3 Kd; // diffuse color
        glm::vec3 Ks; // specular color
        glm::vec3 Ke; // emissive color
        float Ni; // optical dencity
        float d; // opacity
        int illum;
        std::string map_Kd;

        GLuint loadMatTexture() const;
    };

    explicit Renderer(const char* path);

    std::vector<std::pair<std::vector<Vertex>, mtlTex>> GetMesh();

    [[nodiscard]] glm::mat4 TransformMatrix(const float& hPercentage, const float& wPercentage) const;

private:
    std::vector<std::pair<std::string, int>> materialList;
    std::unordered_map<std::string, mtlTex> materialBatch;

    void loadObjModel(const char* path);

    void loadMtl(const char* path);

    [[nodiscard]] float getOptimalScale(const float& hPercentage, const float& wPercentage) const;

    void clear();
};

#endif //RECREN_RENDERER_H
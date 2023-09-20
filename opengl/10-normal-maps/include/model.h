#ifndef __MODEL_H__
#define __MODEL_H__
#include <cstdlib>
#include <vector>
#include <string>

#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "glad/glad.h"

#include "image.h"
#include "log.h"
#include "shader.h"

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 tex_coord;
};

struct Texture {
  unsigned int id;
  std::string name;
  std::string path;
};


class Mesh {
 public:
  Mesh(std::vector<Vertex> vertices,
       std::vector<unsigned> indices,
       std::vector<Texture> textures)
      : vertices_(vertices), indices_(indices), textures_(textures) {
    SetupMesh_();
  }
  void Draw(Shader &shader) {
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    for(unsigned int i = 0; i < textures_.size(); i++) {
      glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding
      // retrieve texture number (the N in diffuse_textureN)
      std::string number;
      std::string name = textures_[i].name;
      if(name == "texture_diffuse")
        number = std::to_string(diffuseNr++);
      else if(name == "texture_specular")
        number = std::to_string(specularNr++);

      shader.SetUniform1i(("material." + name + number).c_str(), i);
      glBindTexture(GL_TEXTURE_2D, textures_[i].id);
    }
    glActiveTexture(GL_TEXTURE0);

    // draw mesh
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indices_.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
  }
 private:
  void SetupMesh_() {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
  
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Vertex), &vertices_[0], GL_STATIC_DRAW);  

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(unsigned int), 
                 &indices_[0], GL_STATIC_DRAW);

    // vertex positions
    glEnableVertexAttribArray(0);	
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    // vertex normals
    glEnableVertexAttribArray(1);	
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    // vertex texture coords
    glEnableVertexAttribArray(2);	
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex_coord));

    glBindVertexArray(0);
  }

  std::vector<Vertex> vertices_;
  std::vector<unsigned> indices_;
  std::vector<Texture> textures_;
  unsigned vao_;
  unsigned vbo_;
  unsigned ebo_;
};

class Model {
 public:
  Model(const std::string &path) {
    LOG("loading model %s...\n", path.c_str());
    LoadModel_(path);
    LOG("model loaded\n");
  }
  void Draw(Shader &shader) {
    for (auto &m : meshes_) {
      m.Draw(shader);
    }
  }
 private:
  std::vector<Texture> LoadMaterialTextures_(aiMaterial *mat, aiTextureType type,
                                             std::string typeName) {
    std::vector<Texture> textures;
    for(unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
      aiString str;
      mat->GetTexture(type, i, &str);
      std::string path = directory_ + "/" + str.C_Str();
      bool skipped = false;
      for (auto &texture_loaded : textures_loaded_) {
        if (texture_loaded.path == path) {
          skipped = true;
          textures.emplace_back(texture_loaded);
          break;
        }
      }
      if (!skipped) {
        Texture texture;
        texture.id = CreateTextureFromImage(path);
        texture.name = typeName;
        texture.path = path;
        textures.emplace_back(texture);
        textures_loaded_.emplace_back(texture);
      }
    }
    return textures;
  }

  void LoadModel_(const std::string &path) {
    Assimp::Importer importer;
    LOG("loading file using Assimp...\n");
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene) {
      LOG_ERROR("failed to load model %s, reason: %s\n", path.c_str(), importer.GetErrorString());
      exit(EXIT_FAILURE);
    }
    LOG("file loaded\n");
    directory_ = path.substr(0, path.find_last_of('/'));
    ProcessNode(scene->mRootNode, scene);
  }

  Mesh ProcessMesh(aiMesh *mesh, const aiScene *scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
      Vertex vertex;
      // process vertex positions, normals and texture coordinates
      glm::vec3 vector; 
      vector.x = mesh->mVertices[i].x;
      vector.y = mesh->mVertices[i].y;
      vector.z = mesh->mVertices[i].z; 
      vertex.position = vector;
      vector.x = mesh->mNormals[i].x;
      vector.y = mesh->mNormals[i].y;
      vector.z = mesh->mNormals[i].z;
      vertex.normal = vector;
      if (mesh->mTextureCoords[0]) {
        glm::vec2 tex_coord;
        tex_coord.x = mesh->mTextureCoords[0][i].x; 
        tex_coord.y = mesh->mTextureCoords[0][i].y;
        vertex.tex_coord = tex_coord;
      } else {
        vertex.tex_coord = glm::vec2(0.0f, 0.0f);
      }
      vertices.push_back(vertex);
    }
    // process indices
    for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
      aiFace face = mesh->mFaces[i];
      for(unsigned int j = 0; j < face.mNumIndices; j++)
        indices.push_back(face.mIndices[j]);
    }
    // process material
    if(mesh->mMaterialIndex >= 0) {
      aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
      std::vector<Texture> diffuseMaps = LoadMaterialTextures_(material, 
                                          aiTextureType_DIFFUSE, "texture_diffuse");
      textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
      std::vector<Texture> specularMaps = LoadMaterialTextures_(material, 
                                          aiTextureType_SPECULAR, "texture_specular");
      textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    }

    return Mesh(vertices, indices, textures);
  }

  void ProcessNode(aiNode *node, const aiScene *scene) {
    static size_t node_count = 0;
    LOG("processing node %lu\n", node_count++);
    if (node == nullptr) {
      LOG_ERROR("invalid node\n");
      exit(EXIT_FAILURE);
    }
    // process all the node's meshes (if any)
    for(unsigned int i = 0; i < node->mNumMeshes; i++) {
      aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; 
      meshes_.emplace_back(ProcessMesh(mesh, scene));			
    }
    // then do the same for each of its children
    for(unsigned int i = 0; i < node->mNumChildren; i++) {
      ProcessNode(node->mChildren[i], scene);
    }
  }

  std::vector<Mesh> meshes_;
  std::string directory_;
  std::vector<Texture> textures_loaded_;
};
#endif // !__MODEL_H__

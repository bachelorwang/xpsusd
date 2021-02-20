#pragma once
#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec3f.h>
#include <cstdint>
#include "utility.h"

struct XpsItem {
  uint32_t type;
  uint32_t count;
  uint32_t info;
};

struct XpsFlag {
  uint32_t flag;
  uint32_t value;
};

struct XpsPose {
  std::string name;
  PXR_NS::GfVec3f rotation;
  PXR_NS::GfVec3f position;
  PXR_NS::GfVec3f scale;
};

struct XpsHeader {
  struct {
    uint16_t major;
    uint16_t minor;
  } version;
  std::string name;
  uint32_t setting_count;
  std::string machine;
  std::string user;
  std::string file;

  uint32_t hash;
  std::vector<XpsItem> items;
  std::vector<XpsFlag> flags;
  std::unordered_map<std::string, XpsPose> poses;
};

struct XpsBone {
  std::string name;
  int16_t parent;
  PXR_NS::GfVec3f position;
};

struct XpsSkinWeights {
  std::vector<int16_t> bones;
  std::vector<float> weights;
};

struct XpsVertex {
  PXR_NS::GfVec3f position;
  PXR_NS::GfVec3f normal;
  uint8_t color[4];
  std::vector<PXR_NS::GfVec2f> uvs;
  XpsSkinWeights skin;
};

struct XpsTexture {
  std::string path;
  uint32_t layer;
};

struct XpsMesh {
  std::string name;
  uint32_t uv_layer_count;
  std::vector<XpsTexture> textures;
  std::vector<XpsVertex> vertices;
  std::vector<uint32_t> indices;
};

struct XpsModel {
  std::vector<XpsBone> bones;
  std::vector<XpsMesh> meshes;
};

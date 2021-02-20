#include "xps_reader.h"
#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec3f.h>
#include <boost/scope_exit.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include "bin_io.h"
#include "xps.h"

PXR_NAMESPACE_USING_DIRECTIVE;

class XpsReaderImp {
  const uint32_t XPS_HEADER_MAGIC_NUM = 323232;

 public:
  XpsReaderImp() = default;
  Ptr<XpsModel> readBinaryXps(const char* file_path);
  ~XpsReaderImp() = default;

 private:
  bool readHeader(std::ifstream& istream);
  bool readBones(std::ifstream& istream);
  bool readTextures(std::ifstream& istream, XpsMesh& mesh);
  bool readVertices(std::istream& istream, XpsMesh& mesh);
  bool readIndices(std::ifstream& istream, XpsMesh& mesh);
  bool readMeshes(std::ifstream& istream);
  bool readModel(std::ifstream& istream);

 private:
  Ptr<XpsHeader> _header;
  Ptr<XpsModel> _model;
};

std::stringstream& operator>>(std::stringstream& ss, GfVec3f& vec) {
  for (size_t e = 0; e < vec.dimension; ++e) {
    ss >> *(vec.data() + e);
  }
  return ss;
}

bool XpsReaderImp::readHeader(std::ifstream& istream) {
  uint32_t result;
  read_value(istream, result);
  if (XPS_HEADER_MAGIC_NUM != result)
    return false;
  _header = std::make_shared<XpsHeader>();
  read_value(istream, _header->version.major);
  read_value(istream, _header->version.minor);
  read_string(istream, _header->name);
  read_value(istream, _header->setting_count);
  read_string(istream, _header->machine);
  read_string(istream, _header->user);
  read_string(istream, _header->file);
  if (_header->version.major <= 2 && _header->version.minor <= 12) {
    return false;
  } else {
    enum ItemType { Dummy = 0, Pose = 1, Flag = 2 };
    const auto block_start = istream.tellg();
    read_value(istream, _header->hash);
    uint32_t item_count;
    read_value(istream, item_count);
    _header->items.resize(item_count);
    for (auto& item : _header->items) {
      read_value(istream, item.type);
      read_value(istream, item.count);
      read_value(istream, item.info);

      switch (item.type) {
        case Dummy:
          istream.seekg(item.count * sizeof(uint32_t), std::ios::cur);
          break;
        case Pose: {
          const auto start = istream.tellg();
          for (uint32_t i = 0; i < item.info; ++i) {
            std::string line;
            read_line(istream, line);
            XpsPose bone;
            auto offset = line.find_first_of(':');
            assert(offset > 0);
            if (offset > 0) {
              bone.name = std::string_view(line.c_str(), offset);
              std::stringstream ss;
              offset += 1;
              assert(offset < line.size());
              ss << std::string_view(line.c_str() + offset,
                                     line.size() - offset);
              ss >> bone.rotation;
              ss >> bone.position;
              ss >> bone.scale;
            }
            _header->poses[bone.name] = bone;
          }
          size_t curr = istream.tellg();
          const auto aligned = (curr + 4 - 1) / 4 * 4;
          assert(aligned >= curr);
          istream.seekg(aligned - curr, std::ios::cur);
        } break;
        case Flag:
          for (uint32_t i = 0; i < item.count; ++i) {
            XpsFlag flag;
            read_value(istream, flag.flag);
            read_value(istream, flag.value);
            _header->flags.push_back(flag);
          }
          break;
        default:
          auto offset = static_cast<std::streamoff>(_header->setting_count) * 4;
          istream.seekg(block_start + offset);
          break;
      }
    }
  }
  return true;
}

bool XpsReaderImp::readBones(std::ifstream& istream) {
  uint32_t bone_count;
  read_value(istream, bone_count);
  _model->bones.resize(bone_count);
  for (auto& bone : _model->bones) {
    read_string(istream, bone.name);
    read_value(istream, bone.parent);
    read_vector(istream, bone.position);
  }
  return true;
}

bool XpsReaderImp::readTextures(std::ifstream& istream, XpsMesh& mesh) {
  uint32_t texture_count;
  read_value(istream, texture_count);
  mesh.textures.resize(texture_count);
  for (auto& texture : mesh.textures) {
    read_string(istream, texture.path);
    read_value(istream, texture.layer);
  }
  return true;
}

bool XpsReaderImp::readVertices(std::istream& istream, XpsMesh& mesh) {
  uint32_t vertex_count;
  read_value(istream, vertex_count);
  mesh.vertices.resize(vertex_count);
  const bool vary_weighted = _header->version.major > 2;
  const bool skined = _model->bones.size() > 0;

  for (auto& vertex : mesh.vertices) {
    read_vector(istream, vertex.position);
    read_vector(istream, vertex.normal);
    read_value(istream, vertex.color);
    vertex.uvs.resize(mesh.uv_layer_count);
    for (auto& uv : vertex.uvs)
      read_vector(istream, uv);
    if (!skined)
      continue;
    int16_t weight_count = 4;
    if (vary_weighted) {
      read_value(istream, weight_count);
    }
    if (weight_count < 0)
      continue;
    vertex.skin.bones.resize(weight_count);
    vertex.skin.weights.resize(weight_count);
    read_vector(istream, vertex.skin.bones);
    read_vector(istream, vertex.skin.weights);
  }
  return true;
}

bool XpsReaderImp::readIndices(std::ifstream& istream, XpsMesh& mesh) {
  uint32_t count;
  read_value(istream, count);
  mesh.indices.resize((size_t)count * 3);
  read_vector(istream, mesh.indices);
  return true;
}

bool XpsReaderImp::readMeshes(std::ifstream& istream) {
  uint32_t mesh_count;
  read_value(istream, mesh_count);
  _model->meshes.resize(mesh_count);

  for (auto& mesh : _model->meshes) {
    read_string(istream, mesh.name);
    read_value(istream, mesh.uv_layer_count);
    readTextures(istream, mesh);
    readVertices(istream, mesh);
    readIndices(istream, mesh);
  }
  return true;
}

bool XpsReaderImp::readModel(std::ifstream& istream) {
  _model = std::make_shared<XpsModel>();
  bool result = true;
  result &= readBones(istream);
  result &= readMeshes(istream);
  return result;
}

Ptr<XpsModel> XpsReaderImp::readBinaryXps(const char* file_path) {
  _header.reset();
  _model.reset();
  std::filesystem::path xps_file_path(file_path);
  if (!std::filesystem::exists(xps_file_path)) {
    return nullptr;
  }
  std::ifstream istream(xps_file_path, std::ios::binary);
  BOOST_SCOPE_EXIT(&istream) { istream.close(); }
  BOOST_SCOPE_EXIT_END;
  if (!readHeader(istream))
    return nullptr;
  if (!readModel(istream))
    _model.reset();
  return _model;
}

XpsReader::XpsReader() {
  _pimp = std::make_shared<XpsReaderImp>();
}

Ptr<XpsModel> XpsReader::readBinaryXps(const char* file_path) {
  return _pimp->readBinaryXps(file_path);
}

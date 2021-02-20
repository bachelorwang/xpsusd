#include <pxr/base/arch/error.h>
#include <pxr/base/arch/systemInfo.h>
#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/tokens.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/sphere.h>
#include <pxr/usd/usdSkel/root.h>
#include <pxr/usd/usdSkel/skeleton.h>
#include <argparse/argparse.hpp>
#include <boost/scope_exit.hpp>
#include "xps.h"
#include "xps_reader.h"

PXR_NAMESPACE_USING_DIRECTIVE;

TF_DEFINE_PRIVATE_TOKENS(XPS_TOKEN, (root));

int InitializeModule() {
  std::string currdir = TfGetPathName(ArchGetExecutablePath());
  std::string pluginDir = TfStringCatPaths(currdir, "usd");

  const auto& validPlugins =
      PlugRegistry::GetInstance().RegisterPlugins(pluginDir);
  return validPlugins.size() > 0 ? SUCCESS : EXTERNAL_ERROR;
}

int main(int argc, char** argv) {
  auto result = InitializeModule();
  if (SUCCESS != result) {
    ARCH_ERROR("failed to load module");
  }
  argparse::ArgumentParser parser;
  parser.add_argument("-i").required();
  parser.add_argument("-o").required();
  try {
    parser.parse_args(argc, argv);
  } catch (const std::runtime_error& err) {
    ARCH_ERROR(err.what());
  }
  XpsReader reader;
  const auto input_file_path = parser.get<std::string>("-i");
  auto model = reader.readBinaryXps(input_file_path.c_str());
  if (!model) {
    ARCH_ERROR("failed to load model");
  }
  const std::string output_path = parser.get<std::string>("-o");

  auto stage = UsdStage::CreateNew(output_path);
  if (!stage) {
    ARCH_ERROR("failed to create stage");
  }
  BOOST_SCOPE_EXIT(stage) { stage->GetRootLayer()->Save(); }
  BOOST_SCOPE_EXIT_END;

  auto root = UsdSkelRoot::Define(
      stage, SdfPath::AbsoluteRootPath().AppendChild(XPS_TOKEN->root));
  if (!stage) {
    ARCH_ERROR("failed to create stage");
  }
  for (const auto& xps_mesh : model->meshes) {
    auto name = xps_mesh.name.empty() ? std::to_string((size_t)&xps_mesh)
                                      : xps_mesh.name;
    if (!TfIsValidIdentifier(name))
      name = TfMakeValidIdentifier(name);
    const auto&& mesh_path = root.GetPath().AppendChild(TfToken(name));
    auto mesh = UsdGeomMesh::Define(stage, mesh_path);
    VtVec3fArray positions(xps_mesh.vertices.size());
    GfRange3f extent;
    for (size_t i = 0; i < positions.size(); ++i) {
      positions[i] = xps_mesh.vertices[i].position;
      extent.UnionWith(positions[i]);
    }
    VtVec3fArray extent_arr({extent.GetMin(), extent.GetMax()});
    mesh.GetExtentAttr().Set(extent_arr);
    mesh.GetPointsAttr().Set(positions);

    assert(xps_mesh.indices.size() % 3 == 0);
    const auto face_count = xps_mesh.indices.size() / 3;
    VtIntArray indices(face_count * 3);
    VtIntArray face_counts(face_count, 3);
    size_t last = 0;
    for (size_t f = 0; f < face_count; ++f) {
      const auto count = face_counts[f];
      for (int i = 0; i < count; ++i)
        indices[last + i] = xps_mesh.indices[last + i];
      last += count;
    }
    mesh.GetFaceVertexIndicesAttr().Set(indices);
    mesh.GetFaceVertexCountsAttr().Set(face_counts);
  }
  return result;
}

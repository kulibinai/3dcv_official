#include <cfloat>
#include <cstring>
#include <fstream>
#include <cstdio>
#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "mesh.h"

namespace cmesh4 {

void SaveMeshToObj(const char* a_fileName, const SimpleMesh &mesh)
{
  std::ofstream out(a_fileName);
  if (!out)
  {
    printf("[SaveMeshToObj::ERROR] Failed to create output file: %s\n", a_fileName);
    return;
  }

  std::string header = "# obj file created by custom obj loader\n";
  std::string o_data = "o MainModel\n";
  std::string v_data;
  std::string tc_data;
  std::string n_data;
  std::string s_data = "s off\n";
  std::string f_data;

  int sz = mesh.vPos4f.size();
  assert(mesh.vNorm4f.size() == sz);
  assert(mesh.vTexCoord2f.size() == sz);

  for (int i = 0; i < sz; ++i)
  {
    v_data += "v " + std::to_string(mesh.vPos4f[i].x) + " " + std::to_string(mesh.vPos4f[i].y) + " " + std::to_string(mesh.vPos4f[i].z) + "\n";
    n_data += "vn " + std::to_string(mesh.vNorm4f[i].x) + " " + std::to_string(mesh.vNorm4f[i].y) + " " + std::to_string(mesh.vNorm4f[i].z) + "\n";
    tc_data += "vt " + std::to_string(mesh.vTexCoord2f[i].x) + " " + std::to_string(mesh.vTexCoord2f[i].y) + "\n";
  }
  for (int i = 0; i < mesh.indices.size() / 3; ++i)
  {
    f_data += "f " + std::to_string(mesh.indices[3*i]+1) + "/" + std::to_string(mesh.indices[3*i]+1) + "/" + std::to_string(mesh.indices[3*i]+1) + " " +
      std::to_string(mesh.indices[3*i+1]+1) + "/" + std::to_string(mesh.indices[3*i+1]+1) + "/" + std::to_string(mesh.indices[3*i+1]+1) + " " +
      std::to_string(mesh.indices[3*i+2]+1) + "/" + std::to_string(mesh.indices[3*i+2]+1) + "/" + std::to_string(mesh.indices[3*i+2]+1) + "\n";
  }
  out << header;
  out << o_data;
  out << v_data;
  out << tc_data;
  out << n_data;
  out << s_data;
  out << f_data;
  out.close();
}

bool check_is_valid(const cmesh4::SimpleMesh &mesh, bool verbose)
{
  bool valid = true;

  if (mesh.vPos4f.size() == 0)
  {
    if (verbose)
      printf("[check_is_valid::ERROR]: mesh has no vertices\n");
    valid = false;
  }

  if (mesh.vPos4f.size() != mesh.vNorm4f.size())
  {
    if (verbose)
      printf("[check_is_valid::ERROR]: mesh has different number of vertices and normals\n");
    valid = false;
  }

  if (mesh.vPos4f.size() != mesh.vTang4f.size())
  {
    if (verbose)
      printf("[check_is_valid::ERROR]: mesh has different number of vertices and tangents\n");
    valid = false;
  }

  if (mesh.vPos4f.size() != mesh.vTexCoord2f.size())
  {
    if (verbose)
      printf("[check_is_valid::ERROR]: mesh has different number of vertices and texture coordinates\n");
    valid = false;
  }

  if (mesh.indices.size() == 0)
  {
    if (verbose)
      printf("[check_is_valid::ERROR]: mesh has no indices\n");
    valid = false;
  }

  if (mesh.indices.size() % 3 != 0)
  {
    if (verbose)
      printf("[check_is_valid::ERROR]: mesh has invalid number of indices (should be 3*num_of_triangles)\n");
    valid = false;
  }

  if (3*mesh.matIndices.size() != mesh.indices.size())
  {
    if (verbose)
      printf("[check_is_valid::ERROR]: mesh has different number of triangles and material indices\n");
    valid = false;
  }

  for (int i=0;i<mesh.indices.size();i+=3)
  {
    int i1 = mesh.indices[i];
    int i2 = mesh.indices[i+1];
    int i3 = mesh.indices[i+2];

    if (i1 == i2 || i1 == i3 || i2 == i3)
    {
      if (verbose)
        printf("[check_is_valid::ERROR]: triangle %d has duplicate vertices (%d, %d, %d)\n", i/3, i1, i2, i3);
      valid = false;
    }

    if (i1 >= mesh.vPos4f.size() || i2 >= mesh.vPos4f.size() || i3 >= mesh.vPos4f.size())
    {
      if (verbose)
        printf("[check_is_valid::ERROR]: triangle %d has out of range vertices (%d, %d, %d)\n", i/3, i1, i2, i3);
      valid = false;
    }
  }

  return valid;
}

void fix_missing(cmesh4::SimpleMesh &mesh, int default_mat_id)
{
  const float4 default_norm = float4(0, 0, 1, 0);
  const float4 default_tangent = float4(1, 0, 0, 0);
  const float2 default_texcoord = float2(0, 0);

  assert(mesh.vPos4f.size() >= mesh.vNorm4f.size());
  mesh.vNorm4f.resize(mesh.vPos4f.size(), default_norm);

  assert(mesh.vPos4f.size() >= mesh.vTang4f.size());
  mesh.vTang4f.resize(mesh.vPos4f.size(), default_tangent);

  assert(mesh.vPos4f.size() >= mesh.vTexCoord2f.size());
  mesh.vTexCoord2f.resize(mesh.vPos4f.size(), default_texcoord);

  assert(mesh.indices.size() >= 3*mesh.matIndices.size());
  assert(mesh.indices.size() % 3 == 0);
  mesh.matIndices.resize(mesh.indices.size() / 3, default_mat_id);
}

struct TinyObjIndexEqual
{
  bool operator()(const tinyobj::index_t &lhs, const tinyobj::index_t &rhs) const
  {
    return lhs.vertex_index == rhs.vertex_index &&
          lhs.normal_index == rhs.normal_index &&
          lhs.texcoord_index == rhs.texcoord_index;
  }
};

struct TinyObjIndexHasher
{
  size_t operator()(const tinyobj::index_t &index) const
  {
    return ((std::hash<int>()(index.vertex_index) ^
            (std::hash<int>()(index.normal_index) << 1)) >> 1) ^
            (std::hash<int>()(index.texcoord_index) << 1);
  }
};

SimpleMesh LoadMeshFromObj(const char* a_fileName, bool verbose)
{
  if (verbose)
    printf("[LoadMesh::INFO] Loading OBJ file %s\n", a_fileName);
  SimpleMesh mesh;

  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;

  std::string warn;
  std::string err;

  bool loading_result = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, a_fileName);

  if (!loading_result)
  {
    printf("[LoadMeshFromObj::ERROR] Failed to load obj file: %s\n", err.c_str());
    return mesh;
  }

  if (verbose)
  {
    if (warn.empty())
      printf("[LoadMeshFromObj::INFO] Loaded obj file: %s\n", a_fileName);
    else
      printf("[LoadMeshFromObj::WARNING] Loaded obj file %s with warnings: %s\n", a_fileName, warn.c_str());
  }

  const LiteMath::float4 default_norm = float4(0, 0, 1, 0);
  const LiteMath::float4 default_tangent = float4(1, 0, 0, 0);
  const LiteMath::float2 default_texcoord = float2(0, 0);

  std::unordered_map<tinyobj::index_t, uint32_t, TinyObjIndexHasher, TinyObjIndexEqual> uniqueVertIndices = {};

  uint32_t numIndices = 0;
  for (const auto& shape : shapes)
  numIndices += shape.mesh.indices.size();

  mesh.vPos4f.reserve(attrib.vertices.size() / 3);
  mesh.vNorm4f.reserve(attrib.vertices.size() / 3);
  mesh.vTang4f.reserve(attrib.vertices.size() / 3);
  mesh.vTexCoord2f.reserve(attrib.vertices.size() / 3);
  mesh.indices.reserve(numIndices);

  for (const auto& shape : shapes)
  {
    mesh.matIndices.insert(std::end(mesh.matIndices), std::begin(shape.mesh.material_ids), std::end(shape.mesh.material_ids));

    for (const auto& index : shape.mesh.indices)
    {
      uint32_t my_index = 0;
      auto it = uniqueVertIndices.find(index);
      if (it != uniqueVertIndices.end())
      {
        my_index = it->second;
      }
      else
      {
        uniqueVertIndices.insert({index, static_cast<uint32_t>(mesh.vPos4f.size())});
        my_index = static_cast<uint32_t>(mesh.vPos4f.size());

        assert(index.vertex_index >= 0 && index.vertex_index < attrib.vertices.size() / 3);
        mesh.vPos4f.push_back({attrib.vertices[3 * index.vertex_index + 0],
          attrib.vertices[3 * index.vertex_index + 1],
          attrib.vertices[3 * index.vertex_index + 2],
          1.0f});
        if(index.normal_index >= 0)
        {
          mesh.vNorm4f.push_back({attrib.normals[3 * index.normal_index + 0],
            attrib.normals[3 * index.normal_index + 1],
            attrib.normals[3 * index.normal_index + 2],
            0.0f});
        }
        else
        {
          mesh.vNorm4f.push_back(default_norm);
        }
        if(index.texcoord_index >= 0)
        {
          mesh.vTexCoord2f.push_back({attrib.texcoords[2 * index.texcoord_index + 0],
            attrib.texcoords[2 * index.texcoord_index + 1]});
        }
        else
        {
          mesh.vTexCoord2f.push_back(default_texcoord);
        }
        mesh.vTang4f.push_back(default_tangent);
      }

      mesh.indices.push_back(my_index);
    }
  }

  // fix material id
  for (unsigned &mid : mesh.matIndices) 
  { 
    if(mid == uint32_t(-1)) 
      mid = 0;
  }

  if (verbose)
  {
    printf("[LoadMeshFromObj::INFO] Loaded obj file %s with %d vertices and %d indices\n", 
           a_fileName, (unsigned)mesh.vPos4f.size(), (unsigned)mesh.indices.size());
  }

  fix_missing(mesh, 0);
  assert(check_is_valid(mesh, true));
  return mesh;
}
} // namespace cmesh4

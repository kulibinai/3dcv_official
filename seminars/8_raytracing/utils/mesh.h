#pragma once

#include <vector>
#include <cassert>

#include "utils/LiteMath.h"

namespace cmesh4
{
  using LiteMath::float4;
  using LiteMath::float2;

  // very simple utility mesh representation for working with geometry on the CPU in C++
  struct SimpleMesh
  {
    static const uint64_t POINTS_IN_TRIANGLE = 3;
    SimpleMesh(){}
    SimpleMesh(size_t a_vertNum, size_t a_indNum) { Resize(a_vertNum, a_indNum); }
    SimpleMesh(const SimpleMesh &other) = default;
    SimpleMesh(SimpleMesh &&other) = default;
    SimpleMesh &operator=(const SimpleMesh &other) = default;
    SimpleMesh &operator=(SimpleMesh &&other) = default;

    inline size_t VerticesNum()  const { return vPos4f.size(); }
    inline size_t IndicesNum()   const { return indices.size();  }
    inline size_t TrianglesNum() const { return IndicesNum() / POINTS_IN_TRIANGLE;  }
    inline void   Resize(size_t a_vertNum, size_t a_indNum)
    {
      vPos4f.resize(a_vertNum);
      vNorm4f.resize(a_vertNum);
      vTang4f.resize(a_vertNum);
      vTexCoord2f.resize(a_vertNum);
      indices.resize(a_indNum);
      matIndices.resize(a_indNum/3); 
      assert(a_indNum%3 == 0); // NOTE: CURRENT IMPLEMENTATION ASSUMES ONLY TRIANGLE MESHES
    };

    inline size_t SizeInBytes() const
    {
      return vPos4f.size()*sizeof(float)*4  +
             vNorm4f.size()*sizeof(float)*4 +
             vTang4f.size()*sizeof(float)*4 +
             vTexCoord2f.size()*sizeof(float)*2 +
             indices.size()*sizeof(int) +
             matIndices.size()*sizeof(int);
    }

    std::vector<LiteMath::float4> vPos4f;
    std::vector<LiteMath::float4> vNorm4f;
    std::vector<LiteMath::float4> vTang4f;
    std::vector<LiteMath::float2> vTexCoord2f;
    std::vector<unsigned int>     indices;     // size = 3*TrianglesNum() for triangle mesh, 4*TrianglesNum() for quad mesh
    std::vector<unsigned int>     matIndices;  // size = 1*TrianglesNum()
  };

  void SaveMeshToObj(const char* a_fileName, const cmesh4::SimpleMesh &mesh);
  SimpleMesh LoadMeshFromObj(const char* a_fileName, bool verbose = false);
};

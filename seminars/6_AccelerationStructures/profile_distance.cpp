// Distance to a 2D profile.
//
// A profile is a set of closed polygonal loops. Every loop is a list of points, consecutive points
// are connected by segments and the last point is connected to the first one. Loops do not intersect
// each other or themselves.
//
// Build: g++ -std=c++17 -O3 profile_distance.cpp -o profile_distance
// Run:   ./profile_distance profiles_small.bin

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <random>
#include <string>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "LiteMath.h"

using LiteMath::float2;
using LiteMath::float3;

struct Profile
{
  std::vector<std::vector<float2>> loops;
};

// Binary file format (little-endian):
//   char     magic[4] = "PRF1"
//   uint32   profile_count
//   profile_count times:
//     uint32 loop_count
//     loop_count times:
//       uint32 point_count
//       float  xy[2 * point_count]
std::vector<Profile> read_profiles(const std::string &filename)
{
  std::vector<Profile> profiles;
  FILE *f = fopen(filename.c_str(), "rb");
  if (!f)
  {
    fprintf(stderr, "Cannot open file %s\n", filename.c_str());
    return profiles;
  }

  char magic[4] = {0, 0, 0, 0};
  uint32_t profile_count = 0;
  bool ok = fread(magic, 1, 4, f) == 4 && magic[0] == 'P' && magic[1] == 'R' && magic[2] == 'F' && magic[3] == '1' &&
            fread(&profile_count, sizeof(uint32_t), 1, f) == 1;

  for (uint32_t i = 0; ok && i < profile_count; i++)
  {
    Profile profile;
    uint32_t loop_count = 0;
    ok = fread(&loop_count, sizeof(uint32_t), 1, f) == 1;
    for (uint32_t j = 0; ok && j < loop_count; j++)
    {
      uint32_t point_count = 0;
      ok = fread(&point_count, sizeof(uint32_t), 1, f) == 1;
      std::vector<float> xy(2 * (size_t)point_count);
      ok = ok && fread(xy.data(), sizeof(float), xy.size(), f) == xy.size();
      std::vector<float2> loop(point_count);
      for (uint32_t k = 0; ok && k < point_count; k++)
        loop[k] = float2(xy[2 * k], xy[2 * k + 1]);
      profile.loops.push_back(std::move(loop));
    }
    if (ok)
      profiles.push_back(std::move(profile));
  }
  fclose(f);

  if (!ok)
    fprintf(stderr, "File %s is corrupted, %zu profiles were read\n", filename.c_str(), profiles.size());
  return profiles;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Experiments

// Region used for images: square around the profile AABB with 5% padding on every side
struct Region
{
  float2 min_pos;
  float size;
};

Region get_region(const Profile &profile)
{
  float2 mn(std::numeric_limits<float>::max()), mx(-std::numeric_limits<float>::max());
  for (const std::vector<float2> &loop : profile.loops)
    for (const float2 &p : loop)
    {
      mn = min(mn, p);
      mx = max(mx, p);
    }
  const float2 center = 0.5f * (mn + mx);
  const float size = 1.1f * std::max(mx.x - mn.x, mx.y - mn.y);
  return Region{center - float2(0.5f * size), size};
}

// Centers of pixels of an NxN image covering the profile region, the first row is the top of the image
std::vector<float2> pixel_centers(const Region &region, uint32_t N)
{
  const float pixel = region.size / N;
  std::vector<float2> points((size_t)N * N);
  for (uint32_t y = 0; y < N; y++)
    for (uint32_t x = 0; x < N; x++)
      points[(size_t)y * N + x] = region.min_pos + pixel * float2(x + 0.5f, N - y - 0.5f);
  return points;
}

// Visualizes distances calculated in pixel centers of an NxN image: color gets darker with distance,
// black isoline is the profile boundary, faint stripes show other isolines.
// Returns RGB image, 3 bytes per pixel.
std::vector<uint8_t> distance_image(const std::vector<float> &distances, const Region &region, uint32_t N)
{
  const float pixel = region.size / N;
  const float3 near_color(1.0f, 0.93f, 0.75f), far_color(0.2f, 0.35f, 0.75f);
  std::vector<uint8_t> image(3 * (size_t)N * N);
  for (size_t i = 0; i < distances.size(); i++)
  {
    const float d = distances[i];
    const float shade = 1.0f - std::exp(-6.0f * d / region.size); // 0 near the border, 1 far away
    const float stripes = 0.85f + 0.15f * std::cos(2.0f * LiteMath::M_PI * d / (0.025f * region.size));
    float3 color = stripes * (near_color + shade * (far_color - near_color));
    if (d < pixel)
      color = float3(0.0f);
    for (int c = 0; c < 3; c++)
      image[3 * i + c] = (uint8_t)LiteMath::clamp(255.0f * color[c] + 0.5f, 0.0f, 255.0f);
  }
  return image;
}

static double seconds_since(std::chrono::high_resolution_clock::time_point t0)
{
  return std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t0).count();
}

static void save_image(const std::string &prefix, const char *method, size_t index, const std::vector<uint8_t> &image,
                       uint32_t size)
{
  std::string name = prefix + "_" + method + "_";
  std::replace(name.begin(), name.end(), ' ', '_');
  char image_name[512];
  snprintf(image_name, sizeof(image_name), "%s%04zu.png", name.c_str(), index);
  if (!stbi_write_png(image_name, (int)size, (int)size, 3, image.data(), 3 * (int)size))
    fprintf(stderr, "Cannot save image %s\n", image_name);
}

// Unsigned distance from point p to segment [a, b]
float segment_distance(float2 p, float2 a, float2 b)
{
  //TODO
  return 0;
}

// Distance from point p to the profile, brute force over all segments
float distance_brute_force(const Profile &profile, float2 p)
{
  float min_dist = std::numeric_limits<float>::max();
  for (const std::vector<float2> &loop : profile.loops)
  {
    const size_t n = loop.size();
    for (size_t i = 0; i < n; i++)
      min_dist = std::min(min_dist, segment_distance(p, loop[i], loop[(i + 1) % n]));
  }
  return min_dist;
}

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    printf("Usage: %s <profiles.bin> [image_size=512] [output_prefix=dist]\n",
           argv[0]);
    printf("image_size=0 disables image calculation, output_prefix=- disables saving images\n");
    return 1;
  }
  const std::string filename = argv[1];
  const uint32_t image_size = argc > 3 ? (uint32_t)std::atoi(argv[2]) : 512;
  const std::string prefix = argc > 4 ? argv[3] : "dist";

  const std::vector<Profile> profiles = read_profiles(filename);
  if (profiles.empty())
    return 1;

  for (size_t i = 0; i < profiles.size(); i++)
  {
    const Profile &profile = profiles[i];
    size_t segments = 0;
    for (const std::vector<float2> &loop : profile.loops)
      segments += loop.size();

    auto t0 = std::chrono::high_resolution_clock::now();
    //TODO: build your acceleration structure here
    double t_build = seconds_since(t0);

    const Region region = get_region(profile);
    const std::vector<float2> pixels = pixel_centers(region, image_size);
    std::vector<float> distances(pixels.size());

    t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < pixels.size(); i++)
      distances[i] = distance_brute_force(profile, pixels[i]);
    const double t_image = seconds_since(t0);

    printf("profile %4zu: %2zu loops, %5zu segments, %.3f+%.3f s (%5.1f Mcalls/s)\n", i, profile.loops.size(), segments,
           t_image, t_build, (double)(pixels.size()/(t_image+t_build)/1'000'000));

    save_image(prefix, "BF", i, distance_image(distances, region, image_size), image_size);
  }
  return 0;
}

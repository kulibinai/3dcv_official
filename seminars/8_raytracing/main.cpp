#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "utils/mesh.h"
#include "utils/LiteMath.h"
#include "utils/public_camera.h"
#include "utils/public_image.h"

#include <cstdio>
#include <cstring>
#include <SDL_keycode.h>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <SDL.h>
#include <chrono>

using LiteMath::float2;
using LiteMath::float3;
using LiteMath::float4;
using LiteMath::int2;
using LiteMath::int3;
using LiteMath::int4;
using LiteMath::uint2;
using LiteMath::uint3;
using LiteMath::uint4;

static constexpr int SCREEN_WIDTH  = 1280;
static constexpr int SCREEN_HEIGHT = 960;

float rad_to_deg(float rad) { return rad * 180.0f / LiteMath::M_PI; }

uint32_t float3_to_RGBA8(float3 c)
{
  uint8_t r = (uint8_t)(std::clamp(c.x,0.0f,1.0f)*255.0f);
  uint8_t g = (uint8_t)(std::clamp(c.y,0.0f,1.0f)*255.0f);
  uint8_t b = (uint8_t)(std::clamp(c.z,0.0f,1.0f)*255.0f);
  return 0xFF000000 | (r<<16) | (g<<8) | b;
}

float sphere_SDF(float3 pos)
{
  constexpr float R = 3.0f;
  //constexpr float r = 0.25f;

  return length(pos - float3(0,0,0)) - R;
}

float torus_SDF(float3 pos)
{
  const float R = 4.0f;
  const float r = 0.8f;

  const float2 p2 = float2(LiteMath::length(float2(pos.x, pos.y)) - R, pos.z);
  return length(p2) - r;
}

float combined_SDF(float3 pos)
{
  return std::min(torus_SDF(float3(pos.z, pos.x, pos.y)),std::min(torus_SDF(pos), torus_SDF(float3(pos.z, pos.y, pos.x))));
}

//p: SDF(p) = 0, n(p) = normalize(grad(SDF(p)))

void draw_frame_example(const Camera &camera, std::vector<uint32_t> &pixels)
{
  const LiteMath::float4x4 view = LiteMath::lookAt(camera.pos, camera.target, camera.up);
  const LiteMath::float4x4 proj = LiteMath::perspectiveMatrix((camera.fov_rad/LiteMath::M_PI)*360.0, (float)SCREEN_WIDTH/SCREEN_HEIGHT, camera.z_near, camera.z_far);
  const LiteMath::float4x4 vp_inv = LiteMath::inverse4x4(proj * view);

  const float3 light_dir = normalize(float3(-1,-1,-1));
  const float3 light_color = float3(0.5, 0.5, 0.5);
  const float3 ambient_color = float3(0.1, 0.1, 0.1);

  for (int i=0; i<SCREEN_HEIGHT; i++)
  {
    for (int j=0; j<SCREEN_WIDTH; j++)
    {
      const float4 point_NDC = float4(2.0*((j+0.5)/SCREEN_WIDTH)-1.0, 2.0*((i+0.5)/SCREEN_HEIGHT) - 1.0, 0, 1);
      const float4 point_view = vp_inv * point_NDC;

      const float3 ray_pos = camera.pos;
      const float3 ray_dir = LiteMath::normalize(LiteMath::to_float3(point_view)/point_view.w - ray_pos);

      //printf("[%d %d] pos %f %f %f dir %f %f %f\n", i, j, ray_pos.x, ray_pos.y, ray_pos.z, ray_dir.x, ray_dir.y, ray_dir.z);

      int iters = 0;
      float t = 0.0f;
      float d = combined_SDF(ray_pos + t*ray_dir);

      while (d > 1e-6f && iters < 1000 && d < 10)
      {
        t += d;
        d = combined_SDF(ray_pos + t*ray_dir);
        iters++;
      }

      if (d <= 1e-6f)
      {
        const float3 p0 = ray_pos + t*ray_dir;
        const float dp = 1e-4f;
        const float3 n = LiteMath::normalize(float3(combined_SDF(p0 + float3(dp, 0, 0)) - combined_SDF(p0 - float3(dp, 0, 0)),
                                                    combined_SDF(p0 + float3(0, dp, 0)) - combined_SDF(p0 - float3(0, dp, 0)),
                                                    combined_SDF(p0 + float3(0, 0, dp)) - combined_SDF(p0 - float3(0, 0, dp))));
        const float3 albedo = float3(1,1,1);
        const float3 color = albedo * (ambient_color + std::max(0.0f, -1.0f*dot(n, light_dir)));
        pixels[(SCREEN_HEIGHT - i - 1)*SCREEN_WIDTH + j] = float3_to_RGBA8(color);
      }
      else
      {
        pixels[(SCREEN_HEIGHT - i - 1)*SCREEN_WIDTH + j] = 0xFF000000; 
      }
    }
  }
}

// You must include the command line parameters for your main function to be recognized by SDL
int main(int argc, char **args)
{
  // Pixel buffer (RGBA format)
  std::vector<uint32_t> pixels(SCREEN_WIDTH * SCREEN_HEIGHT, 0xFFFFFFFF); // Initialize with white pixels
  
  //cmesh4::SimpleMesh mesh = cmesh4::LoadMeshFromObj("models/stanford-bunny.obj", true);

  // Initialize SDL. SDL_Init will return -1 if it fails.
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
  {
    std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
    return 1;
  }

  // Create our window
  SDL_Window *window = SDL_CreateWindow("SDF Viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

  // Make sure creating the window succeeded
  if (!window)
  {
    std::cerr << "Error creating window: " << SDL_GetError() << std::endl;
    return 1;
  }

  // Create a renderer
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer)
  {
    std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  // Create a texture
  SDL_Texture *texture = SDL_CreateTexture(
      renderer,
      SDL_PIXELFORMAT_ARGB8888,    // 32-bit RGBA format
      SDL_TEXTUREACCESS_STREAMING, // Allows us to update the texture
      SCREEN_WIDTH,
      SCREEN_HEIGHT);

  if (!texture)
  {
    std::cerr << "Texture could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  SDL_Event ev;
  bool running = true;

  Camera camera;
  camera.pos = float3(0, 0, 7);
  camera.target = float3(0, 0, 0);
  camera.up = float3(0, 1, 0);

  auto time = std::chrono::high_resolution_clock::now();
  auto prev_time = time;
  float time_from_start = 0;
  uint32_t frameNum = 0;

  // Main loop
  while (running)
  {
    // Process keyboard input
    while (SDL_PollEvent(&ev) != 0)
    {
      // check event type
      switch (ev.type)
      {
      case SDL_QUIT:
        // shut down
        running = false;
        break;
      case SDL_KEYDOWN:
        // test keycode
        switch (ev.key.keysym.sym)
        {
        //ESC to exit 
        case SDLK_ESCAPE:
          running = false;
          break;
          // etc
        }
        break;
      }
    }

    //update camera or scene
    prev_time = time;
    time = std::chrono::high_resolution_clock::now();

    //get delta time in seconds
    float dt = std::chrono::duration<float, std::milli>(time - prev_time).count() / 1000.0f;
    time_from_start += dt;
    frameNum++;

    if (frameNum % 10 == 0)
      printf("Render time: %f ms\n", 1000.0f*dt);

    const float rot_speed = 1.0f;
    camera.pos = 7.0f*float3(sinf(rot_speed * time_from_start), 0, cosf(rot_speed * time_from_start));

    // Render the scene
    draw_frame_example(camera, pixels);

    // Update the texture with the pixel buffer
    SDL_UpdateTexture(texture, nullptr, pixels.data(), SCREEN_WIDTH * sizeof(uint32_t));

    // Clear the renderer
    SDL_RenderClear(renderer);

    // Copy the texture to the renderer
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);

    // Update the screen
    SDL_RenderPresent(renderer);
  }

  // Destroy the window. This will also destroy the surface
  SDL_DestroyWindow(window);

  // Quit SDL
  SDL_Quit();

  // End the program
  return 0;
}
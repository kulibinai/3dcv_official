#pragma once
#include "LiteMath.h"
#include <cstring>
#include <cstdio>
#include <errno.h>

using LiteMath::float3;
struct Camera
{
  float3 pos;         // camera position
  float3 target;      // point, at which camera is looking
  float3 up;          // up vector for camera
  float fov_rad = 3.14159265f / 3;    // field of view in radians
  float z_near = 0.1f;                // distance to near plane
  float z_far = 100.0f;               // distance to far plane

  bool to_file(const char *filename)
  {
    FILE *f = fopen(filename, "w");
    if (!f)
    {
      fprintf(stderr, "failed to open/create file %s. Errno %d\n", filename, (int)errno);
      return false;
    }
    fprintf(f, "camera_position = %f, %f, %f\n", pos.x, pos.y, pos.z);
    fprintf(f, "target = %f, %f, %f\n", target.x, target.y, target.z);
    fprintf(f, "up = %f, %f, %f\n", up.x, up.y, up.z);
    fprintf(f, "field_of_view  = %f\n", fov_rad);
    fprintf(f, "z_near  = %f\n", z_near);
    fprintf(f, "z_far  = %f\n", z_far);

    int res = fclose(f);
    if (res != 0)
    {
      fprintf(stderr, "failed to close file %s. fclose returned %d\n", filename, res);
      return false;
    }
    return true;
  }

  bool from_file(const char *filename)
  {
    FILE *f = fopen(filename, "r");
    if (!f)
    {
      fprintf(stderr, "failed to open file %s. Errno %d\n", filename, (int)errno);
      return false;
    }
    fscanf(f, "camera_position = %f, %f, %f\n", &pos.x, &pos.y, &pos.z);
    fscanf(f, "target = %f, %f, %f\n", &target.x, &target.y, &target.z);
    fscanf(f, "up = %f, %f, %f\n", &up.x, &up.y, &up.z);
    fscanf(f, "field_of_view  = %f\n", &fov_rad);
    fscanf(f, "z_near  = %f\n", &z_near);
    fscanf(f, "z_far  = %f\n", &z_far);

    int res = fclose(f);
    if (res != 0)
    {
      fprintf(stderr, "failed to close file %s. fclose returned %d\n", filename, res);
      return false;
    }
    return true;
  }
};

struct DirectedLight
{
  float3 dir; // direction TO light, i.e 0,1,0 if light is above
  float intensity = 1.0f;

  bool to_file(const char *filename)
  {
    FILE *f = fopen(filename, "w");
    if (!f)
    {
      fprintf(stderr, "failed to open/create file %s. Errno %d\n", filename, (int)errno);
      return false;
    }
    fprintf(f, "light direction = %f, %f, %f\n", dir.x, dir.y, dir.z);
    fprintf(f, "intensity = %f\n", intensity);

    int res = fclose(f);
    if (res != 0)
    {
      fprintf(stderr, "failed to close file %s. fclose returned %d\n", filename, res);
      return false;
    }
    return true;
  }

  bool from_file(const char *filename)
  {
    FILE *f = fopen(filename, "r");
    if (!f)
    {
      fprintf(stderr, "failed to open file %s. Errno %d\n", filename, (int)errno);
      return false;
    }
    fscanf(f, "light direction = %f, %f, %f\n", &dir.x, &dir.y, &dir.z);
    fscanf(f, "intensity = %f\n", &intensity);

    int res = fclose(f);
    if (res != 0)
    {
      fprintf(stderr, "failed to close file %s. fclose returned %d\n", filename, res);
      return false;
    }
    return true;
  }
};
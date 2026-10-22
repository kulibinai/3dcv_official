#pragma once

#include "utils/stb_image.h"
#include "utils/stb_image_write.h"

#include <string>
#include <vector>
#include <cassert>

void read_image_rgb(std::string path, std::vector<float> &image_data, int &width, int &height)
{
  int channels;
  unsigned char *imgData = stbi_load(path.c_str(), &width, &height, &channels, 0);

  image_data.resize(3 * width * height, 0);
  for (int i = 0; i < height; i++)
  {
    for (int j = 0; j < width; j++)
    {
      image_data[3 * (i * width + j) + 0] = imgData[channels * (i * width + j) + 0] / 255.0f;
      image_data[3 * (i * width + j) + 1] = imgData[channels * (i * width + j) + 1] / 255.0f;
      image_data[3 * (i * width + j) + 2] = imgData[channels * (i * width + j) + 2] / 255.0f;
    }
  }

  stbi_image_free(imgData);
}

void write_image_rgb(std::string path, const std::vector<float> &image_data, int width, int height)
{
  assert(3 * width * height == image_data.size());
  unsigned char *data = new unsigned char[3 * width * height];
  for (int i = 0; i < 3 * width * height; i++)
    data[i] = std::max(0, std::min(255, (int)(255 * image_data[i])));

  stbi_write_png(path.c_str(), width, height, 3, data, 3 * width);

  delete[] data;
}
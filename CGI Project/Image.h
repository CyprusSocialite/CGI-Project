#ifndef IMAGE_H
#define IMAGE_H

#include <algorithm>
#include <cmath>
#include <vector>

// See https://github.com/nothings/stb
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "Vec3.h"

class Image {
public:
    Image(int w, int h) : width(w), height(h), data(w*h) {}

    Vec3 operator()(int x, int y) const {
		if (x<0 || x>=width || y<0 || y>=width) throw;
        return data[x+y*width];
    }

    Vec3 &operator()(int x, int y) {
		if (x<0 || x>=width || y<0 || y>=width) throw;
        return data[x+y*width];
    }

    int Save(std::string filename) {
        chardata = std::vector<unsigned char>(width*height*3);
        double a;
		int index;

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                index = x + width*y;
                a = std::min(std::max(data[index].x,0.0),1.0);
                chardata[index*3] = 255.0*std::pow(a,1/2.2);
                a = std::min(std::max(data[index].y,0.0),1.0);
                chardata[index*3+1] = 255.0*std::pow(a,1/2.2);
                a = std::min(std::max(data[index].z,0.0),1.0);
                chardata[index*3+2] = 255.0*std::pow(a,1/2.2);
            }
        }

        return !stbi_write_png(filename.c_str(), width, height,
                               3, &(chardata[0]), 3*width);
    }

    int Width() { return width; }
    int Height() { return height; }

private:
    int width, height;
    std::vector<unsigned char> chardata;
    std::vector<Vec3> data;
};

#endif

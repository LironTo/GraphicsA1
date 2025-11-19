#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <glm/glm.hpp>

#include <iostream>


unsigned char* grayScaleImage(unsigned char * buffer, int comps, int width, int height){

    if (buffer == nullptr) {
        std::cerr << "Failed to load image " << std::endl;
        return nullptr;
    }

    float redMultiplier = 0.2989; // NTSC/PAL Standard
    float greenMultiplier = 0.587;
    float blueMultiplier = 0.114;

    unsigned char* grayBuffer = new unsigned char[width * height];
    for(int i = 0; i < width * height; i++){
        glm::vec3 colors = glm::vec3(buffer[i * comps], buffer[i * comps + 1], buffer[i * comps + 2]);
        grayBuffer[i] = redMultiplier * colors.r + greenMultiplier * colors.g + blueMultiplier * colors.b;
    }

    return grayBuffer;
}


int main(void)
{
    std::string filepath = "res/textures/Lenna.png";
    int width, height, comps;
    int req_comps = 4;

    unsigned char *buffer = stbi_load(filepath.c_str(), &width, &height, &comps, req_comps);

    unsigned char* grayBuffer = grayScaleImage(buffer, req_comps, width, height);
    int grayResult = stbi_write_png("res/textures/Lenna_gray.png", width, height, 1, grayBuffer, width);
    std::cout << "Gray "<< (grayResult ? "Success!" : "Failed") << std::endl;



    stbi_image_free(buffer);
    stbi_image_free(grayBuffer);
    return 0;
}
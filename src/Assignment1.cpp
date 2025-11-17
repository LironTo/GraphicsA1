#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <glm/glm.hpp>

#include <iostream>


unsigned char* grayScaleImage(std::string filepath, int req_comps, int *width, int *height, int *comps){

    float redMultiplier = 0.2989; // NTSC/PAL Standard
    float greenMultiplier = 0.587;
    float blueMultiplier = 0.114;

    unsigned char * buffer = stbi_load(filepath.c_str(), width, height, comps, req_comps);
    if (buffer == nullptr) {
        std::cerr << "Failed to load image: " << filepath << std::endl;
        return nullptr;
    }

    unsigned char* grayBuffer = new unsigned char[(*width) * (*height)];
    for(int i = 0; i < (*width) * (*height); i++){
        glm::vec3 colors = glm::vec3(buffer[i * req_comps], buffer[i * req_comps + 1], buffer[i * req_comps + 2]);
        grayBuffer[i] = redMultiplier * colors.r + greenMultiplier * colors.g + blueMultiplier * colors.b;
    }

    stbi_image_free(buffer);
    return grayBuffer;
}


int main(void)
{
    std::string filepath = "res/textures/plane.png";
    std::string newfilepath = "res/textures/plane_new.png";
    int width, height, comps;
    int req_comps = 4;
    unsigned char * buffer = stbi_load(filepath.c_str(), &width, &height, &comps, req_comps);
    int result = stbi_write_png(newfilepath.c_str(), width, height, req_comps, buffer, width * comps);
    std::cout << "Image write result: " << std::endl;
    std::cout << "Width: " << width << ", Height: " << height << ", Components: " << comps << std::endl;
    std::cout << "Save test " << (result ? "Success!" : "Failed") << std::endl;

    std::cout << "Converting to Grayscale..." << std::endl;
    std::string grayfilepath = "res/textures/Lenna.png";
    int width_gray, height_gray, comps_gray;
    int req_comps_lenna = 4;
    unsigned char* grayBuffer = grayScaleImage(grayfilepath, req_comps_lenna, &width_gray, &height_gray, &comps_gray);
    std::string graynewfilepath = "res/textures/Lenna_gray.png";
    int grayResult = stbi_write_png(graynewfilepath.c_str(), width_gray, height_gray, 1, grayBuffer, width_gray);
    std::cout << "Gray "<< (grayResult ? "Success!" : "Failed") << std::endl;

    stbi_image_free(buffer);
    stbi_image_free(grayBuffer);
    return 0;
}
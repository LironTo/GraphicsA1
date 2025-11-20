#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <glm/glm.hpp>

#include <iostream>

struct Image {
    unsigned char* buffer;
    int width;
    int height;
    int channels;
};

unsigned char* getPixelSafe(int x, int y, Image img){ // Only for single channel images
    if (x < 0 || x >= img.width) throw std::out_of_range("x coordinate is out of image bounds");
    if (y < 0 || y >= img.height) throw std::out_of_range("y coordinate is out of image bounds");
    return &img.buffer[y * img.width + x];
}

Image grayScaleImage(Image img){

    if (img.buffer == nullptr) {
        std::cerr << "Failed to load image " << std::endl;
        return {nullptr, 0, 0, 0};
    }

    float redMultiplier = 0.2989; // NTSC/PAL Standard
    float greenMultiplier = 0.587;
    float blueMultiplier = 0.114;

    Image grayImg;
    grayImg.width = img.width;
    grayImg.height = img.height;
    grayImg.channels = 1;

    unsigned char* grayBuffer = new unsigned char[img.width * img.height];
    for(int i = 0; i < img.width * img.height; i++){
        glm::vec3 colors = glm::vec3(img.buffer[i * img.channels], 
                                    img.buffer[i * img.channels + 1], 
                                    img.buffer[i * img.channels + 2]);
        grayBuffer[i] = redMultiplier * colors.r + greenMultiplier * colors.g + blueMultiplier * colors.b;
    }

    grayImg.buffer = grayBuffer;

    return grayImg;
}

Image noiseReductGauss(Image img){
    
    if (img.buffer == nullptr) {
        std::cerr << "Failed to load image " << std::endl;
        return {nullptr, 0, 0, 0};
    }

    float mat[5][5] = {
        {2,  4,  5,  4, 2},
        {4,  9, 12,  9, 4},
        {5, 12, 15, 12, 5},
        {4,  9, 12,  9, 4},
        {2,  4,  5,  4, 2}
    };

    Image smoothImg;
    smoothImg.width = img.width;
    smoothImg.height = img.height;
    smoothImg.channels = 1;

    unsigned char* smoothBuffer = new unsigned char[img.width * img.height];

    for(int i = 0; i < img.width * img.height; i++){
        int left = -2, right = 2, up = -2, down = 2;
        // Adjust kernel for edge pixels
        if (i % img.width < 2) left = -(i % img.width);
        if (i % img.width > img.width - 3) right = img.width - 1 - (i % img.width);
        if (i / img.width < 2) up = -(i / img.width);
        if (i / img.width > img.height - 3) down = img.height - 1 - (i / img.width);
        int sum = 0;
        for (int j = up; j <= down; j++){
            for (int k = left; k <= right; k++){
                sum += mat[j + 2][k + 2];
            }
        }

        int pixelValue = 0;
        for (int j = up; j <= down; j++){
            for (int k = left; k <= right; k++){
                pixelValue += (*getPixelSafe((i % img.width) + k, (i / img.width) + j, img)) * mat[j + 2][k + 2] / sum;
            }
        }
        smoothBuffer[i] = pixelValue;
    }
    
    smoothImg.buffer = smoothBuffer;

    return smoothImg;
}

Image cannyImage(Image img){
    
    if (img.buffer == nullptr) {
        std::cerr << "Failed to load image " << std::endl;
        return {nullptr, 0, 0, 0};
    }

    Image grayImg = grayScaleImage(img);

    if (grayImg.buffer == nullptr) {
        std::cerr << "Failed to convert to grayscale " << std::endl;
        return {nullptr, 0, 0, 0};
    }

    Image smoothImg = noiseReductGauss(grayImg);
    stbi_image_free(grayImg.buffer);

    return smoothImg;
}

int main(void)
{
    std::string filepath = "res/textures/Lenna.png";
    Image img;
    int req_comps = 4;
    unsigned char *buffer = stbi_load(filepath.c_str(), &img.width, &img.height, &img.channels, req_comps);
    if (buffer == nullptr) {
        std::cerr << "Failed to load image " << filepath << std::endl;
        return -1;
    }
    img.buffer = buffer;

    Image grayImg = grayScaleImage(img);
    int grayResult = stbi_write_png("res/textures/Lenna_gray.png", grayImg.width, grayImg.height, 1, grayImg.buffer, grayImg.width);
    std::cout << "GrayScale creation "<< (grayResult ? "Succeed!" : "Failed :()") << std::endl;

    Image cannyImg = cannyImage(img);
    int cannyResult = stbi_write_png("res/textures/Lenna_canny.png", cannyImg.width, cannyImg.height, 1, cannyImg.buffer, cannyImg.width);
    std::cout << "Canny creation "<< (cannyResult ? "Succeed!" : "Failed :(") << std::endl;

    stbi_image_free(img.buffer);
    stbi_image_free(grayImg.buffer);
    stbi_image_free(cannyImg.buffer);
    return 0;
}
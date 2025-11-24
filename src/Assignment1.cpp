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

//halftone macros
#define HALFTONE_SIZE_MULTIPLYER 2
#define HALFTONE_VALUE_RANGE_WIDTH 51
#define BLACK 0
#define WHITE 255

#define halftonePlacementTopLeft(x, y, width) (HALFTONE_SIZE_MULTIPLYER * x + HALFTONE_SIZE_MULTIPLYER * y * width)
#define halftonePlacementTopRight(x, y, width) (HALFTONE_SIZE_MULTIPLYER* x + HALFTONE_SIZE_MULTIPLYER * y * width + 1)
#define halftonePlacementDownRight(x, y, width) (HALFTONE_SIZE_MULTIPLYER * x + (HALFTONE_SIZE_MULTIPLYER * y + 1) * width + 1)
#define halftonePlacementDownLeft(x, y, width) (HALFTONE_SIZE_MULTIPLYER * x + (HALFTONE_SIZE_MULTIPLYER * y + 1) * width )

//floyed steinberg dithering macros
#define FS_RIGHT_PIXEL_ERROR_FRACTION 7.0/16.0
#define FS_BOTTOM_LEFT_PIXEL_ERROR_FRACTION 3.0/16.0
#define FS_BOTTOM_PIXEL_ERROR_FRACTION 5.0/16.0
#define FS_BOTTOM_RIGHT_PIXEL_ERROR_FRACTION 1.0/16.0
#define index(x, y, width) (x + y * width)
#define indexRight(x, y, width) (x + 1 + y * width)
#define indexBottomLeft(x, y, width) (x -1 + (y + 1) * width)
#define indexBottom(x, y, width) (x + (y + 1) * width)
#define indexBottomRight(x, y, width) (x + 1 + (y + 1) * width)
#define FS_VALUE_RANGE_WIDTH 16



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

/**
 * Applies a halftone effect to the image at the given filepath.
 * square shows the Halftone pattern  
 *(each pixel becomes 4 pixels in the new black and white picture). 
 * @param filepath The path to the input image file.
 * @param req_comps The number of color components to load (e.g., 3 for RGB, 4 for RGBA).
 * @param width Pointer to store the width of the image.
 * @param height Pointer to store the height of the image.
 * @param comps Pointer to store the number of components in the image.
 * @return A pointer to the newly allocated buffer containing the halftone image data.
 * 
 */
unsigned char* halftoneImage(std::string filepath, int req_comps, int *width, int *height, int *comps){


    int NEW_SIZE_MULTIPLYER = 2; // Each pixel becomes 4 pixels (2x2)
    int VALUE_RANGE_WIDTH = 255 / 5; // 5 ranges: 0- 51, 52-102, 103-153, 154-204, 205-255
    int resWidth = (*width) * NEW_SIZE_MULTIPLYER;
    int resHeight = (*height) * NEW_SIZE_MULTIPLYER;
    unsigned int topLeftPlace, topRightPlace, downRightPlace, downLeftPlace;

    unsigned char * buffer = stbi_load(filepath.c_str(), width, height, comps, req_comps);
    if (buffer == nullptr) {
        std::cerr << "Failed to load image: " << filepath << std::endl;
        return nullptr;
    }
    unsigned char* halfBuffer = new unsigned char[(resWidth) * (resHeight) * req_comps];
    for(int i = 0; i < (*height) * req_comps; i++){
        for(int j = 0; j < (*width); j++){

            int valueRange = buffer[i * (*width) + j] / VALUE_RANGE_WIDTH;
            topLeftPlace = halftonePlacementTopLeft(j, i, resWidth);
            topRightPlace = halftonePlacementTopRight(j, i, resWidth);
            downRightPlace = halftonePlacementDownRight(j, i, resWidth);
            downLeftPlace = halftonePlacementDownLeft(j, i, resWidth);

            switch (valueRange)
            {//TODO: uses marcos to calculate the position in the new buffer
            case 0: // 
                halfBuffer[topLeftPlace] = BLACK;
                halfBuffer[topRightPlace] = BLACK;
                halfBuffer[downLeftPlace] = BLACK;
                halfBuffer[downRightPlace] = BLACK;
                break;
            case 1: // 64-127 -> top-left white
                halfBuffer[topLeftPlace] = WHITE;
                halfBuffer[topRightPlace] = BLACK;
                halfBuffer[downLeftPlace] = BLACK;
                halfBuffer[downRightPlace] = BLACK;
                break;
            case 2: // 128-191 -> top-left, bottom-right white
                halfBuffer[topLeftPlace] = WHITE;
                halfBuffer[topRightPlace] = BLACK;
                halfBuffer[downLeftPlace] = BLACK;
                halfBuffer[downRightPlace] = WHITE;
                break;
            case 3: // 192-255 -> top-left, bottom-right, top-right white
                halfBuffer[topLeftPlace] = WHITE;
                halfBuffer[topRightPlace] = WHITE;
                halfBuffer[downLeftPlace] = BLACK;
                halfBuffer[downRightPlace] = WHITE;
                break;
            case 4: // 192-255 -> all white
                halfBuffer[topLeftPlace] = WHITE;
                halfBuffer[topRightPlace] = WHITE;
                halfBuffer[downLeftPlace] = WHITE;
                halfBuffer[downRightPlace] = WHITE;
                break;
            default:
                break;
            }
        }
    }
    stbi_image_free(buffer);
    return halfBuffer;
}

void distributeError(unsigned char* buffer, int i, int j, int *width, int *height, int error){
    buffer[indexRight(i, j, *width)] += error * FS_RIGHT_PIXEL_ERROR_FRACTION;
    buffer[indexBottomLeft(i, j, *width)] += error * FS_BOTTOM_LEFT_PIXEL_ERROR_FRACTION;
    buffer[indexBottom(i, j, *width)] += error * FS_BOTTOM_PIXEL_ERROR_FRACTION;
    buffer[indexBottomRight(i, j, *width)] += error * FS_BOTTOM_RIGHT_PIXEL_ERROR_FRACTION;
}

void distributeErrorRightmostColumn(unsigned char*  buffer, int j, int *width, int *height, int error){
    //Guidance: Use the following Error Diffusion Dither formula (if you have missing edges, split the error equally 
//between the available neighbour pixels)
    buffer[indexBottomLeft((*width) - 1, j, *width)] += error * (FS_BOTTOM_LEFT_PIXEL_ERROR_FRACTION + 
        (FS_RIGHT_PIXEL_ERROR_FRACTION + FS_BOTTOM_RIGHT_PIXEL_ERROR_FRACTION) / 2.0);
    buffer[indexBottom((*width) -1, j, *width)] += error * (FS_BOTTOM_PIXEL_ERROR_FRACTION + 
        (FS_RIGHT_PIXEL_ERROR_FRACTION + FS_BOTTOM_RIGHT_PIXEL_ERROR_FRACTION) / 2.0);
}
void distributeErrorBottomRow(unsigned char* buffer, int i, int j, int *width, int *height, int error){
    // Bottom pixels does not exist
    buffer[indexRight(i, j, *width)] += error ;
    /* all of this is distributed to the right pixel so no need to distribute further, this is equivalent to:
     * (FS_RIGHT_PIXEL_ERROR_FRACTION +  
        FS_RIGHT_PIXEL_ERROR_FRACTION * (
        FS_BOTTOM_LEFT_PIXEL_ERROR_FRACTION + FS_BOTTOM_PIXEL_ERROR_FRACTION + FS_BOTTOM_RIGHT_PIXEL_ERROR_FRACTION) ); */
}

unsigned char *FloyedSteinbergDithering(std::string filepath, int req_comps, int *width, int *height, int *comps){
    double distributionCoeffRight, distributionCoeffBottomLeft, distributionCoeffBottom, distributionCoeffBottomRight;
    unsigned char * buffer = stbi_load(filepath.c_str(), width, height, comps, req_comps);
    int error;

    if (buffer == nullptr) {
        std::cerr << "Failed to load image: " << filepath << std::endl;
        return nullptr;
    }

    unsigned char* ditheredBuffer = new unsigned char[(*width) * (*height)];
    for(int j = 0; j < (*height) - 1; j++){
        for(int i = 0; i < (*width) - 1; i++){
        
        error = buffer[index(i, j, *width)] % FS_VALUE_RANGE_WIDTH; 
        
        ditheredBuffer[index(i, j, *width)] =  buffer[index(i, j, *width)] - error;
        // Distribute the error to neighboring pixels
        distributeError(buffer, i, j, width, height, error);
        }
        error = buffer[index((*width) - 1,j , *width)] % FS_VALUE_RANGE_WIDTH; 
        ditheredBuffer[index((*width) - 1,j, *width)] =  buffer[index((*width) - 1,j, *width)] - error;
        distributeErrorRightmostColumn(buffer, j, width, height, error);
    }
    for(int i = 0; i < (*width) - 1; i++){
        error = buffer[index(i, ((*height) - 1), *width)] % FS_VALUE_RANGE_WIDTH; 
        ditheredBuffer[index(i, ((*height) - 1), *width)] =  buffer[index(i, ((*height) - 1), *width)] - error;
        distributeErrorBottomRow(buffer, i, ((*height) - 1), width, height, error);
    }
    //the last pixel (bottom-right corner)
        error = buffer[index(((*width) - 1), ((*height) - 1), *width)] % FS_VALUE_RANGE_WIDTH;
        ditheredBuffer[index(((*width) - 1), ((*height) - 1), *width)] =  buffer[index(((*width) - 1), ((*height) - 1), *width)] - error;

    stbi_image_free(buffer);
    return ditheredBuffer;
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

    std::string grayfilepath = "res/textures/Lenna.png";
    std::string halfnewfilepath = "res/textures/Lenna_halftone.png";
    std::string graynewfilepath = "res/textures/Lenna_gray.png";
    int width_gray = grayImg.width;
    int height_gray = grayImg.height;
    int comps_gray = grayImg.channels;

        // Halftone effect
    std::cout << "Applying Halftone Effect on the gray Scale Image..." << std::endl;
    unsigned char* halfBuffer = halftoneImage(graynewfilepath, 1, &width_gray, &height_gray, &comps_gray);
    int halfResult = stbi_write_png(halfnewfilepath.c_str(), width_gray * 2, height_gray * 2, 1, halfBuffer, width_gray * 2);

    std::cout << "Halftone "<< (halfResult ? "Success!" : "Failed") << std::endl;
   

    // Floyed Steinberg Dithering
    std::cout << "Applying Floyed Steinberg Dithering on the gray Scale Image..." << std::endl;
    std::string ditherednewfilepath = "res/textures/Lenna_dithered.png";
    unsigned char* ditheredBuffer = FloyedSteinbergDithering(grayfilepath, 1, &width_gray, &height_gray, &comps_gray);
    int ditheredResult = stbi_write_png(ditherednewfilepath.c_str(), width_gray, height_gray, 1, ditheredBuffer, width_gray);
    std::cout << "Dithering "<< (ditheredResult ? "Success!" : "Failed") << std::endl;


    stbi_image_free(img.buffer);
    stbi_image_free(grayImg.buffer);
    stbi_image_free(cannyImg.buffer);
    stbi_image_free(halfBuffer);
    stbi_image_free(ditheredBuffer);

    return 0;
}